// This file Copyright © 2022 Mnemosyne LLC.
// It may be used under GPLv2 (SPDX: GPL-2.0-only), GPLv3 (SPDX: GPL-3.0-only),
// or any future license endorsed by Mnemosyne LLC.
// License text can be found in the licenses/ folder.

#include <algorithm>

#include <libtransmission/transmission.h>

#include "Torrent.h"
#include "TorrentSorter.h"

using namespace std::string_view_literals;

namespace
{

constexpr bool is_valid_eta(time_t value)
{
    return value != TR_ETA_NOT_AVAIL && value != TR_ETA_UNKNOWN;
}

template<typename T>
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
constexpr int compare_generic(T const& lhs, T const& rhs)
{
    if (lhs < rhs)
    {
        return -1;
    }

    if (lhs > rhs)
    {
        return 1;
    }

    return 0;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
constexpr int compare_eta(time_t lhs, time_t rhs)
{
    bool const lhs_valid = is_valid_eta(lhs);
    bool const rhs_valid = is_valid_eta(rhs);

    if (!lhs_valid && !rhs_valid)
    {
        return 0;
    }

    if (!lhs_valid)
    {
        return -1;
    }

    if (!rhs_valid)
    {
        return 1;
    }

    return -compare_generic(lhs, rhs);
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
constexpr int compare_ratio(double lhs, double rhs)
{
    if (static_cast<int>(lhs) == TR_RATIO_INF && static_cast<int>(rhs) == TR_RATIO_INF)
    {
        return 0;
    }

    if (static_cast<int>(lhs) == TR_RATIO_INF)
    {
        return 1;
    }

    if (static_cast<int>(rhs) == TR_RATIO_INF)
    {
        return -1;
    }

    return compare_generic(lhs, rhs);
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
int compare_by_name(Torrent const& lhs, Torrent const& rhs)
{
    return lhs.get_name_collated().compare(rhs.get_name_collated());
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
int compare_by_queue(Torrent const& lhs, Torrent const& rhs)
{
    return compare_generic(lhs.get_queue_position(), rhs.get_queue_position());
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
int compare_by_ratio(Torrent const& lhs, Torrent const& rhs)
{
    auto result = -compare_ratio(lhs.get_ratio(), rhs.get_ratio()); // default descending

    if (result == 0)
    {
        result = compare_by_queue(lhs, rhs);
    }

    return result;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
int compare_by_activity(Torrent const& lhs, Torrent const& rhs)
{
    auto result = -compare_generic(
        lhs.get_speed_up() + lhs.get_speed_down(),
        rhs.get_speed_up() + rhs.get_speed_down()); // default descending

    if (result == 0)
    {
        result = -compare_generic(lhs.get_active_peer_count(), rhs.get_active_peer_count()); // default descending
    }

    if (result == 0)
    {
        result = compare_by_queue(lhs, rhs);
    }

    return result;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
int compare_by_age(Torrent const& lhs, Torrent const& rhs)
{
    auto result = -compare_generic(lhs.get_added_date(), rhs.get_added_date()); // default descending

    if (result == 0)
    {
        result = compare_by_name(lhs, rhs);
    }

    return result;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
int compare_by_size(Torrent const& lhs, Torrent const& rhs)
{
    auto result = -compare_generic(lhs.get_total_size(), rhs.get_total_size()); // default descending

    if (result == 0)
    {
        result = compare_by_name(lhs, rhs);
    }

    return result;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
int compare_by_progress(Torrent const& lhs, Torrent const& rhs)
{
    auto result = -compare_generic(lhs.get_percent_complete(), rhs.get_percent_complete()); // default descending

    if (result == 0)
    {
        result = -compare_generic(lhs.get_seed_ratio_percent_done(), rhs.get_seed_ratio_percent_done()); // default descending
    }

    if (result == 0)
    {
        result = compare_by_ratio(lhs, rhs);
    }

    return result;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
int compare_by_eta(Torrent const& lhs, Torrent const& rhs)
{
    auto result = compare_eta(lhs.get_eta(), rhs.get_eta());

    if (result == 0)
    {
        result = compare_by_name(lhs, rhs);
    }

    return result;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
int compare_by_state(Torrent const& lhs, Torrent const& rhs)
{
    auto result = -compare_generic(lhs.get_activity(), rhs.get_activity());

    if (result == 0)
    {
        result = compare_by_queue(lhs, rhs);
    }

    return result;
}

} // namespace

void TorrentSorter::set_mode(std::string_view mode)
{
    static auto const compare_funcs = std::map<std::string_view, CompareFunc>({
        { "sort-by-activity"sv, &compare_by_activity },
        { "sort-by-age"sv, &compare_by_age },
        { "sort-by-name"sv, &compare_by_name },
        { "sort-by-progress"sv, &compare_by_progress },
        { "sort-by-queue"sv, &compare_by_queue },
        { "sort-by-ratio"sv, &compare_by_ratio },
        { "sort-by-size"sv, &compare_by_size },
        { "sort-by-state"sv, &compare_by_state },
        { "sort-by-time-left"sv, &compare_by_eta },
    });

    auto compare_func = &compare_by_name;
    if (auto const compare_func_it = compare_funcs.find(mode); compare_func_it != compare_funcs.end())
    {
        compare_func = compare_func_it->second;
    }

    if (compare_func_ == compare_func)
    {
        return;
    }

    compare_func_ = compare_func;

    changed(Change::DIFFERENT);
}

void TorrentSorter::set_reversed(bool is_reversed)
{
    if (is_reversed_ == is_reversed)
    {
        return;
    }

    is_reversed_ = is_reversed;

    changed(Change::INVERTED);
}

int TorrentSorter::compare(Torrent const& lhs, Torrent const& rhs) const
{
    return compare_func_ != nullptr ? std::clamp(compare_func_(lhs, rhs), -1, 1) * (is_reversed_ ? -1 : 1) : 0;
}

void TorrentSorter::update(Torrent::ChangeFlags flags)
{
    using ChangeFlag = Torrent::ChangeFlag;

    static auto const compare_flags = std::map<CompareFunc, Torrent::ChangeFlags>({
        { &compare_by_activity,
          Torrent::make_change_flags({
              ChangeFlag::ACTIVE_PEER_COUNT,
              ChangeFlag::QUEUE_POSITION,
              ChangeFlag::SPEED_DOWN,
              ChangeFlag::SPEED_UP,
          }) },
        { &compare_by_age, Torrent::make_change_flags({ ChangeFlag::ADDED_DATE, ChangeFlag::NAME }) },
        { &compare_by_eta, Torrent::make_change_flags({ ChangeFlag::ETA, ChangeFlag::NAME }) },
        { &compare_by_name, Torrent::make_change_flags({ ChangeFlag::NAME }) },
        { &compare_by_progress,
          Torrent::make_change_flags({
              ChangeFlag::PERCENT_COMPLETE,
              ChangeFlag::QUEUE_POSITION,
              ChangeFlag::RATIO,
              ChangeFlag::SEED_RATIO_PERCENT_DONE,
          }) },
        { &compare_by_queue, Torrent::make_change_flags({ ChangeFlag::QUEUE_POSITION }) },
        { &compare_by_ratio, Torrent::make_change_flags({ ChangeFlag::QUEUE_POSITION, ChangeFlag::RATIO }) },
        { &compare_by_size, Torrent::make_change_flags({ ChangeFlag::NAME, ChangeFlag::TOTAL_SIZE }) },
        { &compare_by_state, Torrent::make_change_flags({ ChangeFlag::ACTIVITY, ChangeFlag::QUEUE_POSITION }) },
    });

    if (auto const compare_flags_it = compare_flags.find(compare_func_);
        compare_flags_it != compare_flags.end() && (flags & compare_flags_it->second).any())
    {
        changed(Change::DIFFERENT);
    }
}

#if !GTKMM_CHECK_VERSION(4, 0, 0)

sigc::signal<void()>& TorrentSorter::signal_changed()
{
    return signal_changed_;
}

#endif

Glib::RefPtr<TorrentSorter> TorrentSorter::create()
{
    return Glib::make_refptr_for_instance(new TorrentSorter());
}

#if GTKMM_CHECK_VERSION(4, 0, 0)

Gtk::Ordering TorrentSorter::compare_vfunc(gpointer lhs, gpointer rhs)
{
    auto const& lhs_torrent = *dynamic_cast<Torrent const*>(Glib::wrap_auto(static_cast<GObject*>(lhs)));
    auto const& rhs_torrent = *dynamic_cast<Torrent const*>(Glib::wrap_auto(static_cast<GObject*>(rhs)));

    return Gtk::Ordering{ compare(lhs_torrent, rhs_torrent) };
}

Gtk::Sorter::Order TorrentSorter::get_order_vfunc()
{
    return Gtk::Sorter::Order::PARTIAL;
}

#else

void TorrentSorter::changed(Change /*change*/)
{
    signal_changed_.emit();
}

#endif

TorrentSorter::TorrentSorter()
    : Glib::ObjectBase(typeid(TorrentSorter))
{
}
