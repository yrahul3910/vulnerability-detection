uint32_t kvmppc_get_dfp(void)

{

    return kvmppc_read_int_cpu_dt("ibm,dfp");

}
