static int get_channel_idx(char **map, int *ch, char delim, int max_ch)

{

    char *next = split(*map, delim);

    int len;

    int n = 0;

    if (!next && delim == '-')

        return AVERROR(EINVAL);

    if (!*map)

        return AVERROR(EINVAL);

    len = strlen(*map);

    sscanf(*map, "%d%n", ch, &n);

    if (n != len)

        return AVERROR(EINVAL);

    if (*ch < 0 || *ch > max_ch)

        return AVERROR(EINVAL);

    *map = next;

    return 0;

}
