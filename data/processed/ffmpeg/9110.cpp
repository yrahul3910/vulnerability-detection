const AVOption *av_opt_next(void *obj, const AVOption *last)

{

    AVClass *class = *(AVClass**)obj;

    if (!last && class->option && class->option[0].name)

        return class->option;

    if (last && last[1].name)

        return ++last;

    return NULL;

}
