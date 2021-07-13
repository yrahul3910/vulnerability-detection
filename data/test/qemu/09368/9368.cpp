static void spapr_add_lmbs(DeviceState *dev, uint64_t addr_start, uint64_t size,

                           uint32_t node, bool dedicated_hp_event_source,

                           Error **errp)

{

    sPAPRDRConnector *drc;

    sPAPRDRConnectorClass *drck;

    uint32_t nr_lmbs = size/SPAPR_MEMORY_BLOCK_SIZE;

    int i, fdt_offset, fdt_size;

    void *fdt;

    uint64_t addr = addr_start;



    for (i = 0; i < nr_lmbs; i++) {

        drc = spapr_dr_connector_by_id(SPAPR_DR_CONNECTOR_TYPE_LMB,

                addr/SPAPR_MEMORY_BLOCK_SIZE);

        g_assert(drc);



        fdt = create_device_tree(&fdt_size);

        fdt_offset = spapr_populate_memory_node(fdt, node, addr,

                                                SPAPR_MEMORY_BLOCK_SIZE);



        drck = SPAPR_DR_CONNECTOR_GET_CLASS(drc);

        drck->attach(drc, dev, fdt, fdt_offset, !dev->hotplugged, errp);

        addr += SPAPR_MEMORY_BLOCK_SIZE;







    /* send hotplug notification to the

     * guest only in case of hotplugged memory

     */

    if (dev->hotplugged) {

        if (dedicated_hp_event_source) {

            drc = spapr_dr_connector_by_id(SPAPR_DR_CONNECTOR_TYPE_LMB,

                    addr_start / SPAPR_MEMORY_BLOCK_SIZE);

            drck = SPAPR_DR_CONNECTOR_GET_CLASS(drc);

            spapr_hotplug_req_add_by_count_indexed(SPAPR_DR_CONNECTOR_TYPE_LMB,

                                                   nr_lmbs,

                                                   drck->get_index(drc));

        } else {

            spapr_hotplug_req_add_by_count(SPAPR_DR_CONNECTOR_TYPE_LMB,

                                           nr_lmbs);


