#include "providers/kick/KickLiveUpdates.hpp"

#include "common/FlagsEnum.hpp"
#include "common/network/NetworkRequest.hpp"
#include "common/network/NetworkResult.hpp"
#include "common/QLogging.hpp"
#include "debug/AssertInGuiThread.hpp"
#include "providers/kick/ws/KickCentrifugoManager.hpp"
#include "providers/kick/ws/KickPusherManager.hpp"
#include "providers/kick/ws/KickWebSocketManager.hpp"
#include "singletons/Settings.hpp"

#include <boost/unordered/unordered_flat_set.hpp>

#include <deque>
#include <unordered_map>

using namespace Qt::Literals;

namespace chatterino {

namespace {

constexpr auto KICK_ORIGIN = "https://kick.com";
constexpr auto KICK_REFERER = "https://kick.com/";

const QString KICK_CENTRIFUGO_FALLBACK =
    u"wss://realtime.us-east-1.platform.kick.com/connection/websocket"_s;

constexpr int MAX_CONCURRENT_CONNECTION_REQUESTS = 2;

FlagsEnum<KickConnectionPreference> currentPrefs()
{
    FlagsEnum<KickConnectionPreference> pref =
        getSettings()->kickConnectionPreference;
    if (pref == KickConnectionPreference::Default)
    {
        pref = KickConnectionPreference::Any;
    }
    return pref;
}

NetworkRequest kickRealtimeRequest(const QUrl &url,
                                   NetworkRequestType requestType =
                                       NetworkRequestType::Post)
{
    return NetworkRequest(url, requestType)
        .header("Origin", KICK_ORIGIN)
        .header("Referer", KICK_REFERER);
}

QJsonObject kickConnectionBody(const QString &clientID)
{
    auto pref = currentPrefs();
    QJsonArray accepted;
    if (pref.has(KickConnectionPreference::Pusher))
    {
        accepted.append(QJsonObject{{"provider"_L1, "pusher"_L1}});
    }
    if (pref.has(KickConnectionPreference::Centrifugo))
    {
        accepted.append(QJsonObject{{"provider"_L1, "centrifugo"_L1}});
    }

    return QJsonObject{
        {
            "client"_L1,
            QJsonObject{
                {"id"_L1, clientID},
                {"type"_L1, "web"_L1},
            },
        },
        {
            "capabilities"_L1,
            QJsonObject{{"accepted_providers"_L1, accepted}},
        },
    };
}

// Subscriptions needed for chat messages and most live events without opening
// six sockets per channel (which multiplies quickly with many tabs open).
void subscribeRoom(KickWebSocketManager &manager, uint64_t roomID,
                   uint64_t channelID)
{
    manager.joinChannel(u"chatrooms." % QString::number(roomID) % u".v2");
    manager.joinChannel(u"channel." % QString::number(channelID));
}

void unsubscribeRoom(KickWebSocketManager &manager, uint64_t roomID,
                     uint64_t channelID)
{
    manager.partChannel(u"chatrooms." % QString::number(roomID) % u".v2");
    manager.partChannel(u"channel." % QString::number(channelID));
}

QString managerKey(const QJsonObject &connection)
{
    const auto provider = connection["provider"_L1].toString();
    if (provider == u"pusher"_s)
    {
        const auto creds = connection["credentials"_L1].toObject();
        return u"pusher:" % creds["app_key"_L1].toString() % u':' %
               creds["cluster"_L1].toString();
    }
    if (provider == u"centrifugo"_s)
    {
        const auto creds = connection["credentials"_L1].toObject();
        return creds["url"_L1].toString();
    }
    return u"unknown"_s;
}

std::unique_ptr<KickWebSocketManager> makeManagerFromConnection(
    const QJsonObject &connection, const QString &clientID)
{
    const auto provider = connection["provider"_L1].toString();
    if (provider == u"pusher"_s)
    {
        const auto creds = connection["credentials"_L1].toObject();
        const auto appKey = creds["app_key"_L1].toString();
        const auto cluster = creds["cluster"_L1].toString();
        qCDebug(chatterinoKick) << "Using Pusher with app_key:" << appKey
                                << "cluster:" << cluster;
        return std::make_unique<KickPusherManager>(appKey, cluster);
    }
    if (provider == u"centrifugo"_s)
    {
        const auto creds = connection["credentials"_L1].toObject();
        const auto url = creds["url"_L1].toString();
        qCWarning(chatterinoKick) << "Kick realtime Centrifugo:" << url;
        return std::make_unique<KickCentrifugoManager>(url, clientID);
    }

    qCWarning(chatterinoKick)
        << "Unknown Kick realtime provider" << provider
        << "- using Centrifugo fallback";
    return std::make_unique<KickCentrifugoManager>(KICK_CENTRIFUGO_FALLBACK,
                                                     clientID);
}

}  // namespace

class KickLiveUpdatesPrivate
    : public std::enable_shared_from_this<KickLiveUpdatesPrivate>
{
public:
    KickLiveUpdatesPrivate();

    std::unordered_map<QString, std::unique_ptr<KickWebSocketManager>>
        managersByKey;
    std::unordered_map<uint64_t, QString> channelToManagerKey;
    std::unordered_map<uint64_t, std::pair<uint64_t, uint64_t>> roomsByChannel;

    boost::unordered_flat_set<uint64_t> fetchesInProgress;
    std::deque<uint64_t> fetchQueue;

    QString clientID;

    void ensureManager(const QString &key, const QJsonObject &connection);
    void ensureFallbackManager();

    void pumpFetchQueue();
    void requestConnection(uint64_t channelID);

    void subscribeKnown(uint64_t channelID);
    void joinImpl(uint64_t roomID, uint64_t channelID);
    void partImpl(uint64_t roomID, uint64_t channelID);

private:
    friend KickLiveUpdates;
};

KickLiveUpdatesPrivate::KickLiveUpdatesPrivate()
    : clientID(QUuid::createUuid().toString(QUuid::WithoutBraces))
{
}

void KickLiveUpdatesPrivate::ensureManager(const QString &key,
                                           const QJsonObject &connection)
{
    if (this->managersByKey.contains(key))
    {
        return;
    }
    if (key == KICK_CENTRIFUGO_FALLBACK || connection.empty())
    {
        this->managersByKey[key] =
            std::make_unique<KickCentrifugoManager>(key, this->clientID);
        return;
    }
    this->managersByKey[key] =
        makeManagerFromConnection(connection, this->clientID);
}

void KickLiveUpdatesPrivate::ensureFallbackManager()
{
    if (!this->managersByKey.contains(KICK_CENTRIFUGO_FALLBACK))
    {
        qCWarning(chatterinoKick)
            << "Using fallback Kick Centrifugo" << KICK_CENTRIFUGO_FALLBACK;
        this->managersByKey[KICK_CENTRIFUGO_FALLBACK] =
            std::make_unique<KickCentrifugoManager>(KICK_CENTRIFUGO_FALLBACK,
                                                     this->clientID);
    }
}

void KickLiveUpdatesPrivate::subscribeKnown(uint64_t channelID)
{
    const auto keyIt = this->channelToManagerKey.find(channelID);
    if (keyIt == this->channelToManagerKey.end())
    {
        return;
    }
    const auto mgrIt = this->managersByKey.find(keyIt->second);
    const auto roomIt = this->roomsByChannel.find(channelID);
    if (mgrIt == this->managersByKey.end() ||
        roomIt == this->roomsByChannel.end())
    {
        return;
    }
    const auto [roomID, mappedChannelID] = roomIt->second;
    subscribeRoom(*mgrIt->second, roomID, mappedChannelID);
}

void KickLiveUpdatesPrivate::pumpFetchQueue()
{
    assertInGuiThread();

    while (this->fetchesInProgress.size() <
               static_cast<size_t>(MAX_CONCURRENT_CONNECTION_REQUESTS) &&
           !this->fetchQueue.empty())
    {
        uint64_t channelID = this->fetchQueue.front();
        this->fetchQueue.pop_front();

        if (this->channelToManagerKey.contains(channelID) ||
            this->fetchesInProgress.contains(channelID))
        {
            continue;
        }

        this->requestConnection(channelID);
    }
}

void KickLiveUpdatesPrivate::requestConnection(uint64_t channelID)
{
    assertInGuiThread();
    if (channelID == 0 || this->fetchesInProgress.contains(channelID) ||
        this->channelToManagerKey.contains(channelID))
    {
        return;
    }

    this->fetchesInProgress.insert(channelID);

    const QString endpoint =
        u"https://web.kick.com/api/v1/realtime/channels/"_s %
        QString::number(channelID) % u"/chat/connection"_s;

    kickRealtimeRequest(QUrl(endpoint))
        .json(kickConnectionBody(this->clientID))
        .onSuccess([weak = this->weak_from_this(),
                    channelID](const NetworkResult &res) {
            auto self = weak.lock();
            if (!self)
            {
                return;
            }

            const auto json = res.parseJson();
            const auto connection =
                json["data"_L1]["connections"_L1][0].toObject();
            const auto key = managerKey(connection);
            self->channelToManagerKey[channelID] = key;
            self->ensureManager(key, connection);
            self->subscribeKnown(channelID);

            self->fetchesInProgress.erase(channelID);
            self->pumpFetchQueue();
        })
        .onError([weak = this->weak_from_this(),
                  channelID](const NetworkResult &res) {
            auto self = weak.lock();
            if (!self)
            {
                return;
            }

            qCWarning(chatterinoKick)
                << "Kick connection lookup failed for channel" << channelID
                << res.formatError();

            self->channelToManagerKey[channelID] = KICK_CENTRIFUGO_FALLBACK;
            self->ensureFallbackManager();
            self->subscribeKnown(channelID);

            self->fetchesInProgress.erase(channelID);
            self->pumpFetchQueue();
        })
        .execute();
}

void KickLiveUpdatesPrivate::joinImpl(uint64_t roomID, uint64_t channelID)
{
    assertInGuiThread();
    if (channelID == 0)
    {
        qCWarning(chatterinoKick)
            << "Tried to join Kick room" << roomID << "without channel id";
        return;
    }

    this->roomsByChannel[channelID] = {roomID, channelID};

    if (this->channelToManagerKey.contains(channelID))
    {
        this->subscribeKnown(channelID);
        return;
    }

    this->fetchQueue.push_back(channelID);
    this->pumpFetchQueue();
}

void KickLiveUpdatesPrivate::partImpl(uint64_t roomID, uint64_t channelID)
{
    assertInGuiThread();

    this->roomsByChannel.erase(channelID);

    if (channelID == 0)
    {
        return;
    }

    const auto keyIt = this->channelToManagerKey.find(channelID);
    if (keyIt == this->channelToManagerKey.end())
    {
        return;
    }

    const auto mgrIt = this->managersByKey.find(keyIt->second);
    if (mgrIt == this->managersByKey.end())
    {
        return;
    }

    unsubscribeRoom(*mgrIt->second, roomID, channelID);
}

KickLiveUpdates::KickLiveUpdates()
    : private_(std::make_shared<KickLiveUpdatesPrivate>())
{
}
KickLiveUpdates::~KickLiveUpdates() = default;

void KickLiveUpdates::joinRoom(uint64_t roomID, uint64_t channelID)
{
    this->private_->joinImpl(roomID, channelID);
}

void KickLiveUpdates::leaveRoom(uint64_t roomID, uint64_t channelID)
{
    this->private_->partImpl(roomID, channelID);
}

}  // namespace chatterino