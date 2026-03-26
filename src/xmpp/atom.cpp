// This Source Code Form is subject to the terms of the Mozilla Public
// License, version 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "atom.hh"

#include <strings.h>  // strcasecmp (POSIX)
#include <strophe.h>

// Helper: read text content of a direct child element by tag name.
static std::string atom_text_child(xmpp_ctx_t *ctx, xmpp_stanza_t *parent, const char *tag)
{
    if (!parent) return {};
    xmpp_stanza_t *el = xmpp_stanza_get_child_by_name(parent, tag);
    if (!el) return {};
    char *t = xmpp_stanza_get_text(el);
    if (!t) return {};
    std::string s(t);
    xmpp_free(ctx, t);
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

    // <content> — check for type='xhtml'
    {
        xmpp_stanza_t *content_el = xmpp_stanza_get_child_by_name(entry, "content");
        if (content_el)
        {
            const char *type_attr = xmpp_stanza_get_attribute(content_el, "type");
            if (type_attr && strcasecmp(type_attr, "xhtml") == 0)
            {
                e.content_is_xhtml = true;
                // For XHTML content: get the inner <body> text recursively.
                // xmpp_stanza_get_text() returns the concatenated text nodes which
                // is a reasonable plain-text approximation without a full XHTML renderer.
                char *t = xmpp_stanza_get_text(content_el);
                if (t)
                {
                    e.content = t;
                    xmpp_free(ctx, t);
                }
            }
            else
            {
                char *t = xmpp_stanza_get_text(content_el);
                if (t)
                {
                    e.content = t;
                    xmpp_free(ctx, t);
                }
            }
        }
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
