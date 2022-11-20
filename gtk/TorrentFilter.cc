// This file Copyright © 2022 Mnemosyne LLC.
// It may be used under GPLv2 (SPDX: GPL-2.0-only), GPLv3 (SPDX: GPL-3.0-only),
// or any future license endorsed by Mnemosyne LLC.
// License text can be found in the licenses/ folder.

#include <libtransmission/transmission.h>

#include "Torrent.h"
#include "TorrentFilter.h"
#include "Utils.h"

void TorrentFilter::set_activity(Activity type)
{
    if (activity_type_ == type)
    {
        return;
    }

    auto change = Change::DIFFERENT;
    if (activity_type_ == ACTIVITY_ALL)
    {
        change = Change::MORE_STRICT;
    }
    else if (type == ACTIVITY_ALL)
    {
        change = Change::LESS_STRICT;
    }

    activity_type_ = type;

    changed(change);
}

void TorrentFilter::set_tracker(Tracker type, Glib::ustring const& host)
{
    if (tracker_type_ == type && tracker_host_ == host)
    {
        return;
    }

    auto change = Change::DIFFERENT;
    if (tracker_type_ != type)
    {
        if (tracker_type_ == TRACKER_ALL)
        {
            change = Change::MORE_STRICT;
        }
        else if (type == TRACKER_ALL)
        {
            change = Change::LESS_STRICT;
        }
    }

    tracker_type_ = type;
    tracker_host_ = host;

    changed(change);
}

void TorrentFilter::set_text(Glib::ustring const& text)
{
    auto const normalized_text = gtr_str_strip(text.casefold());
    if (text_ == normalized_text)
    {
        return;
    }

    auto change = Change::DIFFERENT;
    if (text_.empty())
    {
        change = Change::MORE_STRICT;
    }
    else if (normalized_text.empty())
    {
        change = Change::LESS_STRICT;
    }

    text_ = normalized_text;

    changed(change);
}

bool TorrentFilter::test_activity(Torrent const& torrent) const
{
    return test_activity(torrent, activity_type_);
}

bool TorrentFilter::test_tracker(Torrent const& torrent) const
{
    return test_tracker(torrent, tracker_type_, tracker_host_);
}

bool TorrentFilter::test_text(Torrent const& torrent) const
{
    return test_text(torrent, text_);
}

bool TorrentFilter::match(Torrent const& torrent) const
{
    return test_activity(torrent) && test_tracker(torrent) && test_text(torrent);
}

void TorrentFilter::update(Torrent::ChangeFlags flags)
{
    using ChangeFlag = Torrent::ChangeFlag;

    bool refilter_needed = false;

    if (activity_type_ != ACTIVITY_ALL)
    {
        static auto const activity_flags = std::map<Activity, Torrent::ChangeFlags>({
            { ACTIVITY_DOWNLOADING, Torrent::make_change_flags({ ChangeFlag::ACTIVITY }) },
            { ACTIVITY_SEEDING, Torrent::make_change_flags({ ChangeFlag::ACTIVITY }) },
            { ACTIVITY_ACTIVE, Torrent::make_change_flags({ ChangeFlag::ACTIVE_PEER_COUNT, ChangeFlag::ACTIVITY }) },
            { ACTIVITY_PAUSED, Torrent::make_change_flags({ ChangeFlag::ACTIVITY }) },
            { ACTIVITY_FINISHED, Torrent::make_change_flags({ ChangeFlag::FINISHED }) },
            { ACTIVITY_VERIFYING, Torrent::make_change_flags({ ChangeFlag::ACTIVITY }) },
            { ACTIVITY_ERROR, Torrent::make_change_flags({ ChangeFlag::ERROR_CODE }) },
        });

        auto const activity_flags_it = activity_flags.find(activity_type_);
        refilter_needed = activity_flags_it != activity_flags.end() && (flags & activity_flags_it->second).any();
    }

    if (!refilter_needed)
    {
        refilter_needed = tracker_type_ != TRACKER_ALL && flags.test(ChangeFlag::TRACKERS);
    }

    if (!refilter_needed)
    {
        static auto const text_flags = Torrent::make_change_flags({ ChangeFlag::NAME });

        refilter_needed = !text_.empty() && (flags & text_flags).any();
    }

    if (refilter_needed)
    {
        changed();
    }
}

