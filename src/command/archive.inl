int command__mam(const void *pointer, void *data,
                 struct t_gui_buffer *buffer, int argc,
                 char **argv, char **argv_eol)
{
    weechat::account *ptr_account = NULL;
    weechat::channel *ptr_channel = NULL;
    int days;

    (void) pointer;
    (void) data;
    (void) argv_eol;

    buffer__get_account_and_channel(buffer, &ptr_account, &ptr_channel);

    if (!ptr_account)
        return WEECHAT_RC_ERROR;

    if (!ptr_channel)
    {
        weechat_printf(
            ptr_account->buffer,
            _("%s%s: \"%s\" command can not be executed on a account buffer"),
            weechat_prefix("error"), WEECHAT_XMPP_PLUGIN_NAME, "mam");
        return WEECHAT_RC_OK;
    }

    time_t start, end;
    if (argc > 1)
    {
        errno = 0;
        days = strtol(argv[1], NULL, 10);

        if (errno != 0)
        {
            weechat_printf(
                ptr_channel->buffer,
                _("%s%s: \"%s\" is not a valid number of %s for %s"),
                weechat_prefix("error"), WEECHAT_XMPP_PLUGIN_NAME, argv[1], "days", "mam");
            days = MAM_DEFAULT_DAYS;
        }
    }
    else
        days = MAM_DEFAULT_DAYS;
    
    // Calculate time range: (now - days) to now
    end = time(NULL);
    start = end - (days * 24 * 60 * 60);
    
    xmpp_string_guard mam_uuid_g(ptr_account->context, xmpp_uuid_gen(ptr_account->context));
    const char *mam_uuid = mam_uuid_g.ptr;
    ptr_channel->fetch_mam(mam_uuid, &start, &end, NULL);
    // freed by mam_uuid_g

    return WEECHAT_RC_OK;
}

