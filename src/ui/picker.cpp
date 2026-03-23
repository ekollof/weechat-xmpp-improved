// This Source Code Form is subject to the terms of the Mozilla Public
// License, version 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <weechat/weechat-plugin.h>
#include "picker.hh"
#include "../plugin.hh"  // for weechat_plugin global

namespace weechat::ui {

// Static active picker pointer — there is at most one picker open at a time.
picker_base *picker_base::s_active = nullptr;

// ---------------------------------------------------------------------------
// /xmpp-picker-nav up|down|enter|quit
//
// This is a WeeChat command registered by command__init() in command.cpp.
// The per-buffer key bindings on the picker buffer dispatch here.
// ---------------------------------------------------------------------------
int picker_nav_cb(const void * /*pointer*/, void * /*data*/,
                  struct t_gui_buffer * /*buffer*/,
                  int argc, char **argv, char ** /*argv_eol*/)
{
    if (argc < 2 || !picker_base::s_active)
        return WEECHAT_RC_OK_EAT;

    const char *dir = argv[1];

    if (strcmp(dir, "up") == 0)
        picker_base::s_active->navigate(-1);
    else if (strcmp(dir, "down") == 0)
        picker_base::s_active->navigate(+1);
    else if (strcmp(dir, "enter") == 0)
        picker_base::s_active->confirm();
    else if (strcmp(dir, "quit") == 0)
        picker_base::s_active->cancel();

    return WEECHAT_RC_OK_EAT;
}

} // namespace weechat::ui
