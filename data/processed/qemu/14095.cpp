static target_long monitor_get_tbl (const struct MonitorDef *md, int val)

{

    CPUState *env = mon_get_cpu();

    if (!env)

        return 0;

    return cpu_ppc_load_tbl(env);

}
