static void mtree_print_mr(fprintf_function mon_printf, void *f,

                           const MemoryRegion *mr, unsigned int level,

                           target_phys_addr_t base,

                           MemoryRegionListHead *alias_print_queue)

{

    MemoryRegionList *new_ml, *ml, *next_ml;

    MemoryRegionListHead submr_print_queue;

    const MemoryRegion *submr;

    unsigned int i;



    if (!mr) {

        return;

    }



    for (i = 0; i < level; i++) {

        mon_printf(f, "  ");

    }



    if (mr->alias) {

        MemoryRegionList *ml;

        bool found = false;



        /* check if the alias is already in the queue */

        QTAILQ_FOREACH(ml, alias_print_queue, queue) {

            if (ml->mr == mr->alias && !ml->printed) {

                found = true;

            }

        }



        if (!found) {

            ml = g_new(MemoryRegionList, 1);

            ml->mr = mr->alias;

            ml->printed = false;

            QTAILQ_INSERT_TAIL(alias_print_queue, ml, queue);

        }

        mon_printf(f, TARGET_FMT_plx "-" TARGET_FMT_plx " (prio %d): alias %s @%s "

                   TARGET_FMT_plx "-" TARGET_FMT_plx "\n",

                   base + mr->addr,

                   base + mr->addr

                   + (target_phys_addr_t)int128_get64(mr->size) - 1,

                   mr->priority,

                   mr->name,

                   mr->alias->name,

                   mr->alias_offset,

                   mr->alias_offset

                   + (target_phys_addr_t)int128_get64(mr->size) - 1);

    } else {

        mon_printf(f, TARGET_FMT_plx "-" TARGET_FMT_plx " (prio %d): %s\n",

                   base + mr->addr,

                   base + mr->addr

                   + (target_phys_addr_t)int128_get64(mr->size) - 1,

                   mr->priority,

                   mr->name);

    }



    QTAILQ_INIT(&submr_print_queue);



    QTAILQ_FOREACH(submr, &mr->subregions, subregions_link) {

        new_ml = g_new(MemoryRegionList, 1);

        new_ml->mr = submr;

        QTAILQ_FOREACH(ml, &submr_print_queue, queue) {

            if (new_ml->mr->addr < ml->mr->addr ||

                (new_ml->mr->addr == ml->mr->addr &&

                 new_ml->mr->priority > ml->mr->priority)) {

                QTAILQ_INSERT_BEFORE(ml, new_ml, queue);

                new_ml = NULL;

                break;

            }

        }

        if (new_ml) {

            QTAILQ_INSERT_TAIL(&submr_print_queue, new_ml, queue);

        }

    }



    QTAILQ_FOREACH(ml, &submr_print_queue, queue) {

        mtree_print_mr(mon_printf, f, ml->mr, level + 1, base + mr->addr,

                       alias_print_queue);

    }



    QTAILQ_FOREACH_SAFE(next_ml, &submr_print_queue, queue, ml) {

        g_free(ml);

    }

}
