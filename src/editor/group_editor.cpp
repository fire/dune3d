#include "group_editor.hpp"
#include "document/group/group.hpp"
#include "document/group/group_extrude.hpp"
#include "document/group/group_lathe.hpp"
#include "document/group/group_fillet.hpp"
#include "document/group/group_reference.hpp"
#include "widgets/spin_button_dim.hpp"
#include "core/core.hpp"
#include "util/gtk_util.hpp"
#include <iostream>

namespace dune3d {

class GroupEditorSweep : public GroupEditor {
protected:
    using GroupEditor::GroupEditor;
    void add_operation_combo()
    {
        auto &group = get_group();
        auto items = Gtk::StringList::create();
        items->append("Union");
        items->append("Difference");

        m_operation_combo = Gtk::make_managed<Gtk::DropDown>(items);
        m_operation_combo->set_selected(static_cast<guint>(group.m_operation));
        m_operation_combo->property_selected().signal_changed().connect([this] {
            auto &group = get_group();
            group.m_operation = static_cast<GroupExtrude::Operation>(m_operation_combo->get_selected());
            m_core.get_current_document().set_group_update_solid_model_pending(group.m_uuid);
            m_signal_changed.emit();
        });
        grid_attach_label_and_widget(*this, "Operation", *m_operation_combo, m_top);
    }


    void reload() override
    {
        GroupEditor::reload();
        auto &group = get_group();
        m_operation_combo->set_selected(static_cast<guint>(group.m_operation));
    }

private:
    GroupSweep &get_group()
    {
        return m_core.get_current_document().get_group<GroupSweep>(m_group_uu);
    }

    Gtk::DropDown *m_operation_combo = nullptr;
};

class GroupEditorExtrude : public GroupEditorSweep {
public:
    GroupEditorExtrude(Core &core, const UUID &group_uu) : GroupEditorSweep(core, group_uu)
    {
        m_normal_switch = Gtk::make_managed<Gtk::Switch>();
        m_normal_switch->set_valign(Gtk::Align::CENTER);
        m_normal_switch->set_halign(Gtk::Align::START);
        auto &group = get_group();
        m_normal_switch->set_active(group.m_direction == GroupExtrude::Direction::NORMAL);
        grid_attach_label_and_widget(*this, "Along normal", *m_normal_switch, m_top);
        m_normal_switch->property_active().signal_changed().connect([this] {
            auto &group = get_group();
            if (m_normal_switch->get_active())
                group.m_direction = GroupExtrude::Direction::NORMAL;
            else
                group.m_direction = GroupExtrude::Direction::ARBITRARY;
            m_core.get_current_document().set_group_solve_pending(group.m_uuid);
            m_signal_changed.emit();
        });
        add_operation_combo();
        {
            auto items = Gtk::StringList::create();
            items->append("Single");
            items->append("Offset");
            items->append("Offset symmetric");

            m_mode_combo = Gtk::make_managed<Gtk::DropDown>(items);
            m_mode_combo->set_selected(static_cast<guint>(group.m_mode));
            m_mode_combo->property_selected().signal_changed().connect([this] {
                auto &group = get_group();
                group.m_mode = static_cast<GroupExtrude::Mode>(m_mode_combo->get_selected());
                m_core.get_current_document().set_group_generate_pending(group.m_uuid);
                m_signal_changed.emit();
            });
            grid_attach_label_and_widget(*this, "Mode", *m_mode_combo, m_top);
        }
    }

    void reload() override
    {
        GroupEditorSweep::reload();
        auto &group = get_group();
        m_normal_switch->set_active(group.m_direction == GroupExtrude::Direction::NORMAL);
        m_mode_combo->set_selected(static_cast<guint>(group.m_mode));
    }

private:
    GroupExtrude &get_group()
    {
        return m_core.get_current_document().get_group<GroupExtrude>(m_group_uu);
    }

