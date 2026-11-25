#pragma once

#include "util/QStringHash.hpp"

#include <boost/unordered/unordered_flat_map.hpp>
#include <QObject>

#include <memory>

namespace chatterino {

class Channel;
class NotebookTab;
class SplitContainer;
class YouTubeChannel;

class YouTubeChatServer : public QObject
{
public:
    YouTubeChatServer();
    ~YouTubeChatServer() override;

    Q_DISABLE_COPY_MOVE(YouTubeChatServer)

    void initialize();

    std::shared_ptr<Channel> getOrCreate(const QString &name);

    std::shared_ptr<YouTubeChannel> findByName(const QString &name) const;

    /** Re-resolve YouTube when a live tab is selected but YouTube is offline. */
    void refreshOfflineYouTubeOnLiveTabFocus(SplitContainer *container,
                                             const NotebookTab *tab);

private:
    boost::unordered_flat_map<QString, std::weak_ptr<YouTubeChannel>>
        channelsByName_;
};

}  // namespace chatterino
