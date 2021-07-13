static QemuOpts *opts_parse(QemuOptsList *list, const char *params,
                            int permit_abbrev, bool defaults)
{
    const char *firstname;
    char value[1024], *id = NULL;
    const char *p;
    QemuOpts *opts;
    Error *local_err = NULL;
    assert(!permit_abbrev || list->implied_opt_name);
    firstname = permit_abbrev ? list->implied_opt_name : NULL;
    if (strncmp(params, "id=", 3) == 0) {
        get_opt_value(value, sizeof(value), params+3);
        id = value;
    } else if ((p = strstr(params, ",id=")) != NULL) {
        get_opt_value(value, sizeof(value), p+4);
        id = value;
    }
    opts = qemu_opts_create(list, id, !defaults, &local_err);
    if (opts == NULL) {
        if (error_is_set(&local_err)) {
            qerror_report_err(local_err);
            error_free(local_err);
        }
        return NULL;
    }
    if (opts_do_parse(opts, params, firstname, defaults) != 0) {
        qemu_opts_del(opts);
        return NULL;
    }
    return opts;
}