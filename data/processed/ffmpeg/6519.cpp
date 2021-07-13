static int rtsp_parse_request(HTTPContext *c)

{

    const char *p, *p1, *p2;

    char cmd[32];

    char url[1024];

    char protocol[32];

    char line[1024];

    int len;

    RTSPMessageHeader header1 = { 0 }, *header = &header1;



    c->buffer_ptr[0] = '\0';

    p = c->buffer;



    get_word(cmd, sizeof(cmd), &p);

    get_word(url, sizeof(url), &p);

    get_word(protocol, sizeof(protocol), &p);



    av_strlcpy(c->method, cmd, sizeof(c->method));

    av_strlcpy(c->url, url, sizeof(c->url));

    av_strlcpy(c->protocol, protocol, sizeof(c->protocol));



    if (avio_open_dyn_buf(&c->pb) < 0) {

        /* XXX: cannot do more */

        c->pb = NULL; /* safety */

        return -1;

    }



    /* check version name */

    if (strcmp(protocol, "RTSP/1.0") != 0) {

        rtsp_reply_error(c, RTSP_STATUS_VERSION);

        goto the_end;

    }



    /* parse each header line */

    /* skip to next line */

    while (*p != '\n' && *p != '\0')

        p++;

    if (*p == '\n')

        p++;

    while (*p != '\0') {

        p1 = memchr(p, '\n', (char *)c->buffer_ptr - p);

        if (!p1)

            break;

        p2 = p1;

        if (p2 > p && p2[-1] == '\r')

            p2--;

        /* skip empty line */

        if (p2 == p)

            break;

        len = p2 - p;

        if (len > sizeof(line) - 1)

            len = sizeof(line) - 1;

        memcpy(line, p, len);

        line[len] = '\0';

        ff_rtsp_parse_line(header, line, NULL, NULL);

        p = p1 + 1;

    }



    /* handle sequence number */

    c->seq = header->seq;



    if (!strcmp(cmd, "DESCRIBE"))

        rtsp_cmd_describe(c, url);

    else if (!strcmp(cmd, "OPTIONS"))

        rtsp_cmd_options(c, url);

    else if (!strcmp(cmd, "SETUP"))

        rtsp_cmd_setup(c, url, header);

    else if (!strcmp(cmd, "PLAY"))

        rtsp_cmd_play(c, url, header);

    else if (!strcmp(cmd, "PAUSE"))

        rtsp_cmd_interrupt(c, url, header, 1);

    else if (!strcmp(cmd, "TEARDOWN"))

        rtsp_cmd_interrupt(c, url, header, 0);

    else

        rtsp_reply_error(c, RTSP_STATUS_METHOD);



 the_end:

    len = avio_close_dyn_buf(c->pb, &c->pb_buffer);

    c->pb = NULL; /* safety */

    if (len < 0) {

        /* XXX: cannot do more */

        return -1;

    }

    c->buffer_ptr = c->pb_buffer;

    c->buffer_end = c->pb_buffer + len;

    c->state = RTSPSTATE_SEND_REPLY;

    return 0;

}
