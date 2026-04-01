// This Source Code Form is subject to the terms of the Mozilla Public
// License, version 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "node.hh"
#pragma GCC visibility push(default)
#include "ns.hh"
#pragma GCC visibility pop

namespace stanza {

    /* Unique and Stable Stanza IDs (XEP-0359) */
    struct xep0359 {

        // <origin-id xmlns='urn:xmpp:sid:0' id='...'/>
        struct origin_id : virtual public spec {
            origin_id(std::string_view id_val) : spec("origin-id") {
                xmlns<urn::xmpp::sid::_0>();
                attr("id", id_val);
            }
        };

        // <stanza-id xmlns='urn:xmpp:sid:0' id='...' by='...'/>
        struct stanza_id : virtual public spec {
            stanza_id(std::string_view id_val, std::string_view by_val) : spec("stanza-id") {
                xmlns<urn::xmpp::sid::_0>();
                attr("id", id_val);
                attr("by", by_val);
            }
        };

        // stanza::message mixin
        struct message : virtual public spec {
            message() : spec("message") {}

            message& origin_id(xep0359::origin_id o) { child(o); return *this; }
            message& stanza_id(xep0359::stanza_id s) { child(s); return *this; }
        };
    };

}
