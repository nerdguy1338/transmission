// This file Copyright © 2022 Mnemosyne LLC.
// It may be used under GPLv2 (SPDX: GPL-2.0-only), GPLv3 (SPDX: GPL-3.0-only),
// or any future license endorsed by Mnemosyne LLC.
// License text can be found in the licenses/ folder.

#include <map>

#include "ListModelAdapter.h"

namespace
{

template<typename T>
int iter_get_stamp(T const& iter)
{
    return iter.gobj()->stamp;
}

template<typename T>
void iter_set_stamp(T& iter, int value)
{
    iter.gobj()->stamp = value;
}

template<typename T>
int iter_get_item_id(T const& iter)
{
    return GPOINTER_TO_INT(iter.gobj()->user_data);
}

template<typename T>
void iter_set_item_id(T& iter, int value)
{
    iter.gobj()->user_data = GINT_TO_POINTER(value);
}

template<typename T>
void iter_clear(T& iter)
{
    iter_set_stamp(iter, 0);
    iter_set_item_id(iter, 0);
}

} // namespace

ListModelAdapter::ListModelAdapter(
    Glib::RefPtr<Gio::ListModel> const& adaptee,
    Gtk::TreeModelColumnRecord const& columns,
    IdGetter id_getter,
    ValueGetter value_getter)
    : Glib::ObjectBase(typeid(ListModelAdapter))
    , adaptee_(adaptee)
    , columns_(columns)
    , id_getter_(std::move(id_getter))
    , value_getter_(std::move(value_getter))
{
    adaptee_->signal_items_changed().connect(sigc::mem_fun(*this, &ListModelAdapter::on_adaptee_items_changed));

    on_adaptee_items_changed(0, 0, adaptee_->get_n_items());
}

ListModelAdapter::TrTreeModelFlags ListModelAdapter::get_flags_vfunc() const
{
    return TR_GTK_TREE_MODEL_FLAGS(ITERS_PERSIST) | TR_GTK_TREE_MODEL_FLAGS(LIST_ONLY);
}

int ListModelAdapter::get_n_columns_vfunc() const
{
    return columns_.size();
}

GType ListModelAdapter::get_column_type_vfunc(int index) const
{
    return index >= 0 && static_cast<guint>(index) < columns_.size() ? columns_.types()[index] : G_TYPE_INVALID;
}

bool ListModelAdapter::iter_next_vfunc(iterator const& iter, iterator& iter_next) const
{
    iter_clear(iter_next);

    if (iter)
    {
        g_assert(iter_get_stamp(iter) == stamp_);
        auto const item_id = iter_get_item_id(iter);

        if (auto const item_it = std::find(item_ids_.begin(), item_ids_.end(), item_id); item_it != item_ids_.end())
        {
            if (auto const next_it = std::next(item_it); next_it != item_ids_.end())
            {
                iter_set_stamp(iter_next, stamp_);
                iter_set_item_id(iter_next, *next_it);
                return true;
            }
        }
    }

    return false;
}

bool ListModelAdapter::get_iter_vfunc(Path const& path, iterator& iter) const
{
    iter_clear(iter);

    if (path.size() != 1 || path.front() < 0 || static_cast<guint>(path.front()) >= item_ids_.size())
    {
        return false;
    }

    iter_set_stamp(iter, stamp_);
    iter_set_item_id(iter, item_ids_.at(path.front()));
    return true;
}

bool ListModelAdapter::iter_children_vfunc(iterator const& parent, iterator& iter) const
{
    iter_clear(iter);

    if (parent || item_ids_.empty())
    {
        return false;
    }

    iter_set_stamp(iter, stamp_);
    iter_set_item_id(iter, item_ids_.front());
    return true;
}

bool ListModelAdapter::iter_parent_vfunc(iterator const& /*child*/, iterator& iter) const
{
    iter_clear(iter);
    return false;
}

bool ListModelAdapter::iter_nth_root_child_vfunc(int position, iterator& iter) const
{
    iter_clear(iter);

    if (position < 0 || static_cast<guint>(position) >= item_ids_.size())
    {
        return false;
    }

    iter_set_stamp(iter, stamp_);
    iter_set_item_id(iter, item_ids_.at(position));
    return true;
}

bool ListModelAdapter::iter_has_child_vfunc(const_iterator const& /*iter*/) const
{
    return false;
}

int ListModelAdapter::iter_n_root_children_vfunc() const
{
    return item_ids_.size();
}

Gtk::TreeModel::Path ListModelAdapter::get_path_vfunc(const_iterator const& iter) const
{
    auto path = Path();

    if (iter)
    {
        g_assert(iter_get_stamp(iter) == stamp_);
        auto const item_id = iter_get_item_id(iter);

        if (auto const ids_it = std::find(item_ids_.begin(), item_ids_.end(), item_id); ids_it != item_ids_.end())
        {
            path.push_back(std::distance(item_ids_.begin(), ids_it));
        }
    }

    return path;
}

void ListModelAdapter::get_value_vfunc(const_iterator const& iter, int column, Glib::ValueBase& value) const
{
    g_assert(column >= 0);
    g_assert(static_cast<guint>(column) < columns_.size());

    value.init(columns_.types()[column]);

    if (!iter)
    {
        return;
    }

    auto const item_id = iter_get_item_id(iter);
    auto const item_it = std::find(item_ids_.begin(), item_ids_.end(), item_id);
    if (item_it == item_ids_.end())
    {
        return;
    }

    auto const item = adaptee_->get_object(std::distance(item_ids_.begin(), item_it));
    if (item == nullptr)
    {
        return;
    }

    value_getter_(item, column, value);
}

void ListModelAdapter::on_adaptee_items_changed(guint position, guint removed, guint added)
{
    for (auto i = 0U; i < removed; ++i)
    {
        auto const position_from_end = position + removed - i - 1;

        item_ids_.erase(std::next(item_ids_.begin(), position_from_end));

        auto path = Path();
        path.push_back(position_from_end);

        row_deleted(path);
    }

    for (auto i = 0U; i < added; ++i)
    {
        auto const item = adaptee_->get_object(position + i);
        auto const item_id = id_getter_(item);

        item_ids_.insert(std::next(item_ids_.begin(), position + i), item_id);

        auto path = Path();
        path.push_back(position + i);

        auto iter = iterator(this);
        iter_set_stamp(iter, stamp_);
        iter_set_item_id(iter, item_id);

        row_inserted(path, iter);

        auto const notify_callback = G_CALLBACK(&ListModelAdapter::on_adaptee_item_changed_callback);
        auto const notify_callback_ptr = reinterpret_cast<gpointer>(notify_callback);
        if (g_signal_handler_find(item->gobj(), G_SIGNAL_MATCH_FUNC, 0, 0, nullptr, notify_callback_ptr, nullptr) == 0)
        {
            g_signal_connect_after(item->gobj(), "notify", notify_callback, this);
        }
    }
}

void ListModelAdapter::on_adaptee_item_changed(int item_id)
{
    if (auto const ids_it = std::find(item_ids_.begin(), item_ids_.end(), item_id); ids_it != item_ids_.end())
    {
        auto path = Path();
        path.push_back(std::distance(item_ids_.begin(), ids_it));

        auto iter = iterator(this);
        iter_set_stamp(iter, stamp_);
        iter_set_item_id(iter, item_id);

        row_changed(path, iter);
    }
}

void ListModelAdapter::on_adaptee_item_changed_callback(GObject* object, GParamSpec* /*param_spec*/, ListModelAdapter* self)
{
    self->on_adaptee_item_changed(self->id_getter_(Glib::wrap(object, true)));
}
