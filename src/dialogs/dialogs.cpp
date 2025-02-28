#include "dialogs.hpp"
#include "enter_datum_window.hpp"
#include "widgets/spin_button_dim.hpp"
#include <gtkmm.h>

namespace dune3d {

EnterDatumWindow *Dialogs::show_enter_datum_window(const std::string &label, double def)
{
    if (auto win = dynamic_cast<EnterDatumWindow *>(m_window_nonmodal)) {
        win->present();
        return win;
    }
    auto win = new EnterDatumWindow(m_parent, m_interface, label, def);
    m_window_nonmodal = win;
    win->signal_hide().connect(sigc::mem_fun(*this, &Dialogs::close_nonmodal));
    win->present();
    return win;
}

void Dialogs::close_nonmodal()
{
    delete m_window_nonmodal;
    m_window_nonmodal = nullptr;
}

} // namespace dune3d
