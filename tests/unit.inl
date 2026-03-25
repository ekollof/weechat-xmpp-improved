// This Source Code Form is subject to the terms of the Mozilla Public
// License, version 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// Unit tests for pure and near-pure plugin functions.
//
// Calls the real symbols from xmpp.cov.so (coverage-instrumented plugin)
// so that gcovr reports non-zero coverage for the exercised source files.
//
// Functions under test:
//   • char_cmp, unescape                   → src/util.cpp
//   • message__htmldecode                  → src/message.cpp
//   • get_name, get_attribute, get_text    → src/xmpp/node.cpp
//   • jid::jid, jid::is_bare              → src/xmpp/node.cpp
//   • weechat::consistent_color            → src/color.cpp
//   • weechat::angle_to_weechat_color      → src/color.cpp

#include <doctest/doctest.h>

// ── plugin headers (real symbols declared here) ───────────────────────────────
#include "../src/util.hh"
#include "../src/message.hh"
#include "../src/color.hh"
#include "../src/xmpp/node.hh"

// ── stdlib ────────────────────────────────────────────────────────────────────
#include <cstring>
#include <string>

// ── libstrophe ────────────────────────────────────────────────────────────────
#include <strophe.h>

// ─────────────────────────────────────────────────────────────────────────────
// Shared libstrophe fixture
// ─────────────────────────────────────────────────────────────────────────────
namespace {

struct unit_strophe_env {
    xmpp_ctx_t *ctx{nullptr};
    unit_strophe_env()
    {
        xmpp_initialize();
        ctx = xmpp_ctx_new(nullptr, nullptr);
    }
    ~unit_strophe_env()
    {
        if (ctx)
            xmpp_ctx_free(ctx);
        xmpp_shutdown();
    }
};

}  // namespace

// =============================================================================
// TEST CASES – Group A: pure stdlib / C++23 (src/util.cpp, src/message.cpp)
// =============================================================================

TEST_CASE("char_cmp semantics")
{
    // Returns non-zero when the two pointed-to chars are equal
    char a = 'x', b = 'x', c = 'y';
    CHECK(char_cmp(&a, &b) != 0);  // equal → truthy
    CHECK(char_cmp(&a, &c) == 0);  // unequal → falsy
    // Note: deliberately opposite of strcmp/memcmp convention
}

TEST_CASE("unescape: numeric HTML entity decoding")
{
    SUBCASE("no entities returns input unchanged")
    {
        const std::string s = "hello world";
        CHECK(unescape(s) == s);
    }

    SUBCASE("single entity in the middle")
    {
        // &#65; is 'A'
        CHECK(unescape("&#65;") == "A");
    }

    SUBCASE("entity at start")
    {
        // &#72; = 'H', &#101; = 'e'
        CHECK(unescape("&#72;ello") == "Hello");
    }

    SUBCASE("multiple entities")
    {
        // &#65;&#66;&#67; = "ABC"
        CHECK(unescape("&#65;&#66;&#67;") == "ABC");
    }

    SUBCASE("entity at end")
    {
        // "X&#33;" → "X!"  (33 = '!')
        CHECK(unescape("X&#33;") == "X!");
    }

    SUBCASE("mixed text and entities")
    {
        // "a&#43;b" → "a+b"  (43 = '+')
        CHECK(unescape("a&#43;b") == "a+b");
    }

    SUBCASE("non-entity ampersand left unchanged")
    {
        const std::string s = "AT&T";
        CHECK(unescape(s) == s);
    }

    SUBCASE("empty string returns empty")
    {
        CHECK(unescape("") == "");
    }
}

TEST_CASE("message__htmldecode")
{
    auto decode = [](const std::string& src) -> std::string {
        std::string dest(src.size() + 1, '\0');
        message__htmldecode(dest.data(), src.c_str(), src.size() + 1);
        dest.resize(std::strlen(dest.c_str()));
        return dest;
    };

    SUBCASE("plain text passes through")
    {
        CHECK(decode("hello") == "hello");
    }

    SUBCASE("&gt; decoded to >")
    {
        CHECK(decode("a&gt;b") == "a>b");
    }

    SUBCASE("&lt; decoded to <")
    {
        CHECK(decode("a&lt;b") == "a<b");
    }

    SUBCASE("&amp; decoded to &")
    {
        CHECK(decode("a&amp;b") == "a&b");
    }

    SUBCASE("multiple entities in sequence")
    {
        CHECK(decode("&lt;tag&gt;") == "<tag>");
    }

    SUBCASE("entities adjacent to plain text")
    {
        CHECK(decode("x&amp;y&gt;z") == "x&y>z");
    }

    SUBCASE("unknown entity left as-is")
    {
        // &foo; is not decoded – chars are copied verbatim
        const std::string src = "&foo;";
        const std::string got = decode(src);
        CHECK(got.find('&') != std::string::npos);
    }
}

