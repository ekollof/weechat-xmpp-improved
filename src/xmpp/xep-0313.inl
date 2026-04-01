// This Source Code Form is subject to the terms of the Mozilla Public
// License, version 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <optional>
#include <string>
#include <vector>
#include <chrono>

#include "node.hh"
#pragma GCC visibility push(default)
#include "ns.hh"
#pragma GCC visibility pop

namespace stanza {

    /* Message Archive Management (XEP-0313) */
    struct xep0313 {

        // A simple text-bearing sub-element used by x_filter
        struct text_field : virtual public spec {
            text_field(std::string_view var_name, std::string_view value)
                : spec("field") {
                attr("var", var_name);
                struct value_el : virtual public spec {
                    value_el(std::string_view s) : spec("value") { text(s); }
                };
                value_el v(value);
                child(v);
            }
        };

        struct hidden_field : virtual public spec {
            hidden_field(std::string_view var_name, std::string_view value)
                : spec("field") {
                attr("var", var_name);
                attr("type", "hidden");
                struct value_el : virtual public spec {
                    value_el(std::string_view s) : spec("value") { text(s); }
                };
                value_el v(value);
                child(v);
            }
        };

        // Convenience: a jabber:x:data form pre-configured for MAM filters
        struct x_filter : virtual public spec {
            x_filter() : spec("x") {
                xmlns<jabber::x::data>();
                attr("type", "submit");
                // Always include FORM_TYPE
                hidden_field ft("FORM_TYPE", "urn:xmpp:mam:2");
                child(ft);
            }

            x_filter& with(std::string_view jid_s) {
                text_field f("with", jid_s);
                child(f);
                return *this;
            }

            x_filter& start(std::string_view iso8601) {
                text_field f("start", iso8601);
                child(f);
                return *this;
            }

            x_filter& end(std::string_view iso8601) {
                text_field f("end", iso8601);
                child(f);
                return *this;
            }

            x_filter& before_id(std::string_view uid) {
                text_field f("before-id", uid);
                child(f);
                return *this;
            }

            x_filter& after_id(std::string_view uid) {
                text_field f("after-id", uid);
                child(f);
                return *this;
            }
        };

        // <query xmlns='urn:xmpp:mam:2' queryid='...' node='...'>
        struct query : virtual public spec {
            query() : spec("query") {
                xmlns<urn::xmpp::mam::_2>();
            }

            query& queryid(std::string_view s) { attr("queryid", s); return *this; }

            // Optional archive node (for MUC archives)
            query& node(std::string_view s) { attr("node", s); return *this; }

            // Embed a jabber:x:data form for filter parameters
            query& filter(x_filter f) { child(f); return *this; }

            // Embed RSM <set>
            query& rsm(spec& s) { child(s); return *this; }
        };

        // <prefs xmlns='urn:xmpp:mam:2' default='...'> … </prefs>
        // Used by XEP-0441 MAM preferences IQ.
        struct prefs : virtual public spec {
            prefs() : spec("prefs") {
                xmlns<urn::xmpp::mam::_2>();
            }

            prefs& default_(std::string_view val) { attr("default", val); return *this; }

            // A list element <always> or <never> with optional <jid> children.
            struct jid_list : virtual public spec {
                jid_list(std::string_view element_name) : spec(element_name) {}

                jid_list& jid(std::string_view s) {
                    struct jid_el : virtual public spec {
                        jid_el(std::string_view v) : spec("jid") { text(v); }
                    };
                    jid_el j(s);
                    child(j);
                    return *this;
                }
            };

            prefs& always(jid_list l = jid_list("always")) { child(l); return *this; }
            prefs& never_(jid_list l = jid_list("never"))  { child(l); return *this; }
        };

        // stanza::iq mixin
        struct iq : virtual public spec {
            iq() : spec("iq") {}

            iq& xep0313() { return *this; }

            iq& query(xep0313::query q = {}) { child(q); return *this; }

            iq& prefs(xep0313::prefs p = {}) { child(p); return *this; }
        };
    };

}

namespace xml {

    /* Message Archive Management parse layer (XEP-0313) */
    class xep0313 : virtual public node {
    public:
        // Wraps the <forwarded> envelope inside a MAM <result>
        struct forwarded {
            forwarded(node& n) {
                // <delay xmlns='urn:xmpp:delay' stamp='...'/>
                for (auto& ch : n.get_children<urn::xmpp::delay>("delay"))
                    delay_stamp = ch.get().get_attr("stamp");
                // <message ...> is the first non-delay child
                for (auto& ch : n.children) {
                    if (ch.name == "message") {
                        message = ch;
                        break;
                    }
                }
            }

            std::optional<std::string> delay_stamp;
            std::optional<node> message;
        };

        struct result {
            result(node& n) {
                id = n.get_attr("id");
                queryid = n.get_attr("queryid");
                for (auto& ch : n.get_children<urn::xmpp::forward::_0>("forwarded"))
                    this->fwd.emplace(ch.get());
            }

            std::optional<std::string> id;
            std::optional<std::string> queryid;
            std::optional<forwarded> fwd;
        };

        struct fin {
            fin(node& n) {
                complete = (n.get_attr("complete") == std::optional<std::string>{"true"});
                stable   = (n.get_attr("stable")   != std::optional<std::string>{"false"});
                // RSM <set>
                for (auto& ch : n.get_children<jabber_org::protocol::rsm>("set"))
                    rsm_set = ch;
            }

            bool complete = false;
            bool stable = true;
            std::optional<node> rsm_set;
        };

    private:
        std::optional<std::optional<result>> _result;
        std::optional<std::optional<fin>>    _fin;
    public:
        // Parses top-level <result xmlns='urn:xmpp:mam:2'> from a <message>
        std::optional<result>& mam_result() {
            if (!_result) {
                auto rs = get_children<urn::xmpp::mam::_2>("result");
                if (!rs.empty())
                    _result = result(rs.front().get());
                else
                    _result.emplace(std::nullopt);
            }
            return *_result;
        }

        // Parses <fin xmlns='urn:xmpp:mam:2'> from an <iq>
        std::optional<fin>& mam_fin() {
            if (!_fin) {
                auto fins = get_children<urn::xmpp::mam::_2>("fin");
                if (!fins.empty())
                    _fin = fin(fins.front().get());
                else
                    _fin.emplace(std::nullopt);
            }
            return *_fin;
        }
    };

}