    Gtk::Switch *m_normal_switch = nullptr;
    Gtk::DropDown *m_mode_combo = nullptr;
};

class GroupEditorLathe : public GroupEditorSweep {
public:
    GroupEditorLathe(Core &core, const UUID &group_uu) : GroupEditorSweep(core, group_uu)
    {
        add_operation_combo();
    }
};

class GroupEditorFillet : public GroupEditor {
public:
    GroupEditorFillet(Core &core, const UUID &group_uu) : GroupEditor(core, group_uu)
    {
        m_radius_sp = Gtk::make_managed<SpinButtonDim>();
        m_radius_sp->set_range(0, 1000);
        auto &group = get_group();
        m_radius_sp->set_value(group.m_radius);
        spinbutton_connect_activate_immediate(*m_radius_sp, [this] {
            get_group().m_radius = m_radius_sp->get_value();
            m_core.get_current_document().set_group_update_solid_model_pending(get_group().m_uuid);
            m_signal_changed.emit();
        });

        grid_attach_label_and_widget(*this, "Radius", *m_radius_sp, m_top);
    }

    void reload() override
    {
        GroupEditor::reload();
        auto &group = get_group();
        m_radius_sp->set_value(group.m_radius);
    }

private:
    GroupLocalOperation &get_group()
    {
        return m_core.get_current_document().get_group<GroupLocalOperation>(m_group_uu);
    }

    SpinButtonDim *m_radius_sp = nullptr;
};

class GroupEditorReference : public GroupEditor {
public:
    GroupEditorReference(Core &core, const UUID &group_uu) : GroupEditor(core, group_uu)
    {
        auto &group = get_group();

        m_switch_xy = Gtk::make_managed<Gtk::Switch>();
        m_switch_xy->set_halign(Gtk::Align::START);
        m_switch_xy->set_valign(Gtk::Align::CENTER);
        grid_attach_label_and_widget(*this, "XY", *m_switch_xy, m_top);
        m_switch_xy->set_active(group.m_show_xy);
        m_switch_xy->property_active().signal_changed().connect([this] {
            get_group().m_show_xy = m_switch_xy->get_active();
            get_group().generate(m_core.get_current_document());
            m_signal_changed.emit();
        });

        m_switch_yz = Gtk::make_managed<Gtk::Switch>();
        m_switch_yz->set_halign(Gtk::Align::START);
        m_switch_yz->set_valign(Gtk::Align::CENTER);
        grid_attach_label_and_widget(*this, "YZ", *m_switch_yz, m_top);
        m_switch_yz->set_active(group.m_show_yz);
        m_switch_yz->property_active().signal_changed().connect([this] {
            get_group().m_show_yz = m_switch_yz->get_active();
            get_group().generate(m_core.get_current_document());
            m_signal_changed.emit();
        });

        m_switch_zx = Gtk::make_managed<Gtk::Switch>();
        m_switch_zx->set_halign(Gtk::Align::START);
        m_switch_zx->set_valign(Gtk::Align::CENTER);
        grid_attach_label_and_widget(*this, "ZX", *m_switch_zx, m_top);
        m_switch_zx->set_active(group.m_show_zx);
        m_switch_zx->property_active().signal_changed().connect([this] {
            get_group().m_show_zx = m_switch_zx->get_active();
            get_group().generate(m_core.get_current_document());
            m_signal_changed.emit();
        });
    }

    void reload() override
    {
        GroupEditor::reload();
        auto &group = get_group();
        m_switch_xy->set_active(group.m_show_xy);
        m_switch_yz->set_active(group.m_show_yz);
        m_switch_zx->set_active(group.m_show_zx);
    }

private:
    GroupReference &get_group()
    {
        return m_core.get_current_document().get_group<GroupReference>(m_group_uu);
    }

