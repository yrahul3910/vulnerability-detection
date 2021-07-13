static void sd_lock_command(SDState *sd)

{

    int erase, lock, clr_pwd, set_pwd, pwd_len;

    erase = !!(sd->data[0] & 0x08);

    lock = sd->data[0] & 0x04;

    clr_pwd = sd->data[0] & 0x02;

    set_pwd = sd->data[0] & 0x01;



    if (sd->blk_len > 1)

        pwd_len = sd->data[1];

    else

        pwd_len = 0;



    if (erase) {

        if (!(sd->card_status & CARD_IS_LOCKED) || sd->blk_len > 1 ||

                        set_pwd || clr_pwd || lock || sd->wp_switch ||

                        (sd->csd[14] & 0x20)) {

            sd->card_status |= LOCK_UNLOCK_FAILED;

            return;

        }

        memset(sd->wp_groups, 0, sizeof(int) * (sd->size >>

                        (HWBLOCK_SHIFT + SECTOR_SHIFT + WPGROUP_SHIFT)));

        sd->csd[14] &= ~0x10;

        sd->card_status &= ~CARD_IS_LOCKED;

        sd->pwd_len = 0;

        /* Erasing the entire card here! */

        fprintf(stderr, "SD: Card force-erased by CMD42\n");

        return;

    }



    if (sd->blk_len < 2 + pwd_len ||

                    pwd_len <= sd->pwd_len ||

                    pwd_len > sd->pwd_len + 16) {

        sd->card_status |= LOCK_UNLOCK_FAILED;

        return;

    }



    if (sd->pwd_len && memcmp(sd->pwd, sd->data + 2, sd->pwd_len)) {

        sd->card_status |= LOCK_UNLOCK_FAILED;

        return;

    }



    pwd_len -= sd->pwd_len;

    if ((pwd_len && !set_pwd) ||

                    (clr_pwd && (set_pwd || lock)) ||

                    (lock && !sd->pwd_len && !set_pwd) ||

                    (!set_pwd && !clr_pwd &&

                     (((sd->card_status & CARD_IS_LOCKED) && lock) ||

                      (!(sd->card_status & CARD_IS_LOCKED) && !lock)))) {

        sd->card_status |= LOCK_UNLOCK_FAILED;

        return;

    }



    if (set_pwd) {

        memcpy(sd->pwd, sd->data + 2 + sd->pwd_len, pwd_len);

        sd->pwd_len = pwd_len;

    }



    if (clr_pwd) {

        sd->pwd_len = 0;

    }



    if (lock)

        sd->card_status |= CARD_IS_LOCKED;

    else

        sd->card_status &= ~CARD_IS_LOCKED;

}
