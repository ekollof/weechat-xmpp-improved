// This Source Code Form is subject to the terms of the Mozilla Public
// License, version 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// Unit tests for pure and near-pure plugin functions.
//
// Because all plugin symbols have hidden visibility (-fvisibility=hidden),
// we cannot call them from the test binary directly.  Instead we inline the
// same logic here – identical to the approach used in omemo_xep.inl, which
// re-implements its validators without calling plugin symbols.
//
// Groups covered:
//   A – pure stdlib/C++23: unescape, char_cmp, message__htmldecode
//   B – libstrophe context: get_name, get_attribute, get_text, jid parsing,
//        xml::node::bind-equivalent, xml::presence show/status
//   C – OpenSSL: consistent_color / angle_to_weechat_color

#include <doctest/doctest.h>

// ── stdlib ───────────────────────────────────────────────────────────────────
#include <cmath>
#include <cstdint>
#include <cstring>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>

// ── OpenSSL ──────────────────────────────────────────────────────────────────
#include <openssl/sha.h>

// ── libstrophe ───────────────────────────────────────────────────────────────
#include <strophe.h>

// ─────────────────────────────────────────────────────────────────────────────
// Shared fixture (identical pattern to omemo_xep.inl)
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

// ─────────────────────────────────────────────────────────────────────────────
// Group A helpers – inline mirrors of src/util.cpp and src/message.cpp
// ─────────────────────────────────────────────────────────────────────────────

// Mirror of char_cmp() in src/util.cpp:17
// Returns non-zero when *p1 == *p2 (OPPOSITE of strcmp semantics).
int test_char_cmp(const void *p1, const void *p2)
{
    return *static_cast<const char *>(p1) == *static_cast<const char *>(p2);
}

// Mirror of unescape() in src/util.cpp:22
std::string test_unescape(const std::string& str)
{
    std::regex re("\\&\\#(\\d+);");
    std::sregex_iterator begin(str.begin(), str.end(), re), end;
    if (begin != end)
    {
        std::ostringstream out;
        do {
            const std::smatch& m = *begin;
            if (m[1].matched)
                out << m.prefix() << static_cast<char>(std::stoul(m.str(1)));
            else
                out << m.prefix() << m.str(0);
        } while (++begin != end);
        out << str.substr(str.size() - begin->position());
        return out.str();
    }
    return str;
}

// Mirror of message__htmldecode() in src/message.cpp:110
void test_htmldecode(char *dest, const char *src, std::size_t n)
{
    std::size_t i = 0, j = 0;
    for (; i < n; ++i, ++j)
    {
        switch (src[i])
        {
            case '\0':
                dest[j] = '\0';
                return;
            case '&':
                if (src[i+1]=='g' && src[i+2]=='t' && src[i+3]==';')
                { dest[j] = '>'; i += 3; break; }
                else if (src[i+1]=='l' && src[i+2]=='t' && src[i+3]==';')
                { dest[j] = '<'; i += 3; break; }
                else if (src[i+1]=='a' && src[i+2]=='m' && src[i+3]=='p' && src[i+4]==';')
                { dest[j] = '&'; i += 4; break; }
                /* fallthrough */
                [[fallthrough]];
            default:
                dest[j] = src[i];
                break;
        }
    }
    dest[j-1] = '\0';
}

// ─────────────────────────────────────────────────────────────────────────────
// Group B helpers – libstrophe wrappers mirroring src/xmpp/node.cpp
// ─────────────────────────────────────────────────────────────────────────────

std::string test_get_name(xmpp_stanza_t *s)
{
    const char *r = xmpp_stanza_get_name(s);
    return r ? r : "";
}

std::optional<std::string> test_get_attribute(xmpp_stanza_t *s, const char *name)
{
    const char *r = xmpp_stanza_get_attribute(s, name);
    return r ? std::optional<std::string>(r) : std::nullopt;
}

std::string test_get_text(xmpp_stanza_t *s)
{
    const char *r = xmpp_stanza_get_text_ptr(s);
    return r ? r : "";
}

// Mirror of jid parser in src/xmpp/node.cpp:56–80
struct test_jid {
    std::string full;
    std::string bare;
    std::string local;
    std::string domain;
    std::string resource;
    bool        bare_jid{false};  // true when no resource

