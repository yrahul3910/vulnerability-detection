static void pcnet_transmit(PCNetState *s)

{

    target_phys_addr_t xmit_cxda = 0;

    int count = CSR_XMTRL(s)-1;

    s->xmit_pos = -1;



    if (!CSR_TXON(s)) {

        s->csr[0] &= ~0x0008;

        return;

    }



    s->tx_busy = 1;



    txagain:

    if (pcnet_tdte_poll(s)) {

        struct pcnet_TMD tmd;



        TMDLOAD(&tmd, PHYSADDR(s,CSR_CXDA(s)));



#ifdef PCNET_DEBUG_TMD

        printf("  TMDLOAD 0x%08x\n", PHYSADDR(s,CSR_CXDA(s)));

        PRINT_TMD(&tmd);

#endif

        if (GET_FIELD(tmd.status, TMDS, STP)) {

            s->xmit_pos = 0;

            xmit_cxda = PHYSADDR(s,CSR_CXDA(s));

        }

        if (!GET_FIELD(tmd.status, TMDS, ENP)) {

            int bcnt = 4096 - GET_FIELD(tmd.length, TMDL, BCNT);

            s->phys_mem_read(s->dma_opaque, PHYSADDR(s, tmd.tbadr),

                             s->buffer + s->xmit_pos, bcnt, CSR_BSWP(s));

            s->xmit_pos += bcnt;

        } else if (s->xmit_pos >= 0) {

            int bcnt = 4096 - GET_FIELD(tmd.length, TMDL, BCNT);

            s->phys_mem_read(s->dma_opaque, PHYSADDR(s, tmd.tbadr),

                             s->buffer + s->xmit_pos, bcnt, CSR_BSWP(s));

            s->xmit_pos += bcnt;

#ifdef PCNET_DEBUG

            printf("pcnet_transmit size=%d\n", s->xmit_pos);

#endif

            if (CSR_LOOP(s))

                pcnet_receive(s, s->buffer, s->xmit_pos);

            else

                if (s->vc)

                    qemu_send_packet(s->vc, s->buffer, s->xmit_pos);



            s->csr[0] &= ~0x0008;   /* clear TDMD */

            s->csr[4] |= 0x0004;    /* set TXSTRT */

            s->xmit_pos = -1;

        }



        SET_FIELD(&tmd.status, TMDS, OWN, 0);

        TMDSTORE(&tmd, PHYSADDR(s,CSR_CXDA(s)));

        if (!CSR_TOKINTD(s) || (CSR_LTINTEN(s) && GET_FIELD(tmd.status, TMDS, LTINT)))

            s->csr[0] |= 0x0200;    /* set TINT */



        if (CSR_XMTRC(s)<=1)

            CSR_XMTRC(s) = CSR_XMTRL(s);

        else

            CSR_XMTRC(s)--;

        if (count--)

            goto txagain;



    } else

    if (s->xmit_pos >= 0) {

        struct pcnet_TMD tmd;

        TMDLOAD(&tmd, PHYSADDR(s,xmit_cxda));

        SET_FIELD(&tmd.misc, TMDM, BUFF, 1);

        SET_FIELD(&tmd.misc, TMDM, UFLO, 1);

        SET_FIELD(&tmd.status, TMDS, ERR, 1);

        SET_FIELD(&tmd.status, TMDS, OWN, 0);

        TMDSTORE(&tmd, PHYSADDR(s,xmit_cxda));

        s->csr[0] |= 0x0200;    /* set TINT */

        if (!CSR_DXSUFLO(s)) {

            s->csr[0] &= ~0x0010;

        } else

        if (count--)

          goto txagain;

    }



    s->tx_busy = 0;

}
