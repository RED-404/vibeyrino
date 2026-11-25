// SPDX-FileCopyrightText: 2026 Contributors to Vibeyrino
//
// SPDX-License-Identifier: MIT

#include "util/VibeyrinoIcon.hpp"

#include <QPainter>
#include <QStringLiteral>

namespace chatterino {

QPixmap tintPixmapRed(const QPixmap &source, const QColor &color)
{
    if (source.isNull())
    {
        return source;
    }

    QPixmap out(source.size());
    out.fill(Qt::transparent);

    QPainter painter(&out);
    painter.drawPixmap(0, 0, source);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(out.rect(), color);
    painter.end();

    return out;
}

QPixmap loadBrandBasePixmap()
{
    const QPixmap baked(QStringLiteral(":/icon-vibeyrino.png"));
    if (!baked.isNull())
    {
        return baked;
    }

    return tintPixmapRed(QPixmap(QStringLiteral(":/icon.png")));
}

QPixmap vibeyrinoBrandPixmap()
{
    static QPixmap cached;
    static bool initialized = false;
    if (!initialized)
    {
        initialized = true;
        cached = loadBrandBasePixmap();
    }
    return cached;
}

QIcon vibeyrinoBrandIcon()
{
    static QIcon cached;
    static bool initialized = false;
    if (!initialized)
    {
        initialized = true;
        const QPixmap base = loadBrandBasePixmap();
        if (base.isNull())
        {
            return cached;
        }

        QIcon result;
        for (const int size : {16, 20, 24, 32, 48, 64, 128, 256})
        {
            const QPixmap scaled = base.scaled(
                size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            result.addPixmap(scaled);
        }
        cached = result;
    }
    return cached;
}

}  // namespace chatterino