    Gtk::Switch *m_switch_xy = nullptr;
    Gtk::Switch *m_switch_yz = nullptr;
    Gtk::Switch *m_switch_zx = nullptr;
};

GroupEditor::GroupEditor(Core &core, const UUID &group_uu) : m_core(core), m_group_uu(group_uu)
{
    set_valign(Gtk::Align::START);
    set_row_spacing(5);
    set_column_spacing(5);
    set_margin(5);
    set_row_homogeneous(true);
    m_type_label = Gtk::manage(new Gtk::Label);
    m_type_label->set_xalign(0);
    auto &group = m_core.get_current_document().get_group(group_uu);
    switch (group.get_type()) {
    case Group::Type::EXTRUDE:
        m_type_label->set_text("Extrude");
        break;
    case Group::Type::REFERENCE:
        m_type_label->set_text("Reference");
        break;
    case Group::Type::SKETCH:
        m_type_label->set_text("Sketch");
        break;
    case Group::Type::FILLET:
        m_type_label->set_text("Fillet");
        break;
    case Group::Type::CHAMFER:
        m_type_label->set_text("Chamfer");
        break;
    case Group::Type::LATHE:
        m_type_label->set_text("Lathe");
        break;
    }
    grid_attach_label_and_widget(*this, "Type", *m_type_label, m_top);

    m_name_entry = Gtk::make_managed<Gtk::Entry>();
    m_name_entry->set_text(group.m_name);
    grid_attach_label_and_widget(*this, "Name", *m_name_entry, m_top);
    m_name_entry->signal_activate().connect([this] {
        auto &group = m_core.get_current_document().get_group(m_group_uu);
        group.m_name = m_name_entry->get_text();
        m_signal_changed.emit();
    });

    m_body_entry = Gtk::make_managed<Gtk::Entry>();
    if (group.m_body.has_value())
        m_body_entry->set_text(group.m_body->m_name);
    m_body_cb = Gtk::make_managed<Gtk::CheckButton>();
    m_body_cb->set_active(group.m_body.has_value());
    if (&group == core.get_current_document().get_groups_sorted().front())
        m_body_cb->set_sensitive(false);
    m_body_entry->set_sensitive(group.m_body.has_value());
    {
        auto box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 3);
        box->append(*m_body_cb);
        box->append(*m_body_entry);
        grid_attach_label_and_widget(*this, "Body", *box, m_top);
    }
    m_body_entry->signal_activate().connect([this] {
        auto &group = m_core.get_current_document().get_group(m_group_uu);
        if (group.m_body.has_value())
            group.m_body->m_name = m_body_entry->get_text();
        m_signal_changed.emit();
    });
    m_body_cb->signal_toggled().connect([this] {
        auto &group = m_core.get_current_document().get_group(m_group_uu);
        if (m_body_cb->get_active())
            group.m_body.emplace();
        else
            group.m_body.reset();
        m_body_entry->set_sensitive(group.m_body.has_value());
        m_core.get_current_document().set_group_update_solid_model_pending(group.m_uuid);
        m_signal_changed.emit();
    });
}

GroupEditor *GroupEditor::create(Core &core, const UUID &group_uu)
{
    auto &group = core.get_current_document().get_group(group_uu);
    switch (group.get_type()) {
    case Group::Type::EXTRUDE:
        return Gtk::make_managed<GroupEditorExtrude>(core, group_uu);
    case Group::Type::LATHE:
        return Gtk::make_managed<GroupEditorLathe>(core, group_uu);
    case Group::Type::FILLET:
    case Group::Type::CHAMFER:
        return Gtk::make_managed<GroupEditorFillet>(core, group_uu);
    case Group::Type::REFERENCE:
        return Gtk::make_managed<GroupEditorReference>(core, group_uu);
    default:
        return Gtk::make_managed<GroupEditor>(core, group_uu);
    }
}

void GroupEditor::reload()
{
    auto &group = m_core.get_current_document().get_group(m_group_uu);
    m_name_entry->set_text(group.m_name);
    if (group.m_body.has_value())
        m_body_entry->set_text(group.m_body->m_name);
    m_body_cb->set_active(group.m_body.has_value());
}

GroupEditor::~GroupEditor()
{
}

} // namespace dune3d
