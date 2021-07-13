static int getopt(int argc, char *argv[], char *opts)

{

    static int sp = 1;

    int c;

    char *cp;



    if (sp == 1)

        if (optind >= argc ||

            argv[optind][0] != '-' || argv[optind][1] == '\0')

            return EOF;

        else if (!strcmp(argv[optind], "--")) {

            optind++;

            return EOF;

        }

    optopt = c = argv[optind][sp];

    if (c == ':' || (cp = strchr(opts, c)) == NULL) {

        fprintf(stderr, ": illegal option -- %c\n", c);

        if (argv[optind][++sp] == '\0') {

            optind++;

            sp = 1;

        }

        return '?';

    }

    if (*++cp == ':') {

        if (argv[optind][sp+1] != '\0')

            optarg = &argv[optind++][sp+1];

        else if(++optind >= argc) {

            fprintf(stderr, ": option requires an argument -- %c\n", c);

            sp = 1;

            return '?';

        } else

            optarg = argv[optind++];

        sp = 1;

    } else {

        if (argv[optind][++sp] == '\0') {

            sp = 1;

            optind++;

        }

        optarg = NULL;

    }



    return c;

}
