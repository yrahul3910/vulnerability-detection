void parse_options(int argc, char **argv, const OptionDef *options,

                   void (* parse_arg_function)(const char*))

{

    const char *opt, *arg;

    int optindex, handleoptions=1;

    const OptionDef *po;



    /* parse options */

    optindex = 1;

    while (optindex < argc) {

        opt = argv[optindex++];



        if (handleoptions && opt[0] == '-' && opt[1] != '\0') {

            int bool_val = 1;

            if (opt[1] == '-' && opt[2] == '\0') {

                handleoptions = 0;

                continue;

            }

            opt++;

            po= find_option(options, opt);

            if (!po->name && opt[0] == 'n' && opt[1] == 'o') {

                /* handle 'no' bool option */

                po = find_option(options, opt + 2);

                if (!(po->name && (po->flags & OPT_BOOL)))

                    goto unknown_opt;

                bool_val = 0;

            }

            if (!po->name)

                po= find_option(options, "default");

            if (!po->name) {

unknown_opt:

                fprintf(stderr, "%s: unrecognized option '%s'\n", argv[0], opt);

                exit(1);

            }

            arg = NULL;

            if (po->flags & HAS_ARG) {

                arg = argv[optindex++];

                if (!arg) {

                    fprintf(stderr, "%s: missing argument for option '%s'\n", argv[0], opt);

                    exit(1);

                }

            }

            if (po->flags & OPT_STRING) {

                char *str;

                str = av_strdup(arg);

                *po->u.str_arg = str;

            } else if (po->flags & OPT_BOOL) {

                *po->u.int_arg = bool_val;

            } else if (po->flags & OPT_INT) {

                *po->u.int_arg = parse_number_or_die(opt, arg, OPT_INT64, INT_MIN, INT_MAX);

            } else if (po->flags & OPT_INT64) {

                *po->u.int64_arg = parse_number_or_die(opt, arg, OPT_INT64, INT64_MIN, INT64_MAX);

            } else if (po->flags & OPT_FLOAT) {

                *po->u.float_arg = parse_number_or_die(opt, arg, OPT_FLOAT, -1.0/0.0, 1.0/0.0);

            } else if (po->flags & OPT_FUNC2) {

                if (po->u.func2_arg(opt, arg) < 0) {

                    fprintf(stderr, "%s: invalid value '%s' for option '%s'\n", argv[0], arg, opt);

                    exit(1);

                }

            } else {

                po->u.func_arg(arg);

            }

            if(po->flags & OPT_EXIT)

                exit(0);

        } else {

            if (parse_arg_function)

                parse_arg_function(opt);

        }

    }

}
