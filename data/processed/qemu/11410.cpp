static void xics_realize(DeviceState *dev, Error **errp)

{

    XICSState *icp = XICS(dev);

    ICSState *ics = icp->ics;

    Error *error = NULL;

    int i;



    if (!icp->nr_servers) {

        error_setg(errp, "Number of servers needs to be greater 0");

        return;

    }



    /* Registration of global state belongs into realize */

    spapr_rtas_register("ibm,set-xive", rtas_set_xive);

    spapr_rtas_register("ibm,get-xive", rtas_get_xive);

    spapr_rtas_register("ibm,int-off", rtas_int_off);

    spapr_rtas_register("ibm,int-on", rtas_int_on);



    spapr_register_hypercall(H_CPPR, h_cppr);

    spapr_register_hypercall(H_IPI, h_ipi);

    spapr_register_hypercall(H_XIRR, h_xirr);

    spapr_register_hypercall(H_EOI, h_eoi);



    ics->nr_irqs = icp->nr_irqs;

    ics->offset = XICS_IRQ_BASE;

    ics->icp = icp;

    object_property_set_bool(OBJECT(icp->ics), true, "realized", &error);

    if (error) {

        error_propagate(errp, error);

        return;

    }



    icp->ss = g_malloc0(icp->nr_servers*sizeof(ICPState));

    for (i = 0; i < icp->nr_servers; i++) {

        char buffer[32];

        object_initialize(&icp->ss[i], sizeof(icp->ss[i]), TYPE_ICP);

        snprintf(buffer, sizeof(buffer), "icp[%d]", i);

        object_property_add_child(OBJECT(icp), buffer, OBJECT(&icp->ss[i]), NULL);

        object_property_set_bool(OBJECT(&icp->ss[i]), true, "realized", &error);

        if (error) {

            error_propagate(errp, error);

            return;

        }

    }

}
