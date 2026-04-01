// This Source Code Form is subject to the terms of the Mozilla Public
// License, version 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <string>
#include <vector>
#include <optional>

#include "node.hh"
#pragma GCC visibility push(default)
#include "ns.hh"
#pragma GCC visibility pop

namespace stanza {

    /* Blocking Command (XEP-0191) */
    struct xep0191 {
        // <item jid='...'/>
        struct item : virtual public spec {
            item(std::string_view jid_s) : spec("item") {
                attr("jid", jid_s);
            }
        };

        // <block xmlns='urn:xmpp:blocking'>
        //   <item jid='user@example.org'/>
        // </block>
        struct block : virtual public spec {
            block() : spec("block") {
                xmlns<urn::xmpp::blocking>();
            }

            block& item(std::string_view jid_s) {
                xep0191::item it(jid_s);
                child(it);
                return *this;
            }
        };

        // <unblock xmlns='urn:xmpp:blocking'>
        struct unblock : virtual public spec {
            unblock() : spec("unblock") {
                xmlns<urn::xmpp::blocking>();
            }

            // When called with no items: unblock all
            unblock& item(std::string_view jid_s) {
                xep0191::item it(jid_s);
                child(it);
                return *this;
            }
        };

        // <blocklist xmlns='urn:xmpp:blocking'/> (get request)
        struct blocklist : virtual public spec {
            blocklist() : spec("blocklist") {
                xmlns<urn::xmpp::blocking>();
            }
        };

        // stanza::iq mixin
        struct iq : virtual public spec {
            iq() : spec("iq") {}

            iq& xep0191() { return *this; }

            iq& block(xep0191::block b) { child(b); return *this; }
            iq& unblock(xep0191::unblock u) { child(u); return *this; }
            iq& blocklist(xep0191::blocklist l = {}) { child(l); return *this; }
        };
    };

}

namespace xml {

    /* Blocking Command parse layer (XEP-0191) */
    class xep0191 : virtual public node {
    public:
        struct blocklist {
            blocklist(node& n) {
                for (auto& ch : n.get_children("item"))
                    if (auto jid_s = ch.get().get_attr("jid"))
                        items.push_back(*jid_s);
            }

            std::vector<std::string> items;
        };

    private:
        std::optional<std::optional<blocklist>> _blocklist;
    public:
        std::optional<blocklist>& blocking_list() {
            if (!_blocklist) {
                auto bl = get_children<urn::xmpp::blocking>("blocklist");
                if (!bl.empty())
                    _blocklist = blocklist(bl.front().get());
                else
                    _blocklist.emplace(std::nullopt);
            }
            return *_blocklist;
        }
    };

}
