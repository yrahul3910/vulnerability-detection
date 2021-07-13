static void sysctl_write(void *opaque, target_phys_addr_t addr, uint32_t value)

{

    MilkymistSysctlState *s = opaque;



    trace_milkymist_sysctl_memory_write(addr, value);



    addr >>= 2;

    switch (addr) {

    case R_GPIO_OUT:

    case R_GPIO_INTEN:

    case R_TIMER0_COUNTER:

        if (value > s->regs[R_TIMER0_COUNTER]) {

            value = s->regs[R_TIMER0_COUNTER];

            error_report("milkymist_sysctl: timer0: trying to write a "

                    "value greater than the limit. Clipping.");

        }

        /* milkymist timer counts up */

        value = s->regs[R_TIMER0_COUNTER] - value;

        ptimer_set_count(s->ptimer0, value);

        break;

    case R_TIMER1_COUNTER:

        if (value > s->regs[R_TIMER1_COUNTER]) {

            value = s->regs[R_TIMER1_COUNTER];

            error_report("milkymist_sysctl: timer1: trying to write a "

                    "value greater than the limit. Clipping.");

        }

        /* milkymist timer counts up */

        value = s->regs[R_TIMER1_COUNTER] - value;

        ptimer_set_count(s->ptimer1, value);

        break;

    case R_TIMER0_COMPARE:

        ptimer_set_limit(s->ptimer0, value, 0);

        s->regs[addr] = value;

        break;

    case R_TIMER1_COMPARE:

        ptimer_set_limit(s->ptimer1, value, 0);

        s->regs[addr] = value;

        break;

    case R_TIMER0_CONTROL:

        s->regs[addr] = value;

        if (s->regs[R_TIMER0_CONTROL] & CTRL_ENABLE) {

            trace_milkymist_sysctl_start_timer1();

            ptimer_run(s->ptimer0, 0);

        } else {

            trace_milkymist_sysctl_stop_timer1();

            ptimer_stop(s->ptimer0);

        }

        break;

    case R_TIMER1_CONTROL:

        s->regs[addr] = value;

        if (s->regs[R_TIMER1_CONTROL] & CTRL_ENABLE) {

            trace_milkymist_sysctl_start_timer1();

            ptimer_run(s->ptimer1, 0);

        } else {

            trace_milkymist_sysctl_stop_timer1();

            ptimer_stop(s->ptimer1);

        }

        break;

    case R_ICAP:

        sysctl_icap_write(s, value);

        break;

    case R_SYSTEM_ID:

        qemu_system_reset_request();

        break;



    case R_GPIO_IN:

    case R_CAPABILITIES:

        error_report("milkymist_sysctl: write to read-only register 0x"

                TARGET_FMT_plx, addr << 2);

        break;



    default:

        error_report("milkymist_sysctl: write access to unkown register 0x"

                TARGET_FMT_plx, addr << 2);

        break;

    }

}
