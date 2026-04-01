// This Source Code Form is subject to the terms of the Mozilla Public
// License, version 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "node.hh"
#pragma GCC visibility push(default)
#include "ns.hh"
#pragma GCC visibility pop

namespace stanza {

    /* Message Retraction (XEP-0424) */
    struct xep0424 {

        // <retract xmlns='urn:xmpp:message-retract:1' id='...'/>
        struct retract : virtual public spec {
            retract(std::string_view target_id) : spec("retract") {
                xmlns<urn::xmpp::message_retract::_1>();
                attr("id", target_id);
            }
        };

        // stanza::message mixin
        struct message : virtual public spec {
            message() : spec("message") {}

            message& retract(xep0424::retract r) { child(r); return *this; }
        };
    };

}
