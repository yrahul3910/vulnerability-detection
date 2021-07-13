static int kvm_get_sregs(CPUState *env)

{

    struct kvm_sregs sregs;

    uint32_t hflags;

    int bit, i, ret;



    ret = kvm_vcpu_ioctl(env, KVM_GET_SREGS, &sregs);

    if (ret < 0)

        return ret;



    /* There can only be one pending IRQ set in the bitmap at a time, so try

       to find it and save its number instead (-1 for none). */

    env->interrupt_injected = -1;

    for (i = 0; i < ARRAY_SIZE(sregs.interrupt_bitmap); i++) {

        if (sregs.interrupt_bitmap[i]) {

            bit = ctz64(sregs.interrupt_bitmap[i]);

            env->interrupt_injected = i * 64 + bit;

            break;

        }

    }



    get_seg(&env->segs[R_CS], &sregs.cs);

    get_seg(&env->segs[R_DS], &sregs.ds);

    get_seg(&env->segs[R_ES], &sregs.es);

    get_seg(&env->segs[R_FS], &sregs.fs);

    get_seg(&env->segs[R_GS], &sregs.gs);

    get_seg(&env->segs[R_SS], &sregs.ss);



    get_seg(&env->tr, &sregs.tr);

    get_seg(&env->ldt, &sregs.ldt);



    env->idt.limit = sregs.idt.limit;

    env->idt.base = sregs.idt.base;

    env->gdt.limit = sregs.gdt.limit;

    env->gdt.base = sregs.gdt.base;



    env->cr[0] = sregs.cr0;

    env->cr[2] = sregs.cr2;

    env->cr[3] = sregs.cr3;

    env->cr[4] = sregs.cr4;



    cpu_set_apic_base(env->apic_state, sregs.apic_base);



    env->efer = sregs.efer;

    //cpu_set_apic_tpr(env->apic_state, sregs.cr8);



#define HFLAG_COPY_MASK ~( \

			HF_CPL_MASK | HF_PE_MASK | HF_MP_MASK | HF_EM_MASK | \

			HF_TS_MASK | HF_TF_MASK | HF_VM_MASK | HF_IOPL_MASK | \

			HF_OSFXSR_MASK | HF_LMA_MASK | HF_CS32_MASK | \

			HF_SS32_MASK | HF_CS64_MASK | HF_ADDSEG_MASK)







    hflags = (env->segs[R_CS].flags >> DESC_DPL_SHIFT) & HF_CPL_MASK;

    hflags |= (env->cr[0] & CR0_PE_MASK) << (HF_PE_SHIFT - CR0_PE_SHIFT);

    hflags |= (env->cr[0] << (HF_MP_SHIFT - CR0_MP_SHIFT)) &

	    (HF_MP_MASK | HF_EM_MASK | HF_TS_MASK);

    hflags |= (env->eflags & (HF_TF_MASK | HF_VM_MASK | HF_IOPL_MASK));

    hflags |= (env->cr[4] & CR4_OSFXSR_MASK) <<

	    (HF_OSFXSR_SHIFT - CR4_OSFXSR_SHIFT);



    if (env->efer & MSR_EFER_LMA) {

        hflags |= HF_LMA_MASK;

    }



    if ((hflags & HF_LMA_MASK) && (env->segs[R_CS].flags & DESC_L_MASK)) {

        hflags |= HF_CS32_MASK | HF_SS32_MASK | HF_CS64_MASK;

    } else {

        hflags |= (env->segs[R_CS].flags & DESC_B_MASK) >>

		(DESC_B_SHIFT - HF_CS32_SHIFT);

        hflags |= (env->segs[R_SS].flags & DESC_B_MASK) >>

		(DESC_B_SHIFT - HF_SS32_SHIFT);

        if (!(env->cr[0] & CR0_PE_MASK) ||

                   (env->eflags & VM_MASK) ||

                   !(hflags & HF_CS32_MASK)) {

                hflags |= HF_ADDSEG_MASK;

            } else {

                hflags |= ((env->segs[R_DS].base |

                                env->segs[R_ES].base |

                                env->segs[R_SS].base) != 0) <<

                    HF_ADDSEG_SHIFT;

            }

    }

    env->hflags = (env->hflags & HFLAG_COPY_MASK) | hflags;



    return 0;

}
