int ff_get_cpu_flags_ppc(void)

{

#if HAVE_ALTIVEC

#ifdef __AMIGAOS4__

    ULONG result = 0;

    extern struct ExecIFace *IExec;



    IExec->GetCPUInfoTags(GCIT_VectorUnit, &result, TAG_DONE);

    if (result == VECTORTYPE_ALTIVEC)

        return AV_CPU_FLAG_ALTIVEC;

    return 0;

#elif defined(__APPLE__) || defined(__OpenBSD__)

#ifdef __OpenBSD__

    int sels[2] = {CTL_MACHDEP, CPU_ALTIVEC};

#else

    int sels[2] = {CTL_HW, HW_VECTORUNIT};

#endif

    int has_vu = 0;

    size_t len = sizeof(has_vu);

    int err;



    err = sysctl(sels, 2, &has_vu, &len, NULL, 0);



    if (err == 0)

        return has_vu ? AV_CPU_FLAG_ALTIVEC : 0;

    return 0;

#elif defined(__linux__)

    // The linux kernel could have the altivec support disabled

    // even if the cpu has it.

    int i, ret = 0;

    int fd = open("/proc/self/auxv", O_RDONLY);

    unsigned long buf[64] = { 0 };

    ssize_t count;



    if (fd < 0)

        return 0;



    while ((count = read(fd, buf, sizeof(buf))) > 0) {

        for (i = 0; i < count / sizeof(*buf); i += 2) {

            if (buf[i] == AT_NULL)

                goto out;

            if (buf[i] == AT_HWCAP) {

                if (buf[i + 1] & PPC_FEATURE_HAS_ALTIVEC)

                    ret = AV_CPU_FLAG_ALTIVEC;

                goto out;

            }

        }

    }



out:

    close(fd);

    return ret;

#elif CONFIG_RUNTIME_CPUDETECT

    int proc_ver;

    // Support of mfspr PVR emulation added in Linux 2.6.17.

    __asm__ volatile("mfspr %0, 287" : "=r" (proc_ver));

    proc_ver >>= 16;

    if (proc_ver  & 0x8000 ||

        proc_ver == 0x000c ||

        proc_ver == 0x0039 || proc_ver == 0x003c ||

        proc_ver == 0x0044 || proc_ver == 0x0045 ||

        proc_ver == 0x0070)

        return AV_CPU_FLAG_ALTIVEC;

    return 0;

#else

    // Since we were compiled for AltiVec, just assume we have it

    // until someone comes up with a proper way (not involving signal hacks).

    return AV_CPU_FLAG_ALTIVEC;

#endif /* __AMIGAOS4__ */

#endif /* HAVE_ALTIVEC */

    return 0;

}
