int hw_device_init_from_string(const char *arg, HWDevice **dev_out)

{

    // "type=name:device,key=value,key2=value2"

    // "type:device,key=value,key2=value2"

    // -> av_hwdevice_ctx_create()

    // "type=name@name"

    // "type@name"

    // -> av_hwdevice_ctx_create_derived()



    AVDictionary *options = NULL;

    char *type_name = NULL, *name = NULL, *device = NULL;

    enum AVHWDeviceType type;

    HWDevice *dev, *src;

    AVBufferRef *device_ref = NULL;

    int err;

    const char *errmsg, *p, *q;

    size_t k;



    k = strcspn(arg, ":=@");

    p = arg + k;



    type_name = av_strndup(arg, k);

    if (!type_name) {

        err = AVERROR(ENOMEM);

        goto fail;

    }

    type = av_hwdevice_find_type_by_name(type_name);

    if (type == AV_HWDEVICE_TYPE_NONE) {

        errmsg = "unknown device type";

        goto invalid;

    }



    if (*p == '=') {

        k = strcspn(p + 1, ":@");



        name = av_strndup(p + 1, k);

        if (!name) {

            err = AVERROR(ENOMEM);

            goto fail;

        }

        if (hw_device_get_by_name(name)) {

            errmsg = "named device already exists";

            goto invalid;

        }



        p += 1 + k;

    } else {

        // Give the device an automatic name of the form "type%d".

        // We arbitrarily limit at 1000 anonymous devices of the same

        // type - there is probably something else very wrong if you

        // get to this limit.

        size_t index_pos;

        int index, index_limit = 1000;

        index_pos = strlen(type_name);

        name = av_malloc(index_pos + 4);

        if (!name) {

            err = AVERROR(ENOMEM);

            goto fail;

        }

        for (index = 0; index < index_limit; index++) {

            snprintf(name, index_pos + 4, "%s%d", type_name, index);

            if (!hw_device_get_by_name(name))

                break;

        }

        if (index >= index_limit) {

            errmsg = "too many devices";

            goto invalid;

        }

    }



    if (!*p) {

        // New device with no parameters.

        err = av_hwdevice_ctx_create(&device_ref, type,

                                     NULL, NULL, 0);

        if (err < 0)

            goto fail;



    } else if (*p == ':') {

        // New device with some parameters.

        ++p;

        q = strchr(p, ',');

        if (q) {

            device = av_strndup(p, q - p);

            if (!device) {

                err = AVERROR(ENOMEM);

                goto fail;

            }

            err = av_dict_parse_string(&options, q + 1, "=", ",", 0);

            if (err < 0) {

                errmsg = "failed to parse options";

                goto invalid;

            }

        }



        err = av_hwdevice_ctx_create(&device_ref, type,

                                     device ? device : p, options, 0);

        if (err < 0)

            goto fail;



    } else if (*p == '@') {

        // Derive from existing device.



        src = hw_device_get_by_name(p + 1);

        if (!src) {

            errmsg = "invalid source device name";

            goto invalid;

        }



        err = av_hwdevice_ctx_create_derived(&device_ref, type,

                                             src->device_ref, 0);

        if (err < 0)

            goto fail;

    } else {

        errmsg = "parse error";

        goto invalid;

    }



    dev = hw_device_add();

    if (!dev) {

        err = AVERROR(ENOMEM);

        goto fail;

    }



    dev->name = name;

    dev->type = type;

    dev->device_ref = device_ref;



    if (dev_out)

        *dev_out = dev;



    name = NULL;

    err = 0;

done:

    av_freep(&type_name);

    av_freep(&name);

    av_freep(&device);

    av_dict_free(&options);

    return err;

invalid:

    av_log(NULL, AV_LOG_ERROR,

           "Invalid device specification \"%s\": %s\n", arg, errmsg);

    err = AVERROR(EINVAL);

    goto done;

fail:

    av_log(NULL, AV_LOG_ERROR,

           "Device creation failed: %d.\n", err);

    av_buffer_unref(&device_ref);

    goto done;

}
