#include "providers/youtube/YouTubeChatServer.hpp"

#include "providers/youtube/YouTubeChannel.hpp"
#include "util/MultiChannel.hpp"
#include "widgets/helper/NotebookTab.hpp"
#include "widgets/splits/Split.hpp"
#include "widgets/splits/SplitContainer.hpp"

namespace {

using namespace chatterino;

void refreshOfflineYouTubeInChannel(const ChannelPtr &channel)
{
    if (auto *yt = dynamic_cast<YouTubeChannel *>(channel.get()))
    {
        const auto &stream = yt->streamData();
        if (!stream.isLive && !stream.isScheduled)
        {
            yt->refreshLiveStream();
        }
        return;
    }

    if (auto *mc = dynamic_cast<MultiChannel *>(channel.get()))
    {
        for (const auto &child : mc->channels())
        {
            refreshOfflineYouTubeInChannel(child.channel);
        }
    }
}

QString normalizeName(const QString &name)
{
    auto normalized = name.trimmed();
    if (normalized.startsWith('@'))
    {
        normalized = normalized.mid(1);
    }
    return normalized.toLower();
}

}  // namespace

namespace chatterino {

YouTubeChatServer::YouTubeChatServer() = default;

YouTubeChatServer::~YouTubeChatServer() = default;

void YouTubeChatServer::initialize()
{
}

std::shared_ptr<YouTubeChannel> YouTubeChatServer::findByName(
    const QString &name) const
{
    auto it = this->channelsByName_.find(normalizeName(name));
    if (it != this->channelsByName_.end())
    {
        return it->second.lock();
    }
    return nullptr;
}

std::shared_ptr<Channel> YouTubeChatServer::getOrCreate(const QString &name)
{
    auto existing = this->findByName(name);
    if (existing)
    {
        return existing;
    }

    auto displayName = name.trimmed();
    if (displayName.startsWith('@'))
    {
        displayName = displayName.mid(1);
    }

    auto chan = std::make_shared<YouTubeChannel>(displayName);
    this->channelsByName_[normalizeName(name)] = chan;
    chan->refreshLiveStream();
    return chan;
}

void YouTubeChatServer::refreshOfflineYouTubeOnLiveTabFocus(
    SplitContainer *container, const NotebookTab *tab)
{
    if (!container || !tab || !tab->isLive())
    {
        return;
    }

    for (Split *split : container->getSplits())
    {
        if (!split)
        {
            continue;
        }
        refreshOfflineYouTubeInChannel(split->getChannel());
    }
}

}  // namespace chatterino
