static int parse_uint16(DeviceState *dev, Property *prop, const char *str)

{

    uint16_t *ptr = qdev_get_prop_ptr(dev, prop);

    const char *fmt;



    /* accept both hex and decimal */

    fmt = strncasecmp(str, "0x",2) == 0 ? "%" PRIx16 : "%" PRIu16;

    if (sscanf(str, fmt, ptr) != 1)

        return -EINVAL;

    return 0;

}
