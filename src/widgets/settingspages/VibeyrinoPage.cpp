// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "widgets/settingspages/VibeyrinoPage.hpp"

#include "singletons/Settings.hpp"
#include "widgets/BaseWindow.hpp"
#include "widgets/settingspages/GeneralPageView.hpp"
#include "widgets/settingspages/SettingWidget.hpp"

#include <QFrame>
#include <QHBoxLayout>
#include <QVBoxLayout>

namespace chatterino {

VibeyrinoPage::VibeyrinoPage()
{
    auto *outer = new QVBoxLayout;
    auto *inner = new QHBoxLayout;
    auto *view = GeneralPageView::withNavigation(this);
    this->view_ = view;

    inner->addWidget(view);
    auto *frame = new QFrame;
    frame->setLayout(inner);
    outer->addWidget(frame);
    this->setLayout(outer);

    this->initLayout(*view);
}

bool VibeyrinoPage::filterElements(const QString &query)
{
    if (this->view_)
    {
        return this->view_->filterElements(query) || query.isEmpty();
    }

    return false;
}

void VibeyrinoPage::initLayout(GeneralPageView &layout)
{
    auto &s = *getSettings();

    layout.addTitle("Notebook");
    layout.addDescription(
        "Options specific to this Vibeyrino build (not part of upstream "
        "Leafyrino).");

    SettingWidget::checkbox(
        "Always show /live and similar tabs in live-only mode",
        s.alwaysShowSlashChannelTabsInLiveOnly)
        ->setTooltip(
            "When tab visibility is set to only live tabs, keep tabs open "
            "for special channels such as /live, /mentions, /watching, "
            "/whispers, and /automod.")
        ->addKeywords({"/live", "mentions", "watching", "whispers", "automod",
                       "slash", "pinned", "tab", "live only"})
        ->addTo(layout);

    layout.addTitle("YouTube");
    SettingWidget::checkbox("Connect to scheduled waiting rooms",
                            s.youtubeShowScheduledWaitingRooms)
        ->addKeywords({"youtube", "scheduled", "premiere", "waiting", "room",
                       "upcoming", "offline"})
        ->setTooltip(
            "When a channel has an upcoming YouTube live (waiting room), join "
            "that chat and mark the split as scheduled. Does not count as live "
            "for tab filters.")
        ->addTo(layout);

    layout.addStretch();

    auto *inv = new BaseWidget(this);
    layout.addWidget(inv);
}

}  // namespace chatterino