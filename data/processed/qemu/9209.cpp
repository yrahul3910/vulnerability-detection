static void test_acpi_q35_tcg_cphp(void)

{

    test_data data;



    memset(&data, 0, sizeof(data));

    data.machine = MACHINE_Q35;

    data.variant = ".cphp";

    test_acpi_one(" -smp 2,cores=3,sockets=2,maxcpus=6"

                  " -numa node -numa node",

                  &data);

    free_test_data(&data);

}
