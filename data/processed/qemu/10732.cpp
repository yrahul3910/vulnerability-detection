void qemu_start_warp_timer(void)

{

    int64_t clock;

    int64_t deadline;



    if (!use_icount) {

        return;

    }



    /* Nothing to do if the VM is stopped: QEMU_CLOCK_VIRTUAL timers

     * do not fire, so computing the deadline does not make sense.

     */

    if (!runstate_is_running()) {

        return;

    }



    /* warp clock deterministically in record/replay mode */

    if (!replay_checkpoint(CHECKPOINT_CLOCK_WARP_START)) {

        return;

    }



    if (!all_cpu_threads_idle()) {

        return;

    }



    if (qtest_enabled()) {

        /* When testing, qtest commands advance icount.  */

        return;

    }



    /* We want to use the earliest deadline from ALL vm_clocks */

    clock = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL_RT);

    deadline = qemu_clock_deadline_ns_all(QEMU_CLOCK_VIRTUAL);

    if (deadline < 0) {

        static bool notified;

        if (!icount_sleep && !notified) {

            error_report("WARNING: icount sleep disabled and no active timers");

            notified = true;

        }

        return;

    }



    if (deadline > 0) {

        /*

         * Ensure QEMU_CLOCK_VIRTUAL proceeds even when the virtual CPU goes to

         * sleep.  Otherwise, the CPU might be waiting for a future timer

         * interrupt to wake it up, but the interrupt never comes because

         * the vCPU isn't running any insns and thus doesn't advance the

         * QEMU_CLOCK_VIRTUAL.

         */

        if (!icount_sleep) {

            /*

             * We never let VCPUs sleep in no sleep icount mode.

             * If there is a pending QEMU_CLOCK_VIRTUAL timer we just advance

             * to the next QEMU_CLOCK_VIRTUAL event and notify it.

             * It is useful when we want a deterministic execution time,

             * isolated from host latencies.

             */

            seqlock_write_begin(&timers_state.vm_clock_seqlock);

            timers_state.qemu_icount_bias += deadline;

            seqlock_write_end(&timers_state.vm_clock_seqlock);

            qemu_clock_notify(QEMU_CLOCK_VIRTUAL);

        } else {

            /*

             * We do stop VCPUs and only advance QEMU_CLOCK_VIRTUAL after some

             * "real" time, (related to the time left until the next event) has

             * passed. The QEMU_CLOCK_VIRTUAL_RT clock will do this.

             * This avoids that the warps are visible externally; for example,

             * you will not be sending network packets continuously instead of

             * every 100ms.

             */

            seqlock_write_begin(&timers_state.vm_clock_seqlock);

            if (vm_clock_warp_start == -1 || vm_clock_warp_start > clock) {

                vm_clock_warp_start = clock;

            }

            seqlock_write_end(&timers_state.vm_clock_seqlock);

            timer_mod_anticipate(icount_warp_timer, clock + deadline);

        }

    } else if (deadline == 0) {

        qemu_clock_notify(QEMU_CLOCK_VIRTUAL);

    }

}
