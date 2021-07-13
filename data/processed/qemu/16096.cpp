int vmstate_register_with_alias_id(DeviceState *dev, int instance_id,

                                   const VMStateDescription *vmsd,

                                   void *opaque, int alias_id,

                                   int required_for_version,

                                   Error **errp)

{

    SaveStateEntry *se;



    /* If this triggers, alias support can be dropped for the vmsd. */

    assert(alias_id == -1 || required_for_version >= vmsd->minimum_version_id);



    se = g_new0(SaveStateEntry, 1);

    se->version_id = vmsd->version_id;

    se->section_id = savevm_state.global_section_id++;

    se->opaque = opaque;

    se->vmsd = vmsd;

    se->alias_id = alias_id;



    if (dev) {

        char *id = qdev_get_dev_path(dev);

        if (id) {

            if (snprintf(se->idstr, sizeof(se->idstr), "%s/", id) >=

                sizeof(se->idstr)) {

                error_setg(errp, "Path too long for VMState (%s)", id);


                g_free(se);



                return -1;

            }




            se->compat = g_new0(CompatEntry, 1);

            pstrcpy(se->compat->idstr, sizeof(se->compat->idstr), vmsd->name);

            se->compat->instance_id = instance_id == -1 ?

                         calculate_compat_instance_id(vmsd->name) : instance_id;

            instance_id = -1;

        }

    }

    pstrcat(se->idstr, sizeof(se->idstr), vmsd->name);



    if (instance_id == -1) {

        se->instance_id = calculate_new_instance_id(se->idstr);

    } else {

        se->instance_id = instance_id;

    }

    assert(!se->compat || se->instance_id == 0);

    savevm_state_handler_insert(se);

    return 0;

}