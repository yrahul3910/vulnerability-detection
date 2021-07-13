static uint32_t omap_sysctl_read(void *opaque, target_phys_addr_t addr)

{

    struct omap_sysctl_s *s = (struct omap_sysctl_s *) opaque;



    switch (addr) {

    case 0x000:	/* CONTROL_REVISION */

        return 0x20;



    case 0x010:	/* CONTROL_SYSCONFIG */

        return s->sysconfig;



    case 0x030 ... 0x140:	/* CONTROL_PADCONF - only used in the POP */

        return s->padconf[(addr - 0x30) >> 2];



    case 0x270:	/* CONTROL_DEBOBS */

        return s->obs;



    case 0x274:	/* CONTROL_DEVCONF */

        return s->devconfig;



    case 0x28c:	/* CONTROL_EMU_SUPPORT */

        return 0;



    case 0x290:	/* CONTROL_MSUSPENDMUX_0 */

        return s->msuspendmux[0];

    case 0x294:	/* CONTROL_MSUSPENDMUX_1 */

        return s->msuspendmux[1];

    case 0x298:	/* CONTROL_MSUSPENDMUX_2 */

        return s->msuspendmux[2];

    case 0x29c:	/* CONTROL_MSUSPENDMUX_3 */

        return s->msuspendmux[3];

    case 0x2a0:	/* CONTROL_MSUSPENDMUX_4 */

        return s->msuspendmux[4];

    case 0x2a4:	/* CONTROL_MSUSPENDMUX_5 */

        return 0;



    case 0x2b8:	/* CONTROL_PSA_CTRL */

        return s->psaconfig;

    case 0x2bc:	/* CONTROL_PSA_CMD */

    case 0x2c0:	/* CONTROL_PSA_VALUE */

        return 0;



    case 0x2b0:	/* CONTROL_SEC_CTRL */

        return 0x800000f1;

    case 0x2d0:	/* CONTROL_SEC_EMU */

        return 0x80000015;

    case 0x2d4:	/* CONTROL_SEC_TAP */

        return 0x8000007f;

    case 0x2b4:	/* CONTROL_SEC_TEST */

    case 0x2f0:	/* CONTROL_SEC_STATUS */

    case 0x2f4:	/* CONTROL_SEC_ERR_STATUS */

        /* Secure mode is not present on general-pusrpose device.  Outside

         * secure mode these values cannot be read or written.  */

        return 0;



    case 0x2d8:	/* CONTROL_OCM_RAM_PERM */

        return 0xff;

    case 0x2dc:	/* CONTROL_OCM_PUB_RAM_ADD */

    case 0x2e0:	/* CONTROL_EXT_SEC_RAM_START_ADD */

    case 0x2e4:	/* CONTROL_EXT_SEC_RAM_STOP_ADD */

        /* No secure mode so no Extended Secure RAM present.  */

        return 0;



    case 0x2f8:	/* CONTROL_STATUS */

        /* Device Type => General-purpose */

        return 0x0300;

    case 0x2fc:	/* CONTROL_GENERAL_PURPOSE_STATUS */



    case 0x300:	/* CONTROL_RPUB_KEY_H_0 */

    case 0x304:	/* CONTROL_RPUB_KEY_H_1 */

    case 0x308:	/* CONTROL_RPUB_KEY_H_2 */

    case 0x30c:	/* CONTROL_RPUB_KEY_H_3 */

        return 0xdecafbad;



    case 0x310:	/* CONTROL_RAND_KEY_0 */

    case 0x314:	/* CONTROL_RAND_KEY_1 */

    case 0x318:	/* CONTROL_RAND_KEY_2 */

    case 0x31c:	/* CONTROL_RAND_KEY_3 */

    case 0x320:	/* CONTROL_CUST_KEY_0 */

    case 0x324:	/* CONTROL_CUST_KEY_1 */

    case 0x330:	/* CONTROL_TEST_KEY_0 */

    case 0x334:	/* CONTROL_TEST_KEY_1 */

    case 0x338:	/* CONTROL_TEST_KEY_2 */

    case 0x33c:	/* CONTROL_TEST_KEY_3 */

    case 0x340:	/* CONTROL_TEST_KEY_4 */

    case 0x344:	/* CONTROL_TEST_KEY_5 */

    case 0x348:	/* CONTROL_TEST_KEY_6 */

    case 0x34c:	/* CONTROL_TEST_KEY_7 */

    case 0x350:	/* CONTROL_TEST_KEY_8 */

    case 0x354:	/* CONTROL_TEST_KEY_9 */

        /* Can only be accessed in secure mode and when C_FieldAccEnable

         * bit is set in CONTROL_SEC_CTRL.

         * TODO: otherwise an interconnect access error is generated.  */

        return 0;

    }



    OMAP_BAD_REG(addr);

    return 0;

}
