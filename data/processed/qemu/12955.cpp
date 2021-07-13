static void s390_memory_init(ram_addr_t mem_size)

{

    MemoryRegion *sysmem = get_system_memory();

    MemoryRegion *ram = g_new(MemoryRegion, 1);



    /* allocate RAM for core */

    memory_region_allocate_system_memory(ram, NULL, "s390.ram", mem_size);

    memory_region_add_subregion(sysmem, 0, ram);



    /* Initialize storage key device */

    s390_skeys_init();

    /* Initialize storage attributes device */

    s390_stattrib_init();

}
