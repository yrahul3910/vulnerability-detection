static void sdt_cb(MpegTSFilter *filter, const uint8_t *section, int section_len)

{

    MpegTSContext *ts = filter->u.section_filter.opaque;

    SectionHeader h1, *h = &h1;

    const uint8_t *p, *p_end, *desc_list_end, *desc_end;

    int onid, val, sid, desc_list_len, desc_tag, desc_len, service_type;

    char *name, *provider_name;



    av_dlog(ts->stream, "SDT:\n");

    hex_dump_debug(ts->stream, section, section_len);



    p_end = section + section_len - 4;

    p     = section;

    if (parse_section_header(h, &p, p_end) < 0)

        return;

    if (h->tid != SDT_TID)

        return;

    if (ts->skip_changes)

        return;

    onid = get16(&p, p_end);

    if (onid < 0)

        return;

    val = get8(&p, p_end);

    if (val < 0)

        return;

    for (;;) {

        sid = get16(&p, p_end);

        if (sid < 0)

            break;

        val = get8(&p, p_end);

        if (val < 0)

            break;

        desc_list_len = get16(&p, p_end);

        if (desc_list_len < 0)

            break;

        desc_list_len &= 0xfff;

        desc_list_end  = p + desc_list_len;

        if (desc_list_end > p_end)

            break;

        for (;;) {

            desc_tag = get8(&p, desc_list_end);

            if (desc_tag < 0)

                break;

            desc_len = get8(&p, desc_list_end);

            desc_end = p + desc_len;

            if (desc_end > desc_list_end)

                break;



            av_dlog(ts->stream, "tag: 0x%02x len=%d\n",

                    desc_tag, desc_len);



            switch (desc_tag) {

            case 0x48:

                service_type = get8(&p, p_end);

                if (service_type < 0)

                    break;

                provider_name = getstr8(&p, p_end);

                if (!provider_name)

                    break;

                name = getstr8(&p, p_end);

                if (name) {

                    AVProgram *program = av_new_program(ts->stream, sid);

                    if (program) {

                        av_dict_set(&program->metadata, "service_name", name, 0);

                        av_dict_set(&program->metadata, "service_provider",

                                    provider_name, 0);

                    }

                }

                av_free(name);

                av_free(provider_name);

                break;

            default:

                break;

            }

            p = desc_end;

        }

        p = desc_list_end;

    }

}
