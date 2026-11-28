// SPDX-FileCopyrightText: 2018 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "util/StreamLink.hpp"

#include "Application.hpp"
#include "common/LinkParser.hpp"
#include "common/QLogging.hpp"
#include "common/Version.hpp"
#include "singletons/Settings.hpp"
#include "singletons/WindowManager.hpp"
#include "providers/kick/KickChannel.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/youtube/YouTubeChannel.hpp"
#include "util/MultiChannel.hpp"
#include "widgets/dialogs/QualityPopup.hpp"
#include "widgets/dialogs/StreamLinkErrorPopup.hpp"
#include "widgets/Notebook.hpp"
#include "widgets/splits/Split.hpp"
#include "widgets/Window.hpp"

#include <QErrorMessage>
#include <QFileInfo>
#include <QProcess>
#include <QStringBuilder>

#include <array>
#include <functional>

namespace {

using namespace chatterino;

QString getStreamlinkPath()
{
    if (getSettings()->streamlinkUseCustomPath)
    {
        const QString path = getSettings()->streamlinkPath;
        return path.trimmed() % "/" % STREAMLINK_BINARY_NAME;
    }

    return STREAMLINK_BINARY_NAME.toString();
}

void showStreamlinkNotFoundError()
{
    static auto *msg = new QErrorMessage;
    msg->setWindowTitle("Leafyrino - streamlink not found");

    if (getSettings()->streamlinkUseCustomPath)
    {
        msg->showMessage("Unable to find Streamlink executable\nMake sure "
                         "your custom path is pointing to the DIRECTORY "
                         "where the streamlink executable is located");
    }
    else
    {
        msg->showMessage(
            "Unable to find Streamlink executable.\nIf you have Streamlink "
            "installed, you might need to enable the custom path option");
    }
}

QProcess *createStreamlinkProcess()
{
    auto *p = new QProcess;

    const auto path = getStreamlinkPath();

    if (Version::instance().isFlatpak())
    {
        p->setProgram("flatpak-spawn");
        p->setArguments({"--host", path});
    }
    else
    {
        p->setProgram(path);
    }

    p->setProcessChannelMode(QProcess::MergedChannels);

    QObject::connect(p, &QProcess::errorOccurred, [=](auto err) {
        if (err == QProcess::FailedToStart)
        {
            showStreamlinkNotFoundError();
        }
        else
        {
            qCWarning(chatterinoStreamlink) << "Error occurred" << err;
        }

        p->deleteLater();
    });

    QObject::connect(p,
                     static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(
                         &QProcess::finished),
                     [=](int, QProcess::ExitStatus) {
                         p->deleteLater();
                     });

    return p;
}

void getStreamQualitiesFallback(const QStringList &urls, size_t index,
                                std::function<void(QString, QStringList)> cb)
{
    if (index >= static_cast<size_t>(urls.size()))
    {
        if (!urls.isEmpty())
        {
            StreamLinkErrorPopup::showError(
                urls.constLast(), "No configured Streamlink source is available.");
        }
        return;
    }

    auto *p = createStreamlinkProcess();
    QObject::connect(
        p,
        static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(
            &QProcess::finished),
        [=](int exitCode, QProcess::ExitStatus) {
            QString output = QString(p->readAllStandardOutput());
            if (exitCode == 0)
            {
                QString lastLine =
                    output.trimmed().split('\n').last().trimmed();
                if (lastLine.startsWith("Available streams: "))
                {
                    QStringList options;
                    QStringList split =
                        lastLine.right(lastLine.length() - 19).split(", ");

                    for (auto i = split.length() - 1; i >= 0; i--)
                    {
                        QString option = split.at(i);
                        if (option == "best)")
                        {
                            option = split.at(--i);
                            options << option.left(option.length() - 7);
                        }
                        else if (option.endsWith(" (worst)"))
                        {
                            options << option.left(option.length() - 8);
                        }
                        else if (option.endsWith(" (best)"))
                        {
                            options << option.left(option.length() - 7);
                        }
                        else
                        {
                            options << option;
                        }
                    }

                    cb(urls.at(static_cast<int>(index)), options);
                    p->deleteLater();
                    return;
                }
            }

            p->deleteLater();
            getStreamQualitiesFallback(urls, index + 1, cb);
        });

    p->setArguments(p->arguments() +
                    QStringList{urls.at(static_cast<int>(index)),
                                "--default-stream=KKona"});
    p->start();
}

}  // namespace

