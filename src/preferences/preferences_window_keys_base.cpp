#include "preferences_window_keys_base.hpp"
#include "preferences/preferences.hpp"
#include "util/gtk_util.hpp"
#include "util/util.hpp"
#include "util/changeable.hpp"
#include "nlohmann/json.hpp"
#include "core/tool_id.hpp"
#include "widgets/capture_dialog.hpp"
#include <iostream>
namespace dune3d {


GType KeySequencesPreferencesEditorBase::ActionItem::get_type()
{
    // Let's cache once the type does exist.
    static GType gtype;
    if (!gtype)
        gtype = g_type_from_name("gtkmm__CustomObject_KeysActionItem");
    return gtype;
}

KeySequencesPreferencesEditorBase::ActionItem::ActionItem() : Glib::ObjectBase("KeysActionItem"), m_keys(*this, "keys")
{
    m_store = Gio::ListStore<ActionItem>::create();
}


Glib::RefPtr<Gio::ListModel> KeySequencesPreferencesEditorBase::create_model(const Glib::RefPtr<Glib::ObjectBase> &item)
{
    if (auto col = std::dynamic_pointer_cast<ActionItem>(item)) {
        if (col->m_store->get_n_items())
            return col->m_store;
    }
    return nullptr;
}

KeySequencesPreferencesEditorBase::KeySequencesPreferencesEditorBase(BaseObjectType *cobject,
                                                                     const Glib::RefPtr<Gtk::Builder> &x,
                                                                     Preferences &prefs)
    : Gtk::Box(cobject), m_preferences(prefs)
{
    m_store = decltype(m_store)::element_type::create();

    m_model =
            Gtk::TreeListModel::create(m_store, sigc::mem_fun(*this, &KeySequencesPreferencesEditorBase::create_model),
                                       /* passthrough */ false, /* autoexpand */ true);
    m_selection_model = Gtk::SingleSelection::create(m_model);


    m_action_editors = x->get_widget<Gtk::FlowBox>("action_editors");

    m_view = x->get_widget<Gtk::ColumnView>("key_sequences_view");
    m_view->set_model(m_selection_model);
    {
        auto factory = Gtk::SignalListItemFactory::create();
        factory->signal_setup().connect([&](const Glib::RefPtr<Gtk::ListItem> &li) {
            auto expander = Gtk::make_managed<Gtk::TreeExpander>();
            auto label = Gtk::make_managed<Gtk::Label>();
            label->set_halign(Gtk::Align::START);
            expander->set_child(*label);
            li->set_child(*expander);
        });
        factory->signal_bind().connect([&](const Glib::RefPtr<Gtk::ListItem> &li) {
            auto row = std::dynamic_pointer_cast<Gtk::TreeListRow>(li->get_item());
            if (!row)
                return;
            auto col = std::dynamic_pointer_cast<ActionItem>(row->get_item());
            if (!col)
                return;
            auto expander = dynamic_cast<Gtk::TreeExpander *>(li->get_child());
            if (!expander)
                return;
            expander->set_list_row(row);
            auto label = dynamic_cast<Gtk::Label *>(expander->get_child());
            if (!label)
                return;
            label->set_text(Glib::ustring::format(col->m_name));
        });
        auto col = Gtk::ColumnViewColumn::create("Action", factory);
        col->set_expand(true);
        m_view->append_column(col);
    }
    {
        auto factory = Gtk::SignalListItemFactory::create();
        factory->signal_setup().connect([&](const Glib::RefPtr<Gtk::ListItem> &li) {
            auto label = Gtk::make_managed<Gtk::Label>();
            label->set_xalign(0);
            li->set_child(*label);

            auto row_expr =
                    Gtk::PropertyExpression<Glib::RefPtr<Glib::ObjectBase>>::create(Gtk::ListItem::get_type(), "item");
            auto tree_expr = Gtk::PropertyExpression<Glib::RefPtr<Glib::ObjectBase>>::create(
                    Gtk::TreeListRow::get_type(), row_expr, "item");

            auto keys_expr = Gtk::PropertyExpression<Glib::ustring>::create(ActionItem::get_type(), tree_expr, "keys");
            keys_expr->bind(label->property_label(), li);
        });

        auto col = Gtk::ColumnViewColumn::create("Keys", factory);
        m_view->append_column(col);
    }

    m_selection_model->signal_selection_changed().connect([this](guint, guint) { update_action_editors(); });
    update_action_editors();
}

void KeySequencesPreferencesEditorBase::update_keys()
{
    for (size_t i = 0; i < m_store->get_n_items(); i++) {
        auto it_g = m_store->get_item(i);
        for (size_t j = 0; j < it_g->m_store->get_n_items(); j++) {
            auto it = it_g->m_store->get_item(j);
            update_item(*it);
        }
    }
}

class KeysBox : public Gtk::Box {
public:
    KeysBox(KeySequence &k) : Gtk::Box(Gtk::Orientation::HORIZONTAL, 5), keys(k)
    {
        set_margin(5);
    }
    KeySequence &keys;
};

void KeySequencesPreferencesEditorBase::update_action_editors()
{
    while (auto child = m_action_editors->get_first_child())
        m_action_editors->remove(*child);

    auto row = std::dynamic_pointer_cast<Gtk::TreeListRow>(m_selection_model->get_selected_item());
    if (!row)
        return;
    auto col = std::dynamic_pointer_cast<ActionItem>(row->get_item());
    if (!col)
        return;

    update_action_editors(*col);
}

/*
KeySequencesPreferencesEditorBase *KeySequencesPreferencesEditorBase::create(Preferences &prefs)
{
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/org/dune3d/dune3d/preferences/key_sequences.ui");
    auto w = Gtk::Builder::get_widget_derived<KeySequencesPreferencesEditorBase>(x, "key_sequences_box", prefs);
    w->reference();
    return w;
}
*/
} // namespace dune3d