// =============================================================================
// TEST CASES – Group B: libstrophe stanza accessors and JID parser
// =============================================================================

TEST_CASE("strophe stanza accessors")
{
    unit_strophe_env env;
    REQUIRE(env.ctx != nullptr);

    static constexpr const char *xml =
        "<message from='alice@example.org/phone'"
        " to='bob@example.org'"
        " type='chat'"
        " id='msg1'>"
        "<body>Hello</body>"
        "</message>";

    xmpp_stanza_t *root = xmpp_stanza_new_from_string(env.ctx, xml);
    REQUIRE(root != nullptr);

    SUBCASE("get_name returns element name")
    {
        CHECK(get_name(root) == "message");
    }

    SUBCASE("get_attribute present attribute")
    {
        auto from = get_attribute(root, "from");
        REQUIRE(from.has_value());
        CHECK(*from == "alice@example.org/phone");
    }

    SUBCASE("get_attribute missing attribute returns nullopt")
    {
        auto absent = get_attribute(root, "nonexistent");
        CHECK_FALSE(absent.has_value());
    }

    SUBCASE("get_name on child body element")
    {
        xmpp_stanza_t *body = xmpp_stanza_get_child_by_name(root, "body");
        REQUIRE(body != nullptr);
        CHECK(get_name(body) == "body");
    }

    SUBCASE("get_text on child body returns text content")
    {
        xmpp_stanza_t *body = xmpp_stanza_get_child_by_name(root, "body");
        REQUIRE(body != nullptr);
        // Body text is in a text child
        xmpp_stanza_t *text_node = xmpp_stanza_get_children(body);
        if (text_node && xmpp_stanza_is_text(text_node))
            CHECK(get_text(text_node) == "Hello");
    }

    xmpp_stanza_release(root);
}

TEST_CASE("JID parser")
{
    unit_strophe_env env;
    REQUIRE(env.ctx != nullptr);

    SUBCASE("full JID: local@domain/resource")
    {
        jid j(env.ctx, "alice@example.org/phone");
        CHECK(j.full     == "alice@example.org/phone");
        CHECK(j.bare     == "alice@example.org");
        CHECK(j.local    == "alice");
        CHECK(j.domain   == "example.org");
        CHECK(j.resource == "phone");
        CHECK(j.is_bare() == false);
    }

    SUBCASE("bare JID: local@domain")
    {
        jid j(env.ctx, "alice@example.org");
        CHECK(j.full     == "alice@example.org");
        CHECK(j.bare     == "alice@example.org");
        CHECK(j.local    == "alice");
        CHECK(j.domain   == "example.org");
        CHECK(j.resource.empty());
        CHECK(j.is_bare() == true);
    }

    SUBCASE("domain-only JID (server or component)")
    {
        jid j(env.ctx, "conference.example.org");
        CHECK(j.full   == "conference.example.org");
        CHECK(j.bare   == "conference.example.org");
        CHECK(j.domain == "conference.example.org");
        CHECK(j.local.empty());
        CHECK(j.resource.empty());
        CHECK(j.is_bare() == true);
    }

    SUBCASE("resource may be empty string (trailing slash)")
    {
        jid j(env.ctx, "alice@example.org/");
        CHECK(j.bare   == "alice@example.org");
        CHECK(j.local  == "alice");
        CHECK(j.domain == "example.org");
        CHECK(j.resource.empty());
    }

    SUBCASE("resource with spaces is captured")
    {
        jid j(env.ctx, "alice@example.org/res with spaces");
        CHECK(j.resource == "res with spaces");
    }
}