namespace chatterino {

void getStreamQualities(const QString &channelURL,
                        std::function<void(QStringList)> cb)
{
    auto *p = createStreamlinkProcess();

    QObject::connect(
        p,
        static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(
            &QProcess::finished),
        [=](int exitCode, QProcess::ExitStatus) {
            if (exitCode != 0)
            {
                qCWarning(chatterinoStreamlink) << "Got error code" << exitCode;
            }
            QString output = QString(p->readAllStandardOutput());
            QString lastLine = output.trimmed().split('\n').last().trimmed();
            if (!lastLine.startsWith("Available streams: "))
            {
                StreamLinkErrorPopup::showError(channelURL, output);
                return;
            }

            QStringList options;
            QStringList split =
                lastLine.right(lastLine.length() - 19).split(", ");

            for (auto i = split.length() - 1; i >= 0; i--)
            {
                QString option = split.at(i);
                if (option == "best)")
                {
                    // As it turns out, sometimes, one quality option can
                    // be the best and worst quality at the same time.
                    // Since we start loop from the end, we can check
                    // that and act accordingly
                    option = split.at(--i);
                    // "900p60 (worst"
                    options << option.left(option.length() - 7);
                }
                else if (option.endsWith(" (worst)"))
                {
                    options << option.left(option.length() - 8);
                }
                else if (option.endsWith(" (best)"))
                {
                    options << option.left(option.length() - 7);
                }
                else
                {
                    options << option;
                }
            }

            cb(options);
        });

    p->setArguments(p->arguments() +
                    QStringList{channelURL, "--default-stream=KKona"});

    p->start();
}

void openStreamlink(const QString &url, const QString &quality,
                    QStringList extraArguments)
{
    auto *proc = createStreamlinkProcess();
    QObject::connect(proc, &QProcess::finished,
                     [=](int exitCode, QProcess::ExitStatus /*exitStatus*/) {
                         if (exitCode != 0)
                         {
                             StreamLinkErrorPopup::showError(
                                 url, proc->readAllStandardOutput());
                         }
                     });

    auto arguments = proc->arguments()
                     << std::move(extraArguments) << url << quality;

    arguments.removeAll(QString());

    QString additionalOptions = getSettings()->streamlinkOpts.getValue();
    arguments << QProcess::splitCommand(additionalOptions);

    proc->setArguments(arguments);
    proc->start();
}

namespace {

void openStreamlinkFallback(const QStringList &urls, size_t index,
                            const QString &quality,
                            QStringList extraArguments)
{
    if (index >= static_cast<size_t>(urls.size()))
    {
        if (!urls.isEmpty())
        {
            StreamLinkErrorPopup::showError(
                urls.constLast(), "No configured Streamlink source is available.");
        }
        return;
    }

    auto *proc = createStreamlinkProcess();
    QObject::connect(
        proc, &QProcess::finished,
        [=](int exitCode, QProcess::ExitStatus /*exitStatus*/) {
            if (exitCode == 0)
            {
                proc->deleteLater();
                return;
            }

            proc->deleteLater();
            openStreamlinkFallback(urls, index + 1, quality,
                                   std::move(extraArguments));
        });

    auto arguments = proc->arguments()
                     << std::move(extraArguments)
                     << urls.at(static_cast<int>(index)) << quality;
    arguments.removeAll(QString());

    QString additionalOptions = getSettings()->streamlinkOpts.getValue();
    arguments << QProcess::splitCommand(additionalOptions);
    proc->setArguments(arguments);
    proc->start();
}

}  // namespace

void openStreamlinkForMultiChannelSource(
    const MultiChannel &channel, MultiChannel::Platform platform)
{
    for (const auto &child : channel.channels())
    {
        if (child.platform != platform)
        {
            continue;
        }

        switch (platform)
        {
            case MultiChannel::Platform::Kick:
                if (auto *kick =
                        dynamic_cast<KickChannel *>(child.channel.get()))
                {
                    openStreamlinkForChannelOrUrl(
                        u"https://kick.com/" % kick->slug(), u"");
                }
                return;
            case MultiChannel::Platform::YouTube:
                if (auto *youtube =
                        dynamic_cast<YouTubeChannel *>(child.channel.get()))
                {
                    openStreamlinkForChannelOrUrl(youtube->streamUrl(), u"");
                }
                return;
            case MultiChannel::Platform::Twitch:
                openStreamlinkForChannelOrUrl(
                    u"https://www.twitch.tv/" % child.channel->getName(), u"");
                return;
        }
    }
}

void openStreamlinkForMultiChannel(MultiChannel &channel)
{
    if (!channel.streamlinkFallback())
    {
        if (const auto *active = channel.activeChannel())
        {
            openStreamlinkForMultiChannelSource(channel, active->platform);
        }
        return;
    }

    QStringList urls;
    for (const auto &spec : channel.streamlinkPriority())
    {
        for (const auto &child : channel.channels())
        {
            if (child.platform != spec.platform ||
                child.channel->getName().compare(spec.name,
                                                 Qt::CaseInsensitive) != 0)
            {
                continue;
            }

            switch (spec.platform)
            {
                case MultiChannel::Platform::Kick:
                    if (auto *kick =
                            dynamic_cast<KickChannel *>(child.channel.get()))
                    {
                        urls << u"https://kick.com/" % kick->slug();
                    }
                    break;
                case MultiChannel::Platform::YouTube:
                    if (auto *youtube =
                            dynamic_cast<YouTubeChannel *>(child.channel.get()))
                    {
                        urls << youtube->streamUrl();
                    }
                    break;
                case MultiChannel::Platform::Twitch:
                    urls << u"https://www.twitch.tv/" %
                            child.channel->getName();
                    break;
            }
        }
    }

    if (urls.isEmpty())
    {
        return;
    }

    channel.addSystemMessage(
        QString("Opening %1 in Streamlink ...").arg(urls.join(" ")));

    auto preferredQuality = getSettings()->preferredQuality.getEnum();
    if (preferredQuality == StreamLinkPreferredQuality::Choose)
    {
        getStreamQualitiesFallback(
            urls, 0, [=](const QString &successfulUrl,
                         QStringList qualityOptions) {
                QualityPopup::showDialog(successfulUrl, qualityOptions);
            });
        return;
    }

    QStringList args;
    QString quality;
    QString exclude;

    if (preferredQuality == StreamLinkPreferredQuality::High)
    {
        exclude = ">720p30";
        quality = "high,best";
    }
    else if (preferredQuality == StreamLinkPreferredQuality::Medium)
    {
        exclude = ">540p30";
        quality = "medium,best";
    }
    else if (preferredQuality == StreamLinkPreferredQuality::Low)
    {
        exclude = ">360p30";
        quality = "low,best";
    }
    else if (preferredQuality == StreamLinkPreferredQuality::AudioOnly)
    {
        quality = "audio,audio_only";
    }
    else
    {
        quality = "best";
    }

    if (!exclude.isEmpty())
    {
        args << "--stream-sorting-excludes" << exclude;
    }

    openStreamlinkFallback(urls, 0, quality, args);
}

void openStreamlinkForChannelOrUrl(const QString &channelOrUrl,
                                   QStringView prefixURL)
{
    static const QString INFO_TEMPLATE("Opening %1 in Streamlink ...");

    SplitContainer *currentPage = getApp()
                                      ->getWindows()
                                      ->getLastSelectedWindow()
                                      ->getNotebook()
                                      .getSelectedPage();
    if (currentPage != nullptr)
    {
        auto *currentSplit = currentPage->getSelectedSplit();
        if (currentSplit != nullptr)
        {
            currentSplit->getChannel()->addSystemMessage(
                INFO_TEMPLATE.arg(channelOrUrl));
        }
    }

    QString url;
    if (linkparser::parse(channelOrUrl).has_value())
    {
        url = channelOrUrl;
    }
    else
    {
        url = prefixURL % channelOrUrl;
    }

    auto preferredQuality = getSettings()->preferredQuality.getEnum();

    if (preferredQuality == StreamLinkPreferredQuality::Choose)
    {
        getStreamQualities(url, [=](QStringList qualityOptions) {
            QualityPopup::showDialog(url, qualityOptions);
        });

        return;
    }

    QStringList args;

    QString quality;

    QString exclude;

    if (preferredQuality == StreamLinkPreferredQuality::High)
    {
        exclude = ">720p30";
        quality = "high,best";
    }
    else if (preferredQuality == StreamLinkPreferredQuality::Medium)
    {
        exclude = ">540p30";
        quality = "medium,best";
    }
    else if (preferredQuality == StreamLinkPreferredQuality::Low)
    {
        exclude = ">360p30";
        quality = "low,best";
    }
    else if (preferredQuality == StreamLinkPreferredQuality::AudioOnly)
    {
        quality = "audio,audio_only";
    }
    else
    {
        quality = "best";
    }
    if (!exclude.isEmpty())
    {
        args << "--stream-sorting-excludes" << exclude;
    }

    openStreamlink(url, quality, args);
}

}  // namespace chatterino
