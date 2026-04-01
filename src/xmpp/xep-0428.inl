// This Source Code Form is subject to the terms of the Mozilla Public
// License, version 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "node.hh"
#pragma GCC visibility push(default)
#include "ns.hh"
#pragma GCC visibility pop

namespace stanza {

    /* Fallback Indication (XEP-0428) */
    struct xep0428 {

        // <fallback xmlns='urn:xmpp:fallback:0' for='...'/>
        struct fallback : virtual public spec {
            fallback(std::string_view for_ns) : spec("fallback") {
                xmlns<urn::xmpp::fallback::_0>();
                attr("for", for_ns);
            }
        };

        // stanza::message mixin
        struct message : virtual public spec {
            message() : spec("message") {}

            message& fallback(xep0428::fallback f) { child(f); return *this; }
        };
    };

}
