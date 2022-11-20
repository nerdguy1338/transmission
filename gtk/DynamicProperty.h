// This file Copyright © 2022 Mnemosyne LLC.
// It may be used under GPLv2 (SPDX: GPL-2.0-only), GPLv3 (SPDX: GPL-3.0-only),
// or any future license endorsed by Mnemosyne LLC.
// License text can be found in the licenses/ folder.

#pragma once

#include <glibmm.h>

#include "Utils.h"

template<typename ObjectT, guint IndexN, typename ValueT>
struct PropertyTraits
{
    using ObjectType = ObjectT;
    using ValueType = ValueT;

    using GetterFunc = ValueT (ObjectT::*)() const;

    static guint constexpr Index = IndexN;
};

template<typename TraitsT>
class DynamicProperty
{
    using ObjectType = typename TraitsT::ObjectType;
    using ValueType = typename TraitsT::ValueType;

    using GetterFunc = typename TraitsT::GetterFunc;

    static guint constexpr Index = TraitsT::Index;

public:
    constexpr DynamicProperty(char const* name, char const* nick, char const* blurb, GetterFunc getter) noexcept
        : name_(name)
        , nick_(nick)
        , blurb_(blurb)
        , getter_(getter)
    {
    }

    void install(GObjectClass* cls)
    {
        g_assert(param_spec_ == nullptr);
        param_spec_ = create_param_spec();
        g_object_class_install_property(cls, Index, param_spec_);
    }

    void get_value(ObjectType const& object, Glib::ValueBase& value) const
    {
        g_assert(param_spec_ != nullptr);
        static_cast<Glib::Value<ValueType>&>(value).set((object.*getter_)());
    }

    void notify_changed(ObjectType& object) const
    {
        g_assert(param_spec_ != nullptr);
        g_object_notify_by_pspec(object.gobj(), param_spec_);
    }

private:
    GParamSpec* create_param_spec() const
    {
        Glib::Value<ValueType> dummy_value;
        dummy_value.init(decltype(dummy_value)::value_type());

        return dummy_value.create_param_spec(name_, nick_, blurb_, TR_GLIB_PARAM_FLAGS(READABLE));
    }

private:
    char const* const name_;
    char const* const nick_;
    char const* const blurb_;
    GetterFunc const getter_;

    GParamSpec* param_spec_ = nullptr;
};
