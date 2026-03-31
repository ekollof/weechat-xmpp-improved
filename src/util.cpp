// This Source Code Form is subject to the terms of the Mozilla Public
// License, version 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <algorithm>
#include <regex>
#include <string>
#include <sstream>
#include <vector>
#include <cstdint>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strophe.h>
#include <weechat/weechat-plugin.h>

#include "plugin.hh"
#include "util.hh"

XMPP_TEST_EXPORT int char_cmp(const void *p1, const void *p2)
{
    return *(const char *)p1 == *(const char *)p2;
}

XMPP_TEST_EXPORT std::string unescape(const std::string& str)
{
    std::regex regex("\\&\\#(\\d+);");
    std::sregex_iterator begin(str.begin(), str.end(), regex), end;
    if (begin != end)
    {
        std::ostringstream output;
        do {
            std::smatch const& m = *begin;
            if (m[1].matched)
            {
                auto ch = static_cast<char>(std::stoul(m.str(1)));
                output << m.prefix() << ch;
            }
            else output << m.prefix() << m.str(0);
        } while (++begin != end);
        output << str.substr(str.size() - begin->position());
        return output.str();
    }
    return str;
}

// XEP-0393: Message Styling
// Converts XEP-0393 markup to WeeChat color codes
std::string apply_xep393_styling(const std::string& text)
{
    if (text.empty()) return text;
    
    std::string result;
    result.reserve(text.size() * 2); // Reserve extra space for color codes
    
    size_t i = 0;
    bool in_preblock = false;
    
    while (i < text.length())
    {
        // Handle preformatted blocks (```)
        if (!in_preblock && i + 2 < text.length() && 
            text[i] == '`' && text[i+1] == '`' && text[i+2] == '`')
        {
            in_preblock = true;
            result += weechat_color("gray");
            i += 3;
            continue;
        }
        else if (in_preblock && i + 2 < text.length() && 
                 text[i] == '`' && text[i+1] == '`' && text[i+2] == '`')
        {
            in_preblock = false;
            result += weechat_color("resetcolor");
            i += 3;
            continue;
        }
        
        // Inside preformatted block, no styling
        if (in_preblock)
        {
            result += text[i++];
            continue;
        }
        
        // Block quote (> at start of line)
        if ((i == 0 || text[i-1] == '\n') && text[i] == '>' && 
            (i + 1 >= text.length() || text[i+1] == ' '))
        {
            result += weechat_color("green");
            result += '>';
            i++;
            // Continue until end of line
            while (i < text.length() && text[i] != '\n')
            {
                result += text[i++];
            }
            result += weechat_color("resetcolor");
            if (i < text.length())
                result += text[i++]; // Add the newline
            continue;
        }
        
        char ch = text[i];
        
        // Check for inline styling markers
        // Rules: must have whitespace or punctuation before, non-whitespace after
        bool can_start = (i == 0 || isspace(text[i-1]) || ispunct(text[i-1]));
        
        if (can_start && i + 1 < text.length())
        {
            // *strong* (bold)
            if (ch == '*' && !isspace(text[i+1]))
            {
                size_t end = i + 1;
                // XEP-0393: spans MUST NOT cross line boundaries
                while (end < text.length() && text[end] != '*' && text[end] != '\n')
                    end++;
                
                if (end < text.length() && text[end] == '*' && !isspace(text[end-1]) &&
                    (end + 1 >= text.length() || isspace(text[end+1]) || ispunct(text[end+1])))
                {
                    result += weechat_color("bold");
                    i++;
                    while (i < end)
                        result += text[i++];
                    result += weechat_color("-bold");
                    i++; // Skip closing *
                    continue;
                }
            }
            
            // _emphasis_ (italic/underline)
            else if (ch == '_' && !isspace(text[i+1]))
            {
                size_t end = i + 1;
                // XEP-0393: spans MUST NOT cross line boundaries
                while (end < text.length() && text[end] != '_' && text[end] != '\n')
                    end++;
                
                if (end < text.length() && text[end] == '_' && !isspace(text[end-1]) &&
                    (end + 1 >= text.length() || isspace(text[end+1]) || ispunct(text[end+1])))
                {
                    result += weechat_color("underline");
                    i++;
                    while (i < end)
                        result += text[i++];
                    result += weechat_color("-underline");
                    i++; // Skip closing _
                    continue;
                }
            }
            
            // `monospace` (inline code)
            else if (ch == '`' && !isspace(text[i+1]))
            {
                size_t end = i + 1;
                // XEP-0393: spans MUST NOT cross line boundaries
                while (end < text.length() && text[end] != '`' && text[end] != '\n')
                    end++;
                
                if (end < text.length() && text[end] == '`' && !isspace(text[end-1]) &&
                    (end + 1 >= text.length() || isspace(text[end+1]) || ispunct(text[end+1])))
                {
                    result += weechat_color("gray");
                    i++;
                    while (i < end)
                        result += text[i++];
                    result += weechat_color("resetcolor");
                    i++; // Skip closing `
                    continue;
                }
            }
            
            // ~~strikethrough~~ (XEP-0393 uses double-tilde)
            else if (ch == '~' && i + 2 < text.length() && text[i+1] == '~' && !isspace(text[i+2]))
            {
                size_t end = i + 2;
                // XEP-0393: spans MUST NOT cross line boundaries; look for ~~
                while (end + 1 < text.length() &&
                       !(text[end] == '~' && text[end+1] == '~') &&
                       text[end] != '\n')
                    end++;
                
                if (end + 1 < text.length() && text[end] == '~' && text[end+1] == '~' &&
                    !isspace(text[end-1]) &&
                    (end + 2 >= text.length() || isspace(text[end+2]) || ispunct(text[end+2])))
                {
                    result += weechat_color("red");
                    i += 2; // Skip opening ~~
                    while (i < end)
                        result += text[i++];
                    result += weechat_color("resetcolor");
                    i += 2; // Skip closing ~~
                    continue;
                }
            }
        }
        
        // No styling matched, just copy character
        result += text[i++];
    }
    
    return result;
}

