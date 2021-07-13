static void glib_pollfds_fill(uint32_t *cur_timeout)

{

    GMainContext *context = g_main_context_default();

    int timeout = 0;

    int n;



    g_main_context_prepare(context, &max_priority);



    glib_pollfds_idx = gpollfds->len;

    n = glib_n_poll_fds;

    do {

        GPollFD *pfds;

        glib_n_poll_fds = n;

        g_array_set_size(gpollfds, glib_pollfds_idx + glib_n_poll_fds);

        pfds = &g_array_index(gpollfds, GPollFD, glib_pollfds_idx);

        n = g_main_context_query(context, max_priority, &timeout, pfds,

                                 glib_n_poll_fds);

    } while (n != glib_n_poll_fds);



    if (timeout >= 0 && timeout < *cur_timeout) {

        *cur_timeout = timeout;

    }

}
