int av_opt_set_dict2(void *obj, AVDictionary **options, int search_flags)

{

    AVDictionaryEntry *t = NULL;

    AVDictionary    *tmp = NULL;

    int ret = 0;



    if (!options)

        return 0;



    while ((t = av_dict_get(*options, "", t, AV_DICT_IGNORE_SUFFIX))) {

        ret = av_opt_set(obj, t->key, t->value, search_flags);

        if (ret == AVERROR_OPTION_NOT_FOUND)

            av_dict_set(&tmp, t->key, t->value, 0);

        else if (ret < 0) {

            av_log(obj, AV_LOG_ERROR, "Error setting option %s to value %s.\n", t->key, t->value);

            break;

        }

        ret = 0;

    }

    av_dict_free(options);

    *options = tmp;

    return ret;

}