// XEP-0394: Message Markup (receive-only)
// Applies <markup xmlns='urn:xmpp:markup:0'> to `plain_text`, returning a
// WeeChat colour-coded string.  Returns empty string if no <markup> child
// exists in `stanza`.
std::string apply_xep394_markup(xmpp_stanza_t *stanza, const std::string &plain_text)
{
    if (!stanza || plain_text.empty()) return {};

    xmpp_stanza_t *markup_elem = xmpp_stanza_get_child_by_name_and_ns(
        stanza, "markup", "urn:xmpp:markup:0");
    if (!markup_elem) return {};

    // Build a table mapping unicode-codepoint index → UTF-8 byte offset.
    // XEP-0394 start/end are in unicode codepoints; we need byte positions.
    std::vector<std::size_t> cp_to_byte; // cp_to_byte[cp] = byte offset
    cp_to_byte.reserve(plain_text.size() + 1);
    {
        std::size_t byte = 0;
        while (byte <= plain_text.size())
        {
            cp_to_byte.push_back(byte);
            if (byte == plain_text.size()) break;
            // Advance one UTF-8 codepoint
            unsigned char c = static_cast<unsigned char>(plain_text[byte]);
            std::size_t len = 1;
            if      ((c & 0x80) == 0x00) len = 1;
            else if ((c & 0xE0) == 0xC0) len = 2;
            else if ((c & 0xF0) == 0xE0) len = 3;
            else if ((c & 0xF8) == 0xF0) len = 4;
            byte += len;
        }
    }
    std::size_t total_cps [[maybe_unused]] = cp_to_byte.size() - 1; // last entry is end-sentinel

    auto cp_byte = [&](long cp) -> std::size_t {
        if (cp < 0) return 0;
        auto idx = static_cast<std::size_t>(cp);
        if (idx >= cp_to_byte.size()) return plain_text.size();
        return cp_to_byte[idx];
    };

    // Events: at a given byte offset, insert a WeeChat color string.
    // We sort by offset then by priority (open before close at same position).
    struct Event {
        std::size_t byte_off;
        int         priority; // lower = applied first
        std::string code;
    };
    std::vector<Event> events;

    // Helper to get start/end codepoint attributes
    auto get_long_attr = [](xmpp_stanza_t *el, const char *attr, long fallback) -> long {
        const char *v = xmpp_stanza_get_attribute(el, attr);
        if (!v) return fallback;
        char *end = nullptr;
        long val = std::strtol(v, &end, 10);
        return (end && end != v) ? val : fallback;
    };

    for (xmpp_stanza_t *child = xmpp_stanza_get_children(markup_elem);
         child; child = xmpp_stanza_get_next(child))
    {
        const char *child_name = xmpp_stanza_get_name(child);
        if (!child_name) continue;

        if (strcmp(child_name, "span") == 0)
        {
            long start = get_long_attr(child, "start", -1);
            long end   = get_long_attr(child, "end",   -1);
            if (start < 0 || end <= start) continue;

            // Determine which style this span applies (first child wins)
            std::string open_code, close_code;
            for (xmpp_stanza_t *sp = xmpp_stanza_get_children(child);
                 sp; sp = xmpp_stanza_get_next(sp))
            {
                const char *sp_name = xmpp_stanza_get_name(sp);
                if (!sp_name) continue;
                if (strcmp(sp_name, "emphasis") == 0) {
                    open_code  = weechat_color("italic");
                    close_code = weechat_color("-italic");
                } else if (strcmp(sp_name, "strong") == 0) {
                    open_code  = weechat_color("bold");
                    close_code = weechat_color("-bold");
                } else if (strcmp(sp_name, "code") == 0) {
                    open_code  = weechat_color("cyan");
                    close_code = weechat_color("resetcolor");
                } else if (strcmp(sp_name, "deleted") == 0) {
                    open_code  = weechat_color("8");   // dark grey ≈ strikethrough hint
                    close_code = weechat_color("resetcolor");
                }
                if (!open_code.empty()) break;
            }
            if (open_code.empty()) continue;

            events.push_back({cp_byte(start), 0, std::move(open_code)});
            events.push_back({cp_byte(end),   1, std::move(close_code)});
        }
        else if (strcmp(child_name, "bcode") == 0)
        {
            long start = get_long_attr(child, "start", -1);
            long end   = get_long_attr(child, "end",   -1);
            if (start < 0 || end <= start) continue;
            events.push_back({cp_byte(start), 0, weechat_color("gray")});
            events.push_back({cp_byte(end),   1, weechat_color("resetcolor")});
        }
        else if (strcmp(child_name, "bquote") == 0)
        {
            long start = get_long_attr(child, "start", -1);
            long end   = get_long_attr(child, "end",   -1);
            if (start < 0 || end <= start) continue;
            // Insert green color at start of each line within [start, end)
            events.push_back({cp_byte(start), 0, weechat_color("green")});
            // Also emit green after every newline within the range
            std::size_t sb = cp_byte(start);
            std::size_t eb = cp_byte(end);
            for (std::size_t b = sb; b < eb && b < plain_text.size(); ++b)
            {
                if (plain_text[b] == '\n' && b + 1 < eb)
                    events.push_back({b + 1, 0, weechat_color("green")});
            }
            events.push_back({cp_byte(end), 1, weechat_color("resetcolor")});
        }
        else if (strcmp(child_name, "list") == 0)
        {
            // Each <li start="N"/> inserts a bullet marker at that codepoint.
            for (xmpp_stanza_t *li = xmpp_stanza_get_children(child);
                 li; li = xmpp_stanza_get_next(li))
            {
                const char *li_name = xmpp_stanza_get_name(li);
                if (!li_name || strcmp(li_name, "li") != 0) continue;
                long li_start = get_long_attr(li, "start", -1);
                if (li_start < 0) continue;
                // Insert "• " before the list item start
                events.push_back({cp_byte(li_start), -1, "• "});
            }
        }
    }

    if (events.empty()) return plain_text; // markup present but no actionable elements

    // Sort events: by byte offset, then by priority
    std::sort(events.begin(), events.end(), [](const Event &a, const Event &b) {
        if (a.byte_off != b.byte_off) return a.byte_off < b.byte_off;
        return a.priority < b.priority;
    });

    // Build output
    std::string result;
    result.reserve(plain_text.size() + events.size() * 8);
    std::size_t pos = 0;
    for (const auto &ev : events)
    {
        if (ev.byte_off > pos && ev.byte_off <= plain_text.size())
        {
            result += plain_text.substr(pos, ev.byte_off - pos);
            pos = ev.byte_off;
        }
        result += ev.code;
    }
    if (pos < plain_text.size())
        result += plain_text.substr(pos);

    return result;
}