    explicit test_jid(std::string s) : full(std::move(s))
    {
        static const std::regex pat(
            "^((?:([^@/<>'\"]+)@)?([^@/<>'\"]+))(?:/([^<>'\"]*))?$");
        std::smatch m;
        if (std::regex_search(full, m, pat))
        {
            auto as_str = [&](const std::ssub_match& sm) -> std::string {
                return sm.matched ? sm.str() : "";
            };
            bare     = as_str(m[1]);
            local    = as_str(m[2]);
            domain   = as_str(m[3]);
            resource = as_str(m[4]);
        }
        else
        {
            bare   = full;
            domain = bare;
        }
        bare_jid = resource.empty();
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Group C helpers – mirrors of src/color.cpp
// ─────────────────────────────────────────────────────────────────────────────

static double test_generate_angle(const std::string& input)
{
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char *>(input.c_str()),
         input.size(), hash);
    std::uint16_t v = static_cast<std::uint16_t>(hash[0] | (hash[1] << 8));
    return (static_cast<double>(v) / 65536.0) * 360.0;
}

std::string test_angle_to_color(double angle)
{
    while (angle < 0)     angle += 360.0;
    while (angle >= 360.0) angle -= 360.0;

    double h  = angle / 60.0;
    double x  = 1.0 - std::abs(std::fmod(h, 2.0) - 1.0);
    double r, g, b;
    switch (static_cast<int>(h) % 6)
    {
        case 0: r=1; g=x; b=0; break;
        case 1: r=x; g=1; b=0; break;
        case 2: r=0; g=1; b=x; break;
        case 3: r=0; g=x; b=1; break;
        case 4: r=x; g=0; b=1; break;
        case 5: r=1; g=0; b=x; break;
        default: r=0; g=0; b=0; break;
    }
    int ri = static_cast<int>(r * 5.0 + 0.5);
    int gi = static_cast<int>(g * 5.0 + 0.5);
    int bi = static_cast<int>(b * 5.0 + 0.5);
    return std::to_string(16 + 36*ri + 6*gi + bi);
}

std::string test_consistent_color(const std::string& input)
{
    if (input.empty()) return "";
    std::string norm = input;
    for (char& c : norm)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return test_angle_to_color(test_generate_angle(norm));
}

}  // namespace

// =============================================================================
// TEST CASES – Group A: pure stdlib / C++23
// =============================================================================

TEST_CASE("char_cmp semantics")
{
    // Returns non-zero when the two pointed-to chars are equal
    char a = 'x', b = 'x', c = 'y';
    CHECK(test_char_cmp(&a, &b) != 0);  // equal → truthy
    CHECK(test_char_cmp(&a, &c) == 0);  // unequal → falsy
    // Note: deliberately opposite of strcmp/memcmp convention
}

TEST_CASE("unescape: numeric HTML entity decoding")
{
    SUBCASE("no entities returns input unchanged")
    {
        const std::string s = "hello world";
        CHECK(test_unescape(s) == s);
    }

    SUBCASE("single entity in the middle")
    {
        // &#65; is 'A'
        CHECK(test_unescape("&#65;") == "A");
    }

    SUBCASE("entity at start")
    {
        // &#72; = 'H', &#101; = 'e'
        CHECK(test_unescape("&#72;ello") == "Hello");
    }

    SUBCASE("multiple entities")
    {
        // &#65;&#66;&#67; = "ABC"
        CHECK(test_unescape("&#65;&#66;&#67;") == "ABC");
    }

    SUBCASE("entity at end")
    {
        // "X&#33;" → "X!"  (33 = '!')
        CHECK(test_unescape("X&#33;") == "X!");
    }

    SUBCASE("mixed text and entities")
    {
        // "a&#43;b" → "a+b"  (43 = '+')
        CHECK(test_unescape("a&#43;b") == "a+b");
    }

    SUBCASE("non-entity ampersand left unchanged")
    {
        const std::string s = "AT&T";
        CHECK(test_unescape(s) == s);
    }

    SUBCASE("empty string returns empty")
    {
        CHECK(test_unescape("") == "");
    }
}

TEST_CASE("message__htmldecode")
{
    auto decode = [](const std::string& src) -> std::string {
        std::string dest(src.size() + 1, '\0');
        test_htmldecode(dest.data(), src.c_str(), src.size() + 1);
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
        CHECK(test_get_name(root) == "message");
    }

    SUBCASE("get_attribute present attribute")
    {
        auto from = test_get_attribute(root, "from");
        REQUIRE(from.has_value());
        CHECK(*from == "alice@example.org/phone");
    }

    SUBCASE("get_attribute missing attribute returns nullopt")
    {
        auto absent = test_get_attribute(root, "nonexistent");
        CHECK_FALSE(absent.has_value());
    }

    SUBCASE("get_name on child body element")
    {
        xmpp_stanza_t *body = xmpp_stanza_get_child_by_name(root, "body");
        REQUIRE(body != nullptr);
        CHECK(test_get_name(body) == "body");
    }

    SUBCASE("get_text on child body returns text content")
    {
        xmpp_stanza_t *body = xmpp_stanza_get_child_by_name(root, "body");
        REQUIRE(body != nullptr);
        // Body text is in a text child
        xmpp_stanza_t *text_node = xmpp_stanza_get_children(body);
        if (text_node && xmpp_stanza_is_text(text_node))
            CHECK(test_get_text(text_node) == "Hello");
    }

    xmpp_stanza_release(root);
}