TEST_CASE("presence show/status extraction")
{
    unit_strophe_env env;
    REQUIRE(env.ctx != nullptr);

    // Helper: return first child with given name's text via real plugin symbols
    auto first_child_text = [&](xmpp_stanza_t *parent, const char *child_name)
        -> std::optional<std::string>
    {
        xmpp_stanza_t *ch = xmpp_stanza_get_child_by_name(parent, child_name);
        if (!ch)
            return std::nullopt;
        xmpp_stanza_t *txt = xmpp_stanza_get_children(ch);
        if (txt && xmpp_stanza_is_text(txt))
            return get_text(txt);
        return std::nullopt;
    };

    SUBCASE("presence with show and status")
    {
        static constexpr const char *xml =
            "<presence from='alice@example.org/phone'>"
            "<show>away</show>"
            "<status>Out for lunch</status>"
            "</presence>";

        xmpp_stanza_t *root = xmpp_stanza_new_from_string(env.ctx, xml);
        REQUIRE(root != nullptr);

        auto show   = first_child_text(root, "show");
        auto status = first_child_text(root, "status");

        REQUIRE(show.has_value());
        CHECK(*show == "away");

        REQUIRE(status.has_value());
        CHECK(*status == "Out for lunch");

        xmpp_stanza_release(root);
    }

    SUBCASE("presence without show or status returns nullopt")
    {
        static constexpr const char *xml =
            "<presence from='alice@example.org/phone'/>";

        xmpp_stanza_t *root = xmpp_stanza_new_from_string(env.ctx, xml);
        REQUIRE(root != nullptr);

        CHECK_FALSE(first_child_text(root, "show").has_value());
        CHECK_FALSE(first_child_text(root, "status").has_value());

        xmpp_stanza_release(root);
    }
}

// =============================================================================
// TEST CASES – Group C: consistent_color / angle_to_weechat_color (src/color.cpp)
// =============================================================================

TEST_CASE("angle_to_weechat_color output range")
{
    SUBCASE("result is always in [16, 231]")
    {
        for (int i = 0; i <= 360; i += 15)
        {
            int color = std::stoi(weechat::angle_to_weechat_color(static_cast<double>(i)));
            CHECK(color >= 16);
            CHECK(color <= 231);
        }
    }

    SUBCASE("normalises negative angle to same result as positive equivalent")
    {
        CHECK(weechat::angle_to_weechat_color(-90.0) == weechat::angle_to_weechat_color(270.0));
    }

    SUBCASE("normalises angle >= 360 to same result as modulo equivalent")
    {
        CHECK(weechat::angle_to_weechat_color(360.0) == weechat::angle_to_weechat_color(0.0));
        CHECK(weechat::angle_to_weechat_color(720.0) == weechat::angle_to_weechat_color(0.0));
        CHECK(weechat::angle_to_weechat_color(400.0) == weechat::angle_to_weechat_color(40.0));
    }

    SUBCASE("0 degrees maps to a valid color")
    {
        int c = std::stoi(weechat::angle_to_weechat_color(0.0));
        CHECK(c >= 16);
        CHECK(c <= 231);
    }
}

TEST_CASE("consistent_color")
{
    SUBCASE("empty string returns empty")
    {
        CHECK(weechat::consistent_color("") == "");
    }

    SUBCASE("non-empty string returns a color in [16, 231]")
    {
        int c = std::stoi(weechat::consistent_color("alice@example.org"));
        CHECK(c >= 16);
        CHECK(c <= 231);
    }

    SUBCASE("case-insensitive: same color for different cases")
    {
        CHECK(weechat::consistent_color("Alice") == weechat::consistent_color("alice"));
        CHECK(weechat::consistent_color("ALICE") == weechat::consistent_color("alice"));
        CHECK(weechat::consistent_color("AlIcE@Example.ORG")
              == weechat::consistent_color("alice@example.org"));
    }

    SUBCASE("different strings produce potentially different colors (determinism)")
    {
        CHECK(weechat::consistent_color("bob@example.org")
              == weechat::consistent_color("bob@example.org"));
    }

    SUBCASE("different JIDs do not necessarily map to the same color")
    {
        CHECK(weechat::consistent_color("alice@example.org")
              != weechat::consistent_color("zzzz@totally-different-domain.net"));
    }
}
