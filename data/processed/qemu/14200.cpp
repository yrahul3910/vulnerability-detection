static int ahci_populate_sglist(AHCIDevice *ad, QEMUSGList *sglist,

                                AHCICmdHdr *cmd, int64_t limit, int32_t offset)

{

    uint16_t opts = le16_to_cpu(cmd->opts);

    uint16_t prdtl = le16_to_cpu(cmd->prdtl);

    uint64_t cfis_addr = le64_to_cpu(cmd->tbl_addr);

    uint64_t prdt_addr = cfis_addr + 0x80;

    dma_addr_t prdt_len = (prdtl * sizeof(AHCI_SG));

    dma_addr_t real_prdt_len = prdt_len;

    uint8_t *prdt;

    int i;

    int r = 0;

    uint64_t sum = 0;

    int off_idx = -1;

    int64_t off_pos = -1;

    int tbl_entry_size;

    IDEBus *bus = &ad->port;

    BusState *qbus = BUS(bus);



    /*

     * Note: AHCI PRDT can describe up to 256GiB. SATA/ATA only support

     * transactions of up to 32MiB as of ATA8-ACS3 rev 1b, assuming a

     * 512 byte sector size. We limit the PRDT in this implementation to

     * a reasonably large 2GiB, which can accommodate the maximum transfer

     * request for sector sizes up to 32K.

     */



    if (!prdtl) {

        DPRINTF(ad->port_no, "no sg list given by guest: 0x%08x\n", opts);

        return -1;

    }



    /* map PRDT */

    if (!(prdt = dma_memory_map(ad->hba->as, prdt_addr, &prdt_len,

                                DMA_DIRECTION_TO_DEVICE))){

        DPRINTF(ad->port_no, "map failed\n");

        return -1;

    }



    if (prdt_len < real_prdt_len) {

        DPRINTF(ad->port_no, "mapped less than expected\n");

        r = -1;

        goto out;

    }



    /* Get entries in the PRDT, init a qemu sglist accordingly */

    if (prdtl > 0) {

        AHCI_SG *tbl = (AHCI_SG *)prdt;

        sum = 0;

        for (i = 0; i < prdtl; i++) {

            tbl_entry_size = prdt_tbl_entry_size(&tbl[i]);

            if (offset < (sum + tbl_entry_size)) {

                off_idx = i;

                off_pos = offset - sum;

                break;

            }

            sum += tbl_entry_size;

        }

        if ((off_idx == -1) || (off_pos < 0) || (off_pos > tbl_entry_size)) {

            DPRINTF(ad->port_no, "%s: Incorrect offset! "

                            "off_idx: %d, off_pos: %"PRId64"\n",

                            __func__, off_idx, off_pos);

            r = -1;

            goto out;

        }



        qemu_sglist_init(sglist, qbus->parent, (prdtl - off_idx),

                         ad->hba->as);

        qemu_sglist_add(sglist, le64_to_cpu(tbl[off_idx].addr) + off_pos,

                        MIN(prdt_tbl_entry_size(&tbl[off_idx]) - off_pos,

                            limit));



        for (i = off_idx + 1; i < prdtl && sglist->size < limit; i++) {

            qemu_sglist_add(sglist, le64_to_cpu(tbl[i].addr),

                            MIN(prdt_tbl_entry_size(&tbl[i]),

                                limit - sglist->size));

            if (sglist->size > INT32_MAX) {

                error_report("AHCI Physical Region Descriptor Table describes "

                             "more than 2 GiB.");

                qemu_sglist_destroy(sglist);

                r = -1;

                goto out;

            }

        }

    }



out:

    dma_memory_unmap(ad->hba->as, prdt, prdt_len,

                     DMA_DIRECTION_TO_DEVICE, prdt_len);

    return r;

}
