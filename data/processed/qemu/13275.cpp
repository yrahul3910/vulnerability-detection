static int write_elf32_load(DumpState *s, MemoryMapping *memory_mapping,

                            int phdr_index, target_phys_addr_t offset)

{

    Elf32_Phdr phdr;

    int ret;

    int endian = s->dump_info.d_endian;



    memset(&phdr, 0, sizeof(Elf32_Phdr));

    phdr.p_type = cpu_convert_to_target32(PT_LOAD, endian);

    phdr.p_offset = cpu_convert_to_target32(offset, endian);

    phdr.p_paddr = cpu_convert_to_target32(memory_mapping->phys_addr, endian);

    if (offset == -1) {

        /* When the memory is not stored into vmcore, offset will be -1 */

        phdr.p_filesz = 0;

    } else {

        phdr.p_filesz = cpu_convert_to_target32(memory_mapping->length, endian);

    }

    phdr.p_memsz = cpu_convert_to_target32(memory_mapping->length, endian);

    phdr.p_vaddr = cpu_convert_to_target32(memory_mapping->virt_addr, endian);



    ret = fd_write_vmcore(&phdr, sizeof(Elf32_Phdr), s);

    if (ret < 0) {

        dump_error(s, "dump: failed to write program header table.\n");

        return -1;

    }



    return 0;

}
