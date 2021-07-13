target_ulong do_arm_semihosting(CPUARMState *env)

{

    ARMCPU *cpu = arm_env_get_cpu(env);

    CPUState *cs = CPU(cpu);

    target_ulong args;

    target_ulong arg0, arg1, arg2, arg3;

    char * s;

    int nr;

    uint32_t ret;

    uint32_t len;

#ifdef CONFIG_USER_ONLY

    TaskState *ts = cs->opaque;

#else

    CPUARMState *ts = env;

#endif



    if (is_a64(env)) {

        /* Note that the syscall number is in W0, not X0 */

        nr = env->xregs[0] & 0xffffffffU;

        args = env->xregs[1];

    } else {

        nr = env->regs[0];

        args = env->regs[1];

    }



    switch (nr) {

    case TARGET_SYS_OPEN:

        GET_ARG(0);

        GET_ARG(1);

        GET_ARG(2);

        s = lock_user_string(arg0);

        if (!s) {

            /* FIXME - should this error code be -TARGET_EFAULT ? */

            return (uint32_t)-1;

        }

        if (arg1 >= 12) {

            unlock_user(s, arg0, 0);

            return (uint32_t)-1;

        }

        if (strcmp(s, ":tt") == 0) {

            int result_fileno = arg1 < 4 ? STDIN_FILENO : STDOUT_FILENO;

            unlock_user(s, arg0, 0);

            return result_fileno;

        }

        if (use_gdb_syscalls()) {

            ret = arm_gdb_syscall(cpu, arm_semi_cb, "open,%s,%x,1a4", arg0,

                                  (int)arg2+1, gdb_open_modeflags[arg1]);

        } else {

            ret = set_swi_errno(ts, open(s, open_modeflags[arg1], 0644));

        }

        unlock_user(s, arg0, 0);

        return ret;

    case TARGET_SYS_CLOSE:

        GET_ARG(0);

        if (use_gdb_syscalls()) {

            return arm_gdb_syscall(cpu, arm_semi_cb, "close,%x", arg0);

        } else {

            return set_swi_errno(ts, close(arg0));

        }

    case TARGET_SYS_WRITEC:

        {

          char c;



          if (get_user_u8(c, args))

              /* FIXME - should this error code be -TARGET_EFAULT ? */

              return (uint32_t)-1;

          /* Write to debug console.  stderr is near enough.  */

          if (use_gdb_syscalls()) {

                return arm_gdb_syscall(cpu, arm_semi_cb, "write,2,%x,1", args);

          } else {

                return write(STDERR_FILENO, &c, 1);

          }

        }

    case TARGET_SYS_WRITE0:

        if (!(s = lock_user_string(args)))

            /* FIXME - should this error code be -TARGET_EFAULT ? */

            return (uint32_t)-1;

        len = strlen(s);

        if (use_gdb_syscalls()) {

            return arm_gdb_syscall(cpu, arm_semi_cb, "write,2,%x,%x",

                                   args, len);

        } else {

            ret = write(STDERR_FILENO, s, len);

        }

        unlock_user(s, args, 0);

        return ret;

    case TARGET_SYS_WRITE:

        GET_ARG(0);

        GET_ARG(1);

        GET_ARG(2);

        len = arg2;

        if (use_gdb_syscalls()) {

            arm_semi_syscall_len = len;

            return arm_gdb_syscall(cpu, arm_semi_cb, "write,%x,%x,%x",

                                   arg0, arg1, len);

        } else {

            s = lock_user(VERIFY_READ, arg1, len, 1);

            if (!s) {

                /* FIXME - should this error code be -TARGET_EFAULT ? */

                return (uint32_t)-1;

            }

            ret = set_swi_errno(ts, write(arg0, s, len));

            unlock_user(s, arg1, 0);

            if (ret == (uint32_t)-1)

                return -1;

            return len - ret;

        }

    case TARGET_SYS_READ:

        GET_ARG(0);

        GET_ARG(1);

        GET_ARG(2);

        len = arg2;

        if (use_gdb_syscalls()) {

            arm_semi_syscall_len = len;

            return arm_gdb_syscall(cpu, arm_semi_cb, "read,%x,%x,%x",

                                   arg0, arg1, len);

        } else {

            s = lock_user(VERIFY_WRITE, arg1, len, 0);

            if (!s) {

                /* FIXME - should this error code be -TARGET_EFAULT ? */

                return (uint32_t)-1;

            }

            do {

                ret = set_swi_errno(ts, read(arg0, s, len));

            } while (ret == -1 && errno == EINTR);

            unlock_user(s, arg1, len);

            if (ret == (uint32_t)-1)

                return -1;

            return len - ret;

        }

    case TARGET_SYS_READC:

       /* XXX: Read from debug console. Not implemented.  */

        return 0;

    case TARGET_SYS_ISTTY:

        GET_ARG(0);

        if (use_gdb_syscalls()) {

            return arm_gdb_syscall(cpu, arm_semi_cb, "isatty,%x", arg0);

        } else {

            return isatty(arg0);

        }

    case TARGET_SYS_SEEK:

        GET_ARG(0);

        GET_ARG(1);

        if (use_gdb_syscalls()) {

            return arm_gdb_syscall(cpu, arm_semi_cb, "lseek,%x,%x,0",

                                   arg0, arg1);

        } else {

            ret = set_swi_errno(ts, lseek(arg0, arg1, SEEK_SET));

            if (ret == (uint32_t)-1)

              return -1;

            return 0;

        }

    case TARGET_SYS_FLEN:

        GET_ARG(0);

        if (use_gdb_syscalls()) {

            return arm_gdb_syscall(cpu, arm_semi_flen_cb, "fstat,%x,%x",

                                   arg0, arm_flen_buf(cpu));

        } else {

            struct stat buf;

            ret = set_swi_errno(ts, fstat(arg0, &buf));

            if (ret == (uint32_t)-1)

                return -1;

            return buf.st_size;

        }

    case TARGET_SYS_TMPNAM:

        /* XXX: Not implemented.  */

        return -1;

    case TARGET_SYS_REMOVE:

        GET_ARG(0);

        GET_ARG(1);

        if (use_gdb_syscalls()) {

            ret = arm_gdb_syscall(cpu, arm_semi_cb, "unlink,%s",

                                  arg0, (int)arg1+1);

        } else {

            s = lock_user_string(arg0);

            if (!s) {

                /* FIXME - should this error code be -TARGET_EFAULT ? */

                return (uint32_t)-1;

            }

            ret =  set_swi_errno(ts, remove(s));

            unlock_user(s, arg0, 0);

        }

        return ret;

    case TARGET_SYS_RENAME:

        GET_ARG(0);

        GET_ARG(1);

        GET_ARG(2);

        GET_ARG(3);

        if (use_gdb_syscalls()) {

            return arm_gdb_syscall(cpu, arm_semi_cb, "rename,%s,%s",

                                   arg0, (int)arg1+1, arg2, (int)arg3+1);

        } else {

            char *s2;

            s = lock_user_string(arg0);

            s2 = lock_user_string(arg2);

            if (!s || !s2)

                /* FIXME - should this error code be -TARGET_EFAULT ? */

                ret = (uint32_t)-1;

            else

                ret = set_swi_errno(ts, rename(s, s2));

            if (s2)

                unlock_user(s2, arg2, 0);

            if (s)

                unlock_user(s, arg0, 0);

            return ret;

        }

    case TARGET_SYS_CLOCK:

        return clock() / (CLOCKS_PER_SEC / 100);

    case TARGET_SYS_TIME:

        return set_swi_errno(ts, time(NULL));

    case TARGET_SYS_SYSTEM:

        GET_ARG(0);

        GET_ARG(1);

        if (use_gdb_syscalls()) {

            return arm_gdb_syscall(cpu, arm_semi_cb, "system,%s",

                                   arg0, (int)arg1+1);

        } else {

            s = lock_user_string(arg0);

            if (!s) {

                /* FIXME - should this error code be -TARGET_EFAULT ? */

                return (uint32_t)-1;

            }

            ret = set_swi_errno(ts, system(s));

            unlock_user(s, arg0, 0);

            return ret;

        }

    case TARGET_SYS_ERRNO:

#ifdef CONFIG_USER_ONLY

        return ts->swi_errno;

#else

        return syscall_err;

#endif

    case TARGET_SYS_GET_CMDLINE:

        {

            /* Build a command-line from the original argv.

             *

             * The inputs are:

             *     * arg0, pointer to a buffer of at least the size

             *               specified in arg1.

             *     * arg1, size of the buffer pointed to by arg0 in

             *               bytes.

             *

             * The outputs are:

             *     * arg0, pointer to null-terminated string of the

             *               command line.

             *     * arg1, length of the string pointed to by arg0.

             */



            char *output_buffer;

            size_t input_size;

            size_t output_size;

            int status = 0;

#if !defined(CONFIG_USER_ONLY)

            const char *cmdline;

#endif

            GET_ARG(0);

            GET_ARG(1);

            input_size = arg1;

            /* Compute the size of the output string.  */

#if !defined(CONFIG_USER_ONLY)

            cmdline = semihosting_get_cmdline();

            if (cmdline == NULL) {

                cmdline = ""; /* Default to an empty line. */

            }

            output_size = strlen(cmdline) + 1; /* Count terminating 0. */

#else

            unsigned int i;



            output_size = ts->info->arg_end - ts->info->arg_start;

            if (!output_size) {

                /* We special-case the "empty command line" case (argc==0).

                   Just provide the terminating 0. */

                output_size = 1;

            }

#endif



            if (output_size > input_size) {

                 /* Not enough space to store command-line arguments.  */

                return -1;

            }



            /* Adjust the command-line length.  */

            if (SET_ARG(1, output_size - 1)) {

                /* Couldn't write back to argument block */

                return -1;

            }



            /* Lock the buffer on the ARM side.  */

            output_buffer = lock_user(VERIFY_WRITE, arg0, output_size, 0);

            if (!output_buffer) {

                return -1;

            }



            /* Copy the command-line arguments.  */

#if !defined(CONFIG_USER_ONLY)

            pstrcpy(output_buffer, output_size, cmdline);

#else

            if (output_size == 1) {

                /* Empty command-line.  */

                output_buffer[0] = '\0';

                goto out;

            }



            if (copy_from_user(output_buffer, ts->info->arg_start,

                               output_size)) {

                status = -1;

                goto out;

            }



            /* Separate arguments by white spaces.  */

            for (i = 0; i < output_size - 1; i++) {

                if (output_buffer[i] == 0) {

                    output_buffer[i] = ' ';

                }

            }

        out:

#endif

            /* Unlock the buffer on the ARM side.  */

            unlock_user(output_buffer, arg0, output_size);



            return status;

        }

    case TARGET_SYS_HEAPINFO:

        {

            target_ulong retvals[4];

            uint32_t limit;

            int i;



            GET_ARG(0);



#ifdef CONFIG_USER_ONLY

            /* Some C libraries assume the heap immediately follows .bss, so

               allocate it using sbrk.  */

            if (!ts->heap_limit) {

                abi_ulong ret;



                ts->heap_base = do_brk(0);

                limit = ts->heap_base + ARM_ANGEL_HEAP_SIZE;

                /* Try a big heap, and reduce the size if that fails.  */

                for (;;) {

                    ret = do_brk(limit);

                    if (ret >= limit) {

                        break;

                    }

                    limit = (ts->heap_base >> 1) + (limit >> 1);

                }

                ts->heap_limit = limit;

            }



            retvals[0] = ts->heap_base;

            retvals[1] = ts->heap_limit;

            retvals[2] = ts->stack_base;

            retvals[3] = 0; /* Stack limit.  */

#else

            limit = ram_size;

            /* TODO: Make this use the limit of the loaded application.  */

            retvals[0] = limit / 2;

            retvals[1] = limit;

            retvals[2] = limit; /* Stack base */

            retvals[3] = 0; /* Stack limit.  */

#endif



            for (i = 0; i < ARRAY_SIZE(retvals); i++) {

                bool fail;



                if (is_a64(env)) {

                    fail = put_user_u64(retvals[i], arg0 + i * 8);

                } else {

                    fail = put_user_u32(retvals[i], arg0 + i * 4);

                }



                if (fail) {

                    /* Couldn't write back to argument block */

                    return -1;

                }

            }

            return 0;

        }

    case TARGET_SYS_EXIT:

        if (is_a64(env)) {

            /* The A64 version of this call takes a parameter block,

             * so the application-exit type can return a subcode which

             * is the exit status code from the application.

             */

            GET_ARG(0);

            GET_ARG(1);



            if (arg0 == ADP_Stopped_ApplicationExit) {

                ret = arg1;

            } else {

                ret = 1;

            }

        } else {

            /* ARM specifies only Stopped_ApplicationExit as normal

             * exit, everything else is considered an error */

            ret = (args == ADP_Stopped_ApplicationExit) ? 0 : 1;

        }

        gdb_exit(env, ret);

        exit(ret);

    case TARGET_SYS_SYNCCACHE:

        /* Clean the D-cache and invalidate the I-cache for the specified

         * virtual address range. This is a nop for us since we don't

         * implement caches. This is only present on A64.

         */

        if (is_a64(env)) {

            return 0;

        }

        /* fall through -- invalid for A32/T32 */

    default:

        fprintf(stderr, "qemu: Unsupported SemiHosting SWI 0x%02x\n", nr);

        cpu_dump_state(cs, stderr, fprintf, 0);

        abort();

    }

}