TEST_CASE("JID parser")
{
    SUBCASE("full JID: local@domain/resource")
    {
        test_jid j("alice@example.org/phone");
        CHECK(j.full     == "alice@example.org/phone");
        CHECK(j.bare     == "alice@example.org");
        CHECK(j.local    == "alice");
        CHECK(j.domain   == "example.org");
        CHECK(j.resource == "phone");
        CHECK(j.bare_jid == false);
    }

    SUBCASE("bare JID: local@domain")
    {
        test_jid j("alice@example.org");
        CHECK(j.full     == "alice@example.org");
        CHECK(j.bare     == "alice@example.org");
        CHECK(j.local    == "alice");
        CHECK(j.domain   == "example.org");
        CHECK(j.resource.empty());
        CHECK(j.bare_jid == true);
    }

    SUBCASE("domain-only JID (server or component)")
    {
        test_jid j("conference.example.org");
        CHECK(j.full   == "conference.example.org");
        CHECK(j.bare   == "conference.example.org");
        CHECK(j.domain == "conference.example.org");
        CHECK(j.local.empty());
        CHECK(j.resource.empty());
        CHECK(j.bare_jid == true);
    }

    SUBCASE("resource may be empty string (trailing slash)")
    {
        // A JID like "alice@example.org/" has an empty resource
        test_jid j("alice@example.org/");
        CHECK(j.bare   == "alice@example.org");
        CHECK(j.local  == "alice");
        CHECK(j.domain == "example.org");
        CHECK(j.resource.empty());
    }

    SUBCASE("resource with slash-like characters is captured")
    {
        test_jid j("alice@example.org/res with spaces");
        CHECK(j.resource == "res with spaces");
    }
}

TEST_CASE("presence show/status extraction")
{
    unit_strophe_env env;
    REQUIRE(env.ctx != nullptr);

    // Helper: return first child with given name's text
    auto first_child_text = [&](xmpp_stanza_t *parent, const char *child_name)
        -> std::optional<std::string>
    {
        xmpp_stanza_t *ch = xmpp_stanza_get_child_by_name(parent, child_name);
        if (!ch)
            return std::nullopt;
        xmpp_stanza_t *txt = xmpp_stanza_get_children(ch);
        if (txt && xmpp_stanza_is_text(txt))
            return test_get_text(txt);
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
// TEST CASES – Group C: consistent_color / angle_to_weechat_color
// =============================================================================

TEST_CASE("angle_to_weechat_color output range")
{
    SUBCASE("result is always in [16, 231]")
    {
        // Sample angles across full circle
        for (int i = 0; i <= 360; i += 15)
        {
            int color = std::stoi(test_angle_to_color(static_cast<double>(i)));
            CHECK(color >= 16);
            CHECK(color <= 231);
        }
    }

    SUBCASE("normalises negative angle to same result as positive equivalent")
    {
        CHECK(test_angle_to_color(-90.0) == test_angle_to_color(270.0));
    }

    SUBCASE("normalises angle >= 360 to same result as modulo equivalent")
    {
        CHECK(test_angle_to_color(360.0) == test_angle_to_color(0.0));
        CHECK(test_angle_to_color(720.0) == test_angle_to_color(0.0));
        CHECK(test_angle_to_color(400.0) == test_angle_to_color(40.0));
    }

    SUBCASE("0 degrees maps to a valid color")
    {
        int c = std::stoi(test_angle_to_color(0.0));
        CHECK(c >= 16);
        CHECK(c <= 231);
    }
}

TEST_CASE("consistent_color")
{
    SUBCASE("empty string returns empty")
    {
        CHECK(test_consistent_color("") == "");
    }

    SUBCASE("non-empty string returns a color in [16, 231]")
    {
        int c = std::stoi(test_consistent_color("alice@example.org"));
        CHECK(c >= 16);
        CHECK(c <= 231);
    }

    SUBCASE("case-insensitive: same color for different cases")
    {
        CHECK(test_consistent_color("Alice") == test_consistent_color("alice"));
        CHECK(test_consistent_color("ALICE") == test_consistent_color("alice"));
        CHECK(test_consistent_color("AlIcE@Example.ORG")
              == test_consistent_color("alice@example.org"));
    }

    SUBCASE("different strings produce potentially different colors (determinism)")
    {
        // Both calls with the same input must agree
        CHECK(test_consistent_color("bob@example.org")
              == test_consistent_color("bob@example.org"));
    }

    SUBCASE("different JIDs do not necessarily map to the same color")
    {
        // Probabilistically very unlikely to collide for clearly different strings
        CHECK(test_consistent_color("alice@example.org")
              != test_consistent_color("zzzz@totally-different-domain.net"));
    }
}
