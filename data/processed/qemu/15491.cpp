static void cpu_common_reset(CPUState *cpu)

{

    CPUClass *cc = CPU_GET_CLASS(cpu);



    if (qemu_loglevel_mask(CPU_LOG_RESET)) {

        qemu_log("CPU Reset (CPU %d)\n", cpu->cpu_index);

        log_cpu_state(cpu, cc->reset_dump_flags);

    }



    cpu->interrupt_request = 0;

    cpu->halted = 0;

    cpu->mem_io_pc = 0;

    cpu->mem_io_vaddr = 0;

    cpu->icount_extra = 0;

    cpu->icount_decr.u32 = 0;

    cpu->can_do_io = 1;

    cpu->exception_index = -1;

    cpu->crash_occurred = false;



    if (tcg_enabled()) {

        cpu_tb_jmp_cache_clear(cpu);



#ifdef CONFIG_SOFTMMU

        tlb_flush(cpu, 0);

#endif

    }

}
