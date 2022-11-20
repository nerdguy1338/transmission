// This file Copyright © 2022 Mnemosyne LLC.
// It may be used under GPLv2 (SPDX: GPL-2.0-only), GPLv3 (SPDX: GPL-3.0-only),
// or any future license endorsed by Mnemosyne LLC.
// License text can be found in the licenses/ folder.

#pragma once

#include <vector>

#include <gtkmm.h>

#include "Utils.h"

class ListModelAdapter
    : public Gtk::TreeModel
    , public Glib::Object
{
    using IdGetter = std::function<int(Glib::RefPtr<Glib::ObjectBase const> const&)>;
    using ValueGetter = std::function<void(Glib::RefPtr<Glib::ObjectBase const> const&, int, Glib::ValueBase&)>;

    using TrTreeModelFlags = IF_GTKMM4(Gtk::TreeModel::Flags, Gtk::TreeModelFlags);

public:
    template<typename T>
    static Glib::RefPtr<ListModelAdapter> create(Glib::RefPtr<Gio::ListModel> const& adaptee)
    {
        return Glib::make_refptr_for_instance(
            new ListModelAdapter(adaptee, T::get_columns(), &T::get_item_id, &T::get_item_value));
    }

protected:
    // Gtk::TreeModel
    TrTreeModelFlags get_flags_vfunc() const override;
    int get_n_columns_vfunc() const override;
    GType get_column_type_vfunc(int index) const override;
    bool iter_next_vfunc(iterator const& iter, iterator& iter_next) const override;
    bool get_iter_vfunc(Path const& path, iterator& iter) const override;
    bool iter_children_vfunc(iterator const& parent, iterator& iter) const override;
    bool iter_parent_vfunc(iterator const& child, iterator& iter) const override;
    bool iter_nth_root_child_vfunc(int position, iterator& iter) const override;
    bool iter_has_child_vfunc(const_iterator const& iter) const override;
    int iter_n_root_children_vfunc() const override;
    TreeModel::Path get_path_vfunc(const_iterator const& iter) const override;
    void get_value_vfunc(const_iterator const& iter, int column, Glib::ValueBase& value) const override;

private:
    ListModelAdapter(
        Glib::RefPtr<Gio::ListModel> const& adaptee,
        Gtk::TreeModelColumnRecord const& columns,
        IdGetter id_getter,
        ValueGetter value_getter);

    void on_adaptee_items_changed(guint position, guint removed, guint added);
    void on_adaptee_item_changed(int item_id);

    static void on_adaptee_item_changed_callback(GObject* object, GParamSpec* param_spec, ListModelAdapter* self);

private:
    Glib::RefPtr<Gio::ListModel> const adaptee_;
    Gtk::TreeModelColumnRecord const& columns_;
    IdGetter const id_getter_;
    ValueGetter const value_getter_;

    int const stamp_ = 1;
    std::vector<int> item_ids_;
};
