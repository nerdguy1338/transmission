// This file Copyright © 2022 Mnemosyne LLC.
// It may be used under GPLv2 (SPDX: GPL-2.0-only), GPLv3 (SPDX: GPL-3.0-only),
// or any future license endorsed by Mnemosyne LLC.
// License text can be found in the licenses/ folder.

#pragma once

#include <algorithm>
#include <bitset>
#include <initializer_list>
#include <memory>
#include <vector>

#include <glibmm.h>
#include <glibmm/extraclassinit.h>
#include <gtkmm.h>

#include <libtransmission/transmission.h>

struct tr_torrent;

class Torrent
    : public Glib::ExtraClassInit
    , public Glib::Object
{
public:
    class Columns : public Gtk::TreeModelColumnRecord
    {
    public:
        Columns();

        Gtk::TreeModelColumn<Torrent*> self;
        Gtk::TreeModelColumn<Glib::ustring> name_collated;
    };

    enum ChangeFlag
    {
        ACTIVE_PEER_COUNT,
        ACTIVE_PEERS_DOWN,
        ACTIVE_PEERS_UP,
        ACTIVE,
        ACTIVITY,
        ADDED_DATE,
        ERROR_CODE,
        ERROR_MESSAGE,
        ETA,
        FINISHED,
        HAS_METADATA,
        LONG_PROGRESS,
        LONG_STATUS,
        MIME_TYPE,
        NAME,
        PERCENT_COMPLETE,
        PERCENT_DONE,
        PRIORITY,
        QUEUE_POSITION,
        RATIO,
        RECHECK_PROGRESS,
        SEED_RATIO_PERCENT_DONE,
        SPEED_DOWN,
        SPEED_UP,
        STALLED,
        TOTAL_SIZE,
        TRACKERS,

        CHANGE_FLAG_COUNT
    };

    using ChangeFlags = std::bitset<CHANGE_FLAG_COUNT>;

public:
    int get_active_peer_count() const;
    int get_active_peers_down() const;
    int get_active_peers_up() const;
    bool get_active() const;
    tr_torrent_activity get_activity() const;
    time_t get_added_date() const;
    int get_error_code() const;
    Glib::ustring const& get_error_message() const;
    time_t get_eta() const;
    bool get_finished() const;
    tr_torrent_id_t get_id() const;
    Glib::ustring const& get_name_collated() const;
    Glib::ustring get_name() const;
    float get_percent_complete() const;
    float get_percent_done() const;
    tr_priority_t get_priority() const;
    size_t get_queue_position() const;
    float get_ratio() const;
    float get_recheck_progress() const;
    float get_seed_ratio_percent_done() const;
    float get_speed_down() const;
    float get_speed_up() const;
    tr_torrent& get_torrent() const;
    uint64_t get_total_size() const;
    unsigned int get_trackers() const;

    Glib::RefPtr<Gio::Icon> get_icon() const;
    Glib::ustring get_short_status() const;
    Glib::ustring get_long_progress() const;
    Glib::ustring get_long_status() const;
    bool get_sensitive() const;
    std::vector<Glib::ustring> get_css_classes() const;

    ChangeFlags update();

    static Glib::RefPtr<Torrent> create(tr_torrent& torrent);

    static Columns const& get_columns();

    static int get_item_id(Glib::RefPtr<Glib::ObjectBase const> const& item);
    static void get_item_value(Glib::RefPtr<Glib::ObjectBase const> const& item, int column, Glib::ValueBase& value);

    static auto make_change_flags(std::initializer_list<ChangeFlag> flags)
    {
        auto result = ChangeFlags();
        std::for_each(flags.begin(), flags.end(), [&result](auto flag) { result.set(flag); });
        return result;
    }

private:
    Torrent();
    explicit Torrent(tr_torrent& torrent);

private:
    struct Impl;
    std::unique_ptr<Impl> const impl_;
};
