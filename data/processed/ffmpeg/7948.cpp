static int set_string_number(void *obj, const AVOption *o, const char *val, void *dst)

{

    int ret = 0, notfirst = 0;

    for (;;) {

        int i, den = 1;

        char buf[256];

        int cmd = 0;

        double d, num = 1;

        int64_t intnum = 1;



        if (*val == '+' || *val == '-')

            cmd = *(val++);



        for (i = 0; i < sizeof(buf) - 1 && val[i] && val[i] != '+' && val[i] != '-'; i++)

            buf[i] = val[i];

        buf[i] = 0;



        {

            const AVOption *o_named = av_opt_find(obj, buf, o->unit, 0, 0);

            if (o_named && o_named->type == AV_OPT_TYPE_CONST)

                d = DEFAULT_NUMVAL(o_named);

            else if (!strcmp(buf, "default")) d = DEFAULT_NUMVAL(o);

            else if (!strcmp(buf, "max"    )) d = o->max;

            else if (!strcmp(buf, "min"    )) d = o->min;

            else if (!strcmp(buf, "none"   )) d = 0;

            else if (!strcmp(buf, "all"    )) d = ~0;

            else {

                int res = av_expr_parse_and_eval(&d, buf, const_names, const_values, NULL, NULL, NULL, NULL, NULL, 0, obj);

                if (res < 0) {

                    av_log(obj, AV_LOG_ERROR, "Unable to parse option value \"%s\"\n", val);

                    return res;

                }

            }

        }

        if (o->type == AV_OPT_TYPE_FLAGS) {

            read_number(o, dst, NULL, NULL, &intnum);

            if      (cmd == '+') d = intnum | (int64_t)d;

            else if (cmd == '-') d = intnum &~(int64_t)d;

        } else {

            read_number(o, dst, &num, &den, &intnum);

            if      (cmd == '+') d = notfirst*num*intnum/den + d;

            else if (cmd == '-') d = notfirst*num*intnum/den - d;

        }



        if ((ret = write_number(obj, o, dst, d, 1, 1)) < 0)

            return ret;

        val += i;

        if (!*val)

            return 0;

        notfirst = 1;

    }



    return 0;

}
