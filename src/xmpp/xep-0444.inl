// This Source Code Form is subject to the terms of the Mozilla Public
// License, version 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "node.hh"
#pragma GCC visibility push(default)
#include "ns.hh"
#pragma GCC visibility pop

namespace stanza {

    /* Reactions (XEP-0444) */
    struct xep0444 {

        // <reactions xmlns='urn:xmpp:reactions:0' id='target-msg-id'>
        //   <reaction>emoji</reaction>…
        // </reactions>
        struct reactions : virtual public spec {
            reactions(std::string_view target_id) : spec("reactions") {
                xmlns<urn::xmpp::reactions::_0>();
                attr("id", target_id);
            }

            reactions& reaction(std::string_view emoji) {
                struct reaction_el : virtual public spec {
                    reaction_el(std::string_view e) : spec("reaction") { text(e); }
                };
                reaction_el r(emoji);
                child(r);
                return *this;
            }
        };

        // stanza::message mixin
        struct message : virtual public spec {
            message() : spec("message") {}

            message& reactions(xep0444::reactions r) { child(r); return *this; }
        };
    };

}
