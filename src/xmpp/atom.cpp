// This Source Code Form is subject to the terms of the Mozilla Public
// License, version 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "atom.hh"
#include "xhtml.hh"

#include <strings.h>  // strcasecmp (POSIX)
#include <strophe.h>

// Helper: read text content of a direct child element by tag name.
// If the element has type='xhtml', render via xhtml_to_weechat().
// If the element has type='html', strip tags via html_strip_to_plain().
static std::string atom_text_child(xmpp_ctx_t *ctx, xmpp_stanza_t *parent, const char *tag)
{
    if (!parent) return {};
    xmpp_stanza_t *el = xmpp_stanza_get_child_by_name(parent, tag);
    if (!el) return {};
    const char *type_attr = xmpp_stanza_get_attribute(el, "type");
    if (type_attr && strcasecmp(type_attr, "xhtml") == 0)
        return xhtml_to_weechat(el);
    char *t = xmpp_stanza_get_text(el);
    if (!t) return {};
    std::string s(t);
    xmpp_free(ctx, t);
    if (type_attr && strcasecmp(type_attr, "html") == 0)
        return html_strip_to_plain(s);
    return s;
}

atom_entry parse_atom_entry(xmpp_ctx_t *ctx, xmpp_stanza_t *entry)
{
    atom_entry e;
    if (!entry) return e;

    e.title   = atom_text_child(ctx, entry, "title");
    e.summary = atom_text_child(ctx, entry, "summary");
    e.pubdate = atom_text_child(ctx, entry, "published");
    e.item_id = atom_text_child(ctx, entry, "id");

    // <content> — RFC 4287 §4.1.3.
    // Movim and other clients often publish both type='text' and type='xhtml'.
    // Prefer plain text when available; fall back to XHTML (rendered) or HTML
    // (tag-stripped).  Iterate all <content> children to find the best one.
    {
        std::string text_content, xhtml_content, html_content;
        bool found_text = false, found_xhtml = false, found_html = false;

        for (xmpp_stanza_t *child = xmpp_stanza_get_children(entry);
             child; child = xmpp_stanza_get_next(child))
        {
            const char *child_name = xmpp_stanza_get_name(child);
            if (!child_name || strcasecmp(child_name, "content") != 0)
                continue;

            const char *type_attr = xmpp_stanza_get_attribute(child, "type");

            if (!type_attr || strcasecmp(type_attr, "text") == 0)
            {
                if (!found_text)
                {
                    char *t = xmpp_stanza_get_text(child);
                    if (t) { text_content = t; xmpp_free(ctx, t); found_text = true; }
                }
            }
            else if (strcasecmp(type_attr, "xhtml") == 0)
            {
                if (!found_xhtml)
                {
                    // xhtml_to_weechat() renders the stanza tree with WeeChat
                    // colour/attribute codes for bold, italic, links, etc.
                    xhtml_content = xhtml_to_weechat(child);
                    found_xhtml = !xhtml_content.empty();
                    if (found_xhtml) e.content_is_xhtml = true;
                }
            }
            else if (strcasecmp(type_attr, "html") == 0)
            {
                if (!found_html)
                {
                    char *t = xmpp_stanza_get_text(child);
                    if (t)
                    {
                        html_content = html_strip_to_plain(std::string(t));
                        xmpp_free(ctx, t);
                        found_html = !html_content.empty();
                    }
                }
            }
        }

        // Preference order: plain text > XHTML rendered > HTML stripped
        if (found_text)
            e.content = std::move(text_content);
        else if (found_xhtml)
            e.content = std::move(xhtml_content);
        else if (found_html)
            e.content = std::move(html_content);
    }

    // <author><name>…</name></author>
    {
        xmpp_stanza_t *author_el = xmpp_stanza_get_child_by_name(entry, "author");
        if (author_el)
        {
            xmpp_stanza_t *name_el = xmpp_stanza_get_child_by_name(author_el, "name");
            if (name_el)
            {
                char *t = xmpp_stanza_get_text(name_el);
                if (t)
                {
                    e.author = t;
                    xmpp_free(ctx, t);
                }
            }
        }
    }

    // Iterate children for <link> and <thr:in-reply-to>
    for (xmpp_stanza_t *child = xmpp_stanza_get_children(entry);
         child; child = xmpp_stanza_get_next(child))
    {
        const char *child_name = xmpp_stanza_get_name(child);
        if (!child_name) continue;

        if (strcasecmp(child_name, "link") == 0)
        {
            const char *rel  = xmpp_stanza_get_attribute(child, "rel");
            const char *href = xmpp_stanza_get_attribute(child, "href");
            if (!href) continue;

            if (!rel || strcasecmp(rel, "alternate") == 0)
            {
                if (e.link.empty())
                    e.link = href;
            }
            else if (strcasecmp(rel, "via") == 0)
            {
                if (e.via_link.empty())
                    e.via_link = href;
            }
        }
        else if (strcasecmp(child_name, "in-reply-to") == 0 ||
                 strcasecmp(child_name, "thr:in-reply-to") == 0)
        {
            // Prefer href (full XMPP URI) but fall back to ref (item ID).
            if (e.reply_to.empty())
            {
                const char *href = xmpp_stanza_get_attribute(child, "href");
                if (href)
                    e.reply_to = href;
                else
                {
                    const char *ref = xmpp_stanza_get_attribute(child, "ref");
                    if (ref) e.reply_to = ref;
                }
            }
        }
    }

    return e;
}
