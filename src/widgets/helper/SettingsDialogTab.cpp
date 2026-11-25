// SPDX-FileCopyrightText: 2017 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "widgets/helper/SettingsDialogTab.hpp"

#include "util/VibeyrinoIcon.hpp"
#include "widgets/dialogs/SettingsDialog.hpp"
#include "widgets/settingspages/SettingsPage.hpp"

#include <QPainter>
#include <QStyleOption>

namespace chatterino {

SettingsDialogTab::SettingsDialogTab(SettingsDialog *_dialog,
                                     std::function<SettingsPage *()> _lazyPage,
                                     const QString &name, QString imageFileName,
                                     SettingsTabId id)
    : BaseWidget(_dialog)
    , dialog_(_dialog)
    , lazyPage_(std::move(_lazyPage))
    , id_(id)
    , name_(name)
{
    this->ui_.labelText = name;
    this->ui_.icon.addFile(imageFileName);

    this->setCursor(QCursor(Qt::PointingHandCursor));

    this->setStyleSheet("");
}

void SettingsDialogTab::setSelected(bool _selected)
{
    if (this->selected_ == _selected)
    {
        return;
    }

    //    height: <checkbox-size>px;

    this->selected_ = _selected;
    this->selectedChanged(this->selected_);
}

SettingsPage *SettingsDialogTab::page()
{
    if (this->page_)
    {
        return this->page_;
    }

    this->page_ = this->lazyPage_();
    this->page_->setTab(this);
    return this->page_;
}

SettingsPage *SettingsDialogTab::createdPage() const
{
    return this->page_;
}

void SettingsDialogTab::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    QPainter ppcolor;

    QStyleOption opt;
    opt.initFrom(this);

    this->style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);

    int iconSize = 20 * this->scale();
    int pad = (this->height() - iconSize) / 2;
    const QSize pixmapSize(this->height() - pad * 2, this->height() - pad * 2);
    QPixmap pixmap;
    if (this->id_ == SettingsTabId::Vibeyrino)
    {
        pixmap = vibeyrinoBrandPixmap();
        if (pixmap.size() != pixmapSize)
        {
            pixmap = pixmap.scaled(pixmapSize, Qt::KeepAspectRatio,
                                   Qt::SmoothTransformation);
        }
    }
    else
    {
        pixmap = this->ui_.icon.pixmap(pixmapSize);
        ppcolor.begin(&pixmap);
        ppcolor.setCompositionMode(QPainter::CompositionMode_SourceIn);
        ppcolor.fillRect(pixmap.rect(),
                         this->palette().color(QPalette::Text));
        ppcolor.end();
    }

    painter.drawPixmap(pad, pad, pixmap);

    pad = (3 * pad) + iconSize;

    this->style()->drawItemText(
        &painter, QRect(pad, 0, this->width() - pad, this->height()),
        Qt::AlignLeft | Qt::AlignVCenter, this->palette(), false,
        this->ui_.labelText);
}

void SettingsDialogTab::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
    {
        return;
    }

    this->dialog_->selectTab(this);

    this->setFocus();
}

const QString &SettingsDialogTab::name() const
{
    return this->name_;
}

SettingsTabId SettingsDialogTab::id() const
{
    return this->id_;
}

}  // namespace chatterino
