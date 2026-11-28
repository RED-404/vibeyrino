// SPDX-FileCopyrightText: 2018 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QString>
#include <QStringList>
#include "util/MultiChannel.hpp"

#include <stdexcept>
#include <string>

namespace chatterino {

class Exception : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

#ifdef Q_OS_WIN
constexpr inline QStringView STREAMLINK_BINARY_NAME = u"streamlink.exe";
#else
constexpr inline QStringView STREAMLINK_BINARY_NAME = u"streamlink";
#endif

// Open streamlink for given url, quality and extra arguments
// the "Additional arguments" are fetched and added at the beginning of the
// streamlink call
void openStreamlink(const QString &url, const QString &quality,
                    QStringList extraArguments = QStringList());

// Start opening streamlink for the given channel or url, reading settings like quality
// from settings and opening a quality dialog if the quality is "Choose"
void openStreamlinkForChannelOrUrl(const QString &channelOrUrl,
                                   QStringView prefixURL = u"twitch.tv/");

inline void openStreamlinkForChannel(const QString &channel,
                                     QStringView prefixURL = u"twitch.tv/")
{
    openStreamlinkForChannelOrUrl(channel, prefixURL);
}

// Open the first available source in a multichannel split, using the
// configured Streamlink source priority and falling back to the next source.
void openStreamlinkForMultiChannel(MultiChannel &channel);

// Open one specific platform from a multichannel split.
void openStreamlinkForMultiChannelSource(
    const MultiChannel &channel, MultiChannel::Platform platform);

}  // namespace chatterino
