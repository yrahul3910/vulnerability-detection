static int mov_open_dref(AVIOContext **pb, const char *src, MOVDref *ref,

                         AVIOInterruptCB *int_cb, int use_absolute_path, AVFormatContext *fc)

{

    /* try relative path, we do not try the absolute because it can leak information about our

       system to an attacker */

    if (ref->nlvl_to > 0 && ref->nlvl_from > 0) {

        char filename[1024];

        const char *src_path;

        int i, l;



        /* find a source dir */

        src_path = strrchr(src, '/');

        if (src_path)

            src_path++;

        else

            src_path = src;



        /* find a next level down to target */

        for (i = 0, l = strlen(ref->path) - 1; l >= 0; l--)

            if (ref->path[l] == '/') {

                if (i == ref->nlvl_to - 1)

                    break;

                else

                    i++;

            }



        /* compose filename if next level down to target was found */

        if (i == ref->nlvl_to - 1 && src_path - src  < sizeof(filename)) {

            memcpy(filename, src, src_path - src);

            filename[src_path - src] = 0;



            for (i = 1; i < ref->nlvl_from; i++)

                av_strlcat(filename, "../", sizeof(filename));



            av_strlcat(filename, ref->path + l + 1, sizeof(filename));



            if (!avio_open2(pb, filename, AVIO_FLAG_READ, int_cb, NULL))

                return 0;

        }

    } else if (use_absolute_path) {

        av_log(fc, AV_LOG_WARNING, "Using absolute path on user request, "

               "this is a possible security issue\n");

        if (!avio_open2(pb, ref->path, AVIO_FLAG_READ, int_cb, NULL))

            return 0;

    }



    return AVERROR(ENOENT);

}
