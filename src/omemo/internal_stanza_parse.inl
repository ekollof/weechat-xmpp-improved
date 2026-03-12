        const char *name = xmpp_stanza_get_name(child);
        if (!name)
            continue;

        if (weechat_strcasecmp(name, "spk") == 0)
        {
            const char *id = xmpp_stanza_get_attribute(child, "id");
            if (id)
                bundle.signed_pre_key_id = id;
            bundle.signed_pre_key = stanza_text(child);
        }
        else if (weechat_strcasecmp(name, "spks") == 0)
        {
            bundle.signed_pre_key_signature = stanza_text(child);
        }
        else if (weechat_strcasecmp(name, "ik") == 0)
        {
            bundle.identity_key = stanza_text(child);
        }
        else if (weechat_strcasecmp(name, "prekeys") == 0)
        {
            for (xmpp_stanza_t *prekey = xmpp_stanza_get_children(child);
                 prekey;
                 prekey = xmpp_stanza_get_next(prekey))
            {
                const char *prekey_name = xmpp_stanza_get_name(prekey);
                if (!prekey_name || weechat_strcasecmp(prekey_name, "pk") != 0)
                    continue;

                const char *id = xmpp_stanza_get_attribute(prekey, "id");
                if (!id)
                    continue;

                const auto parsed_id = parse_uint32(id).value_or(0);
                if (!is_valid_omemo_device_id(parsed_id))
                {
                    weechat_printf(nullptr,
                                   "%somemo: bundle parse failed: invalid prekey id '%s'",
                                   weechat_prefix("error"),
                                   id);
                    return std::nullopt;
                }

                const bool duplicate_id = std::ranges::any_of(
                    bundle.prekeys,
                    [&](const auto &existing) {
                        return existing.first == id;
                    });
                if (duplicate_id)
                {
                    weechat_printf(nullptr,
                                   "%somemo: bundle parse failed: duplicate prekey id '%s'",
                                   weechat_prefix("error"),
                                   id);
                    return std::nullopt;
                }

                bundle.prekeys.emplace_back(id, stanza_text(prekey));
            }
        }
    }

    if (!is_valid_omemo_device_id(parse_uint32(bundle.signed_pre_key_id).value_or(0)))
    {
        weechat_printf(nullptr,
                       "%somemo: bundle parse failed: invalid signed prekey id '%s'",
                       weechat_prefix("error"),
                       bundle.signed_pre_key_id.c_str());
        return std::nullopt;
    }

    if (bundle.signed_pre_key_id.empty() || bundle.signed_pre_key.empty()
        || bundle.signed_pre_key_signature.empty() || bundle.identity_key.empty())
    {
        weechat_printf(nullptr,
                       "%somemo: bundle parse failed: missing required spk/spks/ik fields",
                       weechat_prefix("error"));
        return std::nullopt;
    }

    if (bundle.prekeys.empty())
    {
        weechat_printf(nullptr,
                       "%somemo: bundle parse failed: bundle has no prekeys",
                       weechat_prefix("error"));
        return std::nullopt;
    }

    const bool has_empty_prekey = std::ranges::any_of(
        bundle.prekeys,
        [](const auto &prekey) {
            return prekey.first.empty() || prekey.second.empty();
        });
    if (has_empty_prekey)
    {
        weechat_printf(nullptr,
                       "%somemo: bundle parse failed: bundle contains empty prekey entries",
                       weechat_prefix("error"));
        return std::nullopt;
    }

    return bundle;
}

void signal_log_emit(int level, const char *message, std::size_t length, void *user_data)
{
    (void) user_data;

    const char *prefix = weechat_prefix(level <= SG_LOG_WARNING ? "error" : "network");
    std::string text {message, length};
    weechat_printf(nullptr, "%somemo: %s", prefix, text.c_str());
}

