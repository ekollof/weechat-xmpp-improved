// This Source Code Form is subject to the terms of the Mozilla Public
// License, version 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "node.hh"
#pragma GCC visibility push(default)
#include "ns.hh"
#pragma GCC visibility pop

namespace stanza {

    /* Message Correct (XEP-0308) */
    struct xep0308 {

        // <replace xmlns='urn:xmpp:message-correct:0' id='...'/>
        struct replace : virtual public spec {
            replace(std::string_view replaced_id) : spec("replace") {
                xmlns<urn::xmpp::message_correct::_0>();
                attr("id", replaced_id);
            }
        };

        // stanza::message mixin
        struct message : virtual public spec {
            message() : spec("message") {}

            message& replace(xep0308::replace r) { child(r); return *this; }
        };
    };

}
