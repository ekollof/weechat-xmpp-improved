// This Source Code Form is subject to the terms of the Mozilla Public
// License, version 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "node.hh"
#pragma GCC visibility push(default)
#include "ns.hh"
#pragma GCC visibility pop

namespace stanza {

    /* Replies to Messages (XEP-0461) */
    struct xep0461 {

        // <reply xmlns='urn:xmpp:reply:0' id='...' to='...'/>
        struct reply : virtual public spec {
            reply(std::string_view target_id, std::string_view to_jid) : spec("reply") {
                xmlns<urn::xmpp::reply::_0>();
                attr("id", target_id);
                attr("to", to_jid);
            }
        };

        // stanza::message mixin
        struct message : virtual public spec {
            message() : spec("message") {}

            message& reply(xep0461::reply r) { child(r); return *this; }
        };
    };

}
