// This Source Code Form is subject to the terms of the Mozilla Public
// License, version 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <string>
#include <strophe.h>

// Parsed representation of an Atom <entry> element (RFC 4287 / XEP-0277 / XEP-0472).
struct atom_entry
{
    std::string title;          // <title>
    std::string summary;        // <summary>
    std::string content;        // <content> body text (XHTML stripped to plain text if needed)
    bool        content_is_xhtml = false; // true when <content type='xhtml'>
    std::string pubdate;        // <published>
    std::string link;           // <link rel='alternate' href='...'/>
    std::string via_link;       // <link rel='via' href='...'/>  (XEP-0472 boost/repeat)
    std::string author;         // <author><name>…</name></author>
    std::string reply_to;       // <thr:in-reply-to ref|href='…'>
    std::string item_id;        // atom <id> element text

    // Convenience: returns the best available body text.
    // XEP-0277/RFC 4287 precedence: <content> beats <summary>.
    const std::string &body() const
    {
        if (!content.empty()) return content;
        if (!summary.empty()) return summary;
        return content; // empty
    }

    bool empty() const
    {
        return title.empty() && summary.empty() && content.empty();
    }
};

// Parse an Atom <entry> stanza into an atom_entry.
// Returns a default-constructed (empty) atom_entry when entry is nullptr.
atom_entry parse_atom_entry(xmpp_ctx_t *ctx, xmpp_stanza_t *entry);
