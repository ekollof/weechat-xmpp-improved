// This Source Code Form is subject to the terms of the Mozilla Public
// License, version 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "node.hh"
#pragma GCC visibility push(default)
#include "ns.hh"
#pragma GCC visibility pop

namespace stanza {

    /* Stanza Content Encryption Fastening / Apply-To (XEP-0422) */
    struct xep0422 {

        // <apply-to xmlns='urn:xmpp:fasten:0' id='...'>…</apply-to>
        struct apply_to : virtual public spec {
            apply_to(std::string_view target_id) : spec("apply-to") {
                xmlns<urn::xmpp::fasten::_0>();
                attr("id", target_id);
            }

            apply_to& child_el(spec& ch) { child(ch); return *this; }
        };

        // stanza::message mixin
        struct message : virtual public spec {
            message() : spec("message") {}

            message& apply_to(xep0422::apply_to a) { child(a); return *this; }
        };
    };

}
