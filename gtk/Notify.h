// This file Copyright © 2008-2022 Mnemosyne LLC.
// It may be used under GPLv2 (SPDX: GPL-2.0-only), GPLv3 (SPDX: GPL-3.0-only),
// or any future license endorsed by Mnemosyne LLC.
// License text can be found in the licenses/ folder.

#pragma once

#include <glibmm.h>

#include <libtransmission/transmission.h>

class Session;

void gtr_notify_init();

void gtr_notify_torrent_added(
    Glib::RefPtr<Session> const& core,
    Glib::RefPtr<Gio::Settings> const& settings,
    tr_torrent_id_t tor_id);

void gtr_notify_torrent_completed(
    Glib::RefPtr<Session> const& core,
    Glib::RefPtr<Gio::Settings> const& settings,
    tr_torrent_id_t tor_id);
