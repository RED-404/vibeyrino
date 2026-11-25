// SPDX-FileCopyrightText: 2026 Contributors to Vibeyrino
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QColor>
#include <QIcon>
#include <QPixmap>

namespace chatterino {

constexpr QColor VIBEYRINO_BRAND_RED{0xea, 0x69, 0x56};

/** Tint a pixmap (expects alpha) with the Vibeyrino accent red. */
QPixmap tintPixmapRed(const QPixmap &source,
                      const QColor &color = VIBEYRINO_BRAND_RED);

QPixmap vibeyrinoBrandPixmap();
QIcon vibeyrinoBrandIcon();

}  // namespace chatterino