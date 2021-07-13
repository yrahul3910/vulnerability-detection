void register_displaychangelistener(DisplayChangeListener *dcl)

{

    static DisplaySurface *dummy;

    QemuConsole *con;



    trace_displaychangelistener_register(dcl, dcl->ops->dpy_name);

    dcl->ds = get_alloc_displaystate();

    QLIST_INSERT_HEAD(&dcl->ds->listeners, dcl, next);

    gui_setup_refresh(dcl->ds);

    if (dcl->con) {

        dcl->con->dcls++;

        con = dcl->con;

    } else {

        con = active_console;

    }

    if (dcl->ops->dpy_gfx_switch) {

        if (con) {

            dcl->ops->dpy_gfx_switch(dcl, con->surface);

        } else {

            if (!dummy) {

                dummy = qemu_create_dummy_surface();

            }

            dcl->ops->dpy_gfx_switch(dcl, dummy);

        }

    }

}
