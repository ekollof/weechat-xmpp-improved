// This Source Code Form is subject to the terms of the Mozilla Public
// License, version 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <strophe.h>

namespace xmpp { namespace xep0054 {

    // XEP-0054: vcard-temp
    inline xmpp_stanza_t *vcard_request(xmpp_ctx_t *context, const char *to)
    {
        xmpp_stanza_t *iq = xmpp_iq_new(context, "get", xmpp_uuid_gen(context));
        if (to)
            xmpp_stanza_set_to(iq, to);
        
        xmpp_stanza_t *vcard = xmpp_stanza_new(context);
        xmpp_stanza_set_name(vcard, "vCard");
        xmpp_stanza_set_ns(vcard, "vcard-temp");
        
        xmpp_stanza_add_child(iq, vcard);
        xmpp_stanza_release(vcard);
        
        return iq;
    }

} }
