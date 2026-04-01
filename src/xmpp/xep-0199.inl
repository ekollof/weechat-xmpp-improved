// This Source Code Form is subject to the terms of the Mozilla Public
// License, version 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "node.hh"
#pragma GCC visibility push(default)
#include "ns.hh"
#pragma GCC visibility pop

namespace stanza {

    /* XMPP Ping (XEP-0199) */
    struct xep0199 {
        // <ping xmlns='urn:xmpp:ping'/>
        struct ping : virtual public spec {
            ping() : spec("ping") {
                xmlns<urn::xmpp::ping>();
            }
        };

        // stanza::iq mixin
        struct iq : virtual public spec {
            iq() : spec("iq") {}

            iq& xep0199() { return *this; }

            iq& ping() {
                xep0199::ping p;
                child(p);
                return *this;
            }
        };
    };

}
