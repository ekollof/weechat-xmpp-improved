// This Source Code Form is subject to the terms of the Mozilla Public
// License, version 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <cstddef>
#include <cstdint>
#include <strophe.h>

#include "xep-0163.inl"

namespace weechat::xep0084
{
    // XEP-0084: User Avatar (PEP-based)
    // Namespace constants
    inline constexpr const char *METADATA_NS = "urn:xmpp:avatar:metadata";
    inline constexpr const char *DATA_NS = "urn:xmpp:avatar:data";
    
    // Request avatar data from PubSub
    inline xmpp_stanza_t *request_avatar_data(xmpp_ctx_t *context, 
                                              const char *to, 
                                              const char *id)
    {
        xmpp_stanza_t *iq = xmpp_iq_new(context, "get", id);
        xmpp_stanza_set_to(iq, to);
        
        xmpp_stanza_t *pubsub = xmpp_stanza_new(context);
        xmpp_stanza_set_name(pubsub, "pubsub");
        xmpp_stanza_set_ns(pubsub, "http://jabber.org/protocol/pubsub");
        
        xmpp_stanza_t *items = xmpp_stanza_new(context);
        xmpp_stanza_set_name(items, "items");
        xmpp_stanza_set_attribute(items, "node", DATA_NS);
        
        xmpp_stanza_add_child(pubsub, items);
        xmpp_stanza_add_child(iq, pubsub);
        
        xmpp_stanza_release(items);
        xmpp_stanza_release(pubsub);
        
        return iq;
    }

    // Publish avatar image data to urn:xmpp:avatar:data PEP node.
    // base64_data: base64-encoded raw image bytes (no newlines).
    // hash_id:     hex SHA-1 of the raw image bytes (used as item id).
    inline xmpp_stanza_t *publish_avatar_data(xmpp_ctx_t *context,
                                              const char *base64_data,
                                              const char *hash_id)
    {
        xmpp_stanza_t *data_elem = xmpp_stanza_new(context);
        xmpp_stanza_set_name(data_elem, "data");
        xmpp_stanza_set_ns(data_elem, DATA_NS);

        xmpp_stanza_t *text = xmpp_stanza_new(context);
        xmpp_stanza_set_text(text, base64_data);
        xmpp_stanza_add_child(data_elem, text);
        xmpp_stanza_release(text);

        xmpp_stanza_t *iq = ::xmpp::xep0163::publish_pep(context, DATA_NS, data_elem, hash_id);
        xmpp_stanza_release(data_elem);
        return iq;
    }

    // Publish avatar metadata to urn:xmpp:avatar:metadata PEP node.
    // hash_id:   hex SHA-1 (used as item id and info id attribute).
    // mime_type: e.g. "image/png" or "image/jpeg".
    // bytes:     raw file size in bytes.
    // width/height: image dimensions (may be 0 if unknown).
    inline xmpp_stanza_t *publish_avatar_metadata(xmpp_ctx_t *context,
                                                  const char *hash_id,
                                                  const char *mime_type,
                                                  size_t bytes,
                                                  uint32_t width,
                                                  uint32_t height)
    {
        xmpp_stanza_t *metadata = xmpp_stanza_new(context);
        xmpp_stanza_set_name(metadata, "metadata");
        xmpp_stanza_set_ns(metadata, METADATA_NS);

        xmpp_stanza_t *info = xmpp_stanza_new(context);
        xmpp_stanza_set_name(info, "info");
        xmpp_stanza_set_attribute(info, "id", hash_id);
        xmpp_stanza_set_attribute(info, "type", mime_type);

        char buf[32];
        snprintf(buf, sizeof(buf), "%zu", bytes);
        xmpp_stanza_set_attribute(info, "bytes", buf);

        if (width > 0)
        {
            snprintf(buf, sizeof(buf), "%u", width);
            xmpp_stanza_set_attribute(info, "width", buf);
        }
        if (height > 0)
        {
            snprintf(buf, sizeof(buf), "%u", height);
            xmpp_stanza_set_attribute(info, "height", buf);
        }

        xmpp_stanza_add_child(metadata, info);
        xmpp_stanza_release(info);

        xmpp_stanza_t *iq = ::xmpp::xep0163::publish_pep(context, METADATA_NS, metadata, hash_id);
        xmpp_stanza_release(metadata);
        return iq;
    }
}
