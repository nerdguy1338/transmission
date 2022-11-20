// This file Copyright © 2022 Mnemosyne LLC.
// It may be used under GPLv2 (SPDX: GPL-2.0-only), GPLv3 (SPDX: GPL-3.0-only),
// or any future license endorsed by Mnemosyne LLC.
// License text can be found in the licenses/ folder.

#pragma once

#include <gtkmm.h>

#include "Utils.h"

class Torrent;

class TorrentFilter : public IF_GTKMM4(Gtk::Filter, Glib::Object)
{
public:
    enum Activity
    {
        ACTIVITY_ALL,
        ACTIVITY_DOWNLOADING,
        ACTIVITY_SEEDING,
        ACTIVITY_ACTIVE,
        ACTIVITY_PAUSED,
        ACTIVITY_FINISHED,
        ACTIVITY_VERIFYING,
        ACTIVITY_ERROR,
    };

    enum Tracker
    {
        TRACKER_ALL,
        TRACKER_HOST,
    };

#if !GTKMM_CHECK_VERSION(4, 0, 0)
    enum class Change{
        DIFFERENT,
        LESS_STRICT,
        MORE_STRICT,
    };
#endif

public:
    void set_activity(Activity type);
    void set_tracker(Tracker type, Glib::ustring const& host);
    void set_text(Glib::ustring const& text);

    bool test_activity(Torrent const& torrent) const;
    bool test_tracker(Torrent const& torrent) const;
    bool test_text(Torrent const& torrent) const;

    bool match(Torrent const& torrent) const;

    void update(Torrent::ChangeFlags flags);

#if !GTKMM_CHECK_VERSION(4, 0, 0)
    sigc::signal<void()>& signal_changed();
#endif

    static Glib::RefPtr<TorrentFilter> create();

    static bool test_activity(Torrent const& torrent, Activity type);
    static bool test_tracker(Torrent const& torrent, Tracker type, Glib::ustring const& host);
    static bool test_text(Torrent const& torrent, Glib::ustring const& text);

protected:
#if GTKMM_CHECK_VERSION(4, 0, 0)
    bool match_vfunc(Glib::RefPtr<Glib::ObjectBase> const& item) override;
    Match get_strictness_vfunc() override;
#else
    void changed(Change change);
#endif

private:
    TorrentFilter();

private:
    Activity activity_type_ = ACTIVITY_ALL;
    Tracker tracker_type_ = TRACKER_ALL;
    Glib::ustring tracker_host_;
    Glib::ustring text_;

#if !GTKMM_CHECK_VERSION(4, 0, 0)
    sigc::signal<void()> signal_changed_;
#endif
};
