static void object_set_link_property(Object *obj, Visitor *v, void *opaque,

                                     const char *name, Error **errp)

{

    Object **child = opaque;

    bool ambiguous = false;

    const char *type;

    char *path;



    type = object_property_get_type(obj, name, NULL);



    visit_type_str(v, &path, name, errp);



    if (*child) {

        object_unref(*child);

    }



    if (strcmp(path, "") != 0) {

        Object *target;



        target = object_resolve_path(path, &ambiguous);

        if (target) {

            gchar *target_type;



            target_type = g_strdup_printf("link<%s>",

                                          object_get_typename(OBJECT(target)));

            if (strcmp(target_type, type) == 0) {

                *child = target;

                object_ref(target);

            } else {

                error_set(errp, QERR_INVALID_PARAMETER_TYPE, name, type);

            }



            g_free(target_type);

        } else {

            error_set(errp, QERR_DEVICE_NOT_FOUND, path);

        }

    } else {

        *child = NULL;

    }



    g_free(path);

}
