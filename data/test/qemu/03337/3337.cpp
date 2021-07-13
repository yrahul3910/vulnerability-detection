static void vtd_handle_gcmd_qie(IntelIOMMUState *s, bool en)

{

    uint64_t iqa_val = vtd_get_quad_raw(s, DMAR_IQA_REG);



    trace_vtd_inv_qi_enable(en);



    if (en) {

        s->iq = iqa_val & VTD_IQA_IQA_MASK(VTD_HOST_ADDRESS_WIDTH);

        /* 2^(x+8) entries */

        s->iq_size = 1UL << ((iqa_val & VTD_IQA_QS) + 8);

        s->qi_enabled = true;

        trace_vtd_inv_qi_setup(s->iq, s->iq_size);

        /* Ok - report back to driver */

        vtd_set_clear_mask_long(s, DMAR_GSTS_REG, 0, VTD_GSTS_QIES);



        if (s->iq_tail != 0) {

            /*

             * This is a spec violation but Windows guests are known to set up

             * Queued Invalidation this way so we allow the write and process

             * Invalidation Descriptors right away.

             */

            trace_vtd_warn_invalid_qi_tail(s->iq_tail);

            if (!(vtd_get_long_raw(s, DMAR_FSTS_REG) & VTD_FSTS_IQE)) {

                vtd_fetch_inv_desc(s);

            }

        }

    } else {

        if (vtd_queued_inv_disable_check(s)) {

            /* disable Queued Invalidation */

            vtd_set_quad_raw(s, DMAR_IQH_REG, 0);

            s->iq_head = 0;

            s->qi_enabled = false;

            /* Ok - report back to driver */

            vtd_set_clear_mask_long(s, DMAR_GSTS_REG, VTD_GSTS_QIES, 0);

        } else {

            trace_vtd_err_qi_disable(s->iq_head, s->iq_tail, s->iq_last_desc_type);

        }

    }

}
