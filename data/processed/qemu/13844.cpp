void cpu_loop(CPUM68KState *env)

{

    CPUState *cs = CPU(m68k_env_get_cpu(env));

    int trapnr;

    unsigned int n;

    target_siginfo_t info;

    TaskState *ts = cs->opaque;



    for(;;) {

        cpu_exec_start(cs);

        trapnr = cpu_m68k_exec(cs);

        cpu_exec_end(cs);

        switch(trapnr) {

        case EXCP_ILLEGAL:

            {

                if (ts->sim_syscalls) {

                    uint16_t nr;

                    get_user_u16(nr, env->pc + 2);

                    env->pc += 4;

                    do_m68k_simcall(env, nr);

                } else {

                    goto do_sigill;

                }

            }

            break;

        case EXCP_HALT_INSN:

            /* Semihosing syscall.  */

            env->pc += 4;

            do_m68k_semihosting(env, env->dregs[0]);

            break;

        case EXCP_LINEA:

        case EXCP_LINEF:

        case EXCP_UNSUPPORTED:

        do_sigill:

            info.si_signo = TARGET_SIGILL;

            info.si_errno = 0;

            info.si_code = TARGET_ILL_ILLOPN;

            info._sifields._sigfault._addr = env->pc;

            queue_signal(env, info.si_signo, &info);

            break;

        case EXCP_TRAP0:

            {

                ts->sim_syscalls = 0;

                n = env->dregs[0];

                env->pc += 2;

                env->dregs[0] = do_syscall(env,

                                          n,

                                          env->dregs[1],

                                          env->dregs[2],

                                          env->dregs[3],

                                          env->dregs[4],

                                          env->dregs[5],

                                          env->aregs[0],

                                          0, 0);

            }

            break;

        case EXCP_INTERRUPT:

            /* just indicate that signals should be handled asap */

            break;

        case EXCP_ACCESS:

            {

                info.si_signo = TARGET_SIGSEGV;

                info.si_errno = 0;

                /* XXX: check env->error_code */

                info.si_code = TARGET_SEGV_MAPERR;

                info._sifields._sigfault._addr = env->mmu.ar;

                queue_signal(env, info.si_signo, &info);

            }

            break;

        case EXCP_DEBUG:

            {

                int sig;



                sig = gdb_handlesig(cs, TARGET_SIGTRAP);

                if (sig)

                  {

                    info.si_signo = sig;

                    info.si_errno = 0;

                    info.si_code = TARGET_TRAP_BRKPT;

                    queue_signal(env, info.si_signo, &info);

                  }

            }

            break;

        default:

            fprintf(stderr, "qemu: unhandled CPU exception 0x%x - aborting\n",

                    trapnr);

            cpu_dump_state(cs, stderr, fprintf, 0);

            abort();

        }

        process_pending_signals(env);

    }

}