#if !GTKMM_CHECK_VERSION(4, 0, 0)

sigc::signal<void()>& TorrentFilter::signal_changed()
{
    return signal_changed_;
}

#endif

Glib::RefPtr<TorrentFilter> TorrentFilter::create()
{
    return Glib::make_refptr_for_instance(new TorrentFilter());
}

bool TorrentFilter::test_activity(Torrent const& torrent, Activity type)
{
    auto activity = tr_torrent_activity();

    switch (type)
    {
    case ACTIVITY_DOWNLOADING:
        activity = torrent.get_activity();
        return activity == TR_STATUS_DOWNLOAD || activity == TR_STATUS_DOWNLOAD_WAIT;

    case ACTIVITY_SEEDING:
        activity = torrent.get_activity();
        return activity == TR_STATUS_SEED || activity == TR_STATUS_SEED_WAIT;

    case ACTIVITY_ACTIVE:
        return torrent.get_active_peer_count() > 0 || torrent.get_activity() == TR_STATUS_CHECK;

    case ACTIVITY_PAUSED:
        return torrent.get_activity() == TR_STATUS_STOPPED;

    case ACTIVITY_FINISHED:
        return torrent.get_finished();

    case ACTIVITY_VERIFYING:
        activity = torrent.get_activity();
        return activity == TR_STATUS_CHECK || activity == TR_STATUS_CHECK_WAIT;

    case ACTIVITY_ERROR:
        return torrent.get_error_code() != 0;

    default: /* ACTIVITY_ALL */
        return true;
    }
}

bool TorrentFilter::test_tracker(Torrent const& torrent, Tracker type, Glib::ustring const& host)
{
    if (type != TRACKER_HOST)
    {
        return true;
    }

    auto const& raw_torrent = torrent.get_torrent();

    for (size_t i = 0, n = tr_torrentTrackerCount(&raw_torrent); i < n; ++i)
    {
        if (auto const tracker = tr_torrentTracker(&raw_torrent, i); std::data(tracker.sitename) == host)
        {
            return true;
        }
    }

    return false;
}

bool TorrentFilter::test_text(Torrent const& torrent, Glib::ustring const& text)
{
    bool ret = false;

    if (text.empty())
    {
        ret = true;
    }
    else
    {
        auto const& raw_torrent = torrent.get_torrent();

        /* test the torrent name... */
        ret = torrent.get_name().casefold().find(text) != Glib::ustring::npos;

        /* test the files... */
        for (tr_file_index_t i = 0, n = tr_torrentFileCount(&raw_torrent); i < n && !ret; ++i)
        {
            ret = Glib::ustring(tr_torrentFile(&raw_torrent, i).name).casefold().find(text) != Glib::ustring::npos;
        }
    }

    return ret;
}

#if GTKMM_CHECK_VERSION(4, 0, 0)

bool TorrentFilter::match_vfunc(Glib::RefPtr<Glib::ObjectBase> const& item)
{
    auto const torrent = gtr_ptr_dynamic_cast<Torrent>(item);
    if (torrent == nullptr)
    {
        return false;
    }

    return match(*torrent);
}

TorrentFilter::Match TorrentFilter::get_strictness_vfunc()
{
    return activity_type_ == ACTIVITY_ALL && tracker_type_ == TRACKER_ALL && text_.empty() ? Match::ALL : Match::SOME;
}

#else

void TorrentFilter::changed(Change /*change*/)
{
    signal_changed_.emit();
}

#endif

TorrentFilter::TorrentFilter()
    : Glib::ObjectBase(typeid(TorrentFilter))
{
}
