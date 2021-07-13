static void term_up_char(void)

{

    int idx;



    if (term_hist_entry == 0)

	return;

    if (term_hist_entry == -1) {

	/* Find latest entry */

	for (idx = 0; idx < TERM_MAX_CMDS; idx++) {

	    if (term_history[idx] == NULL)

		break;

	}

	term_hist_entry = idx;

    }

    term_hist_entry--;

    if (term_hist_entry >= 0) {

	pstrcpy(term_cmd_buf, sizeof(term_cmd_buf), 

                term_history[term_hist_entry]);

	term_cmd_buf_index = term_cmd_buf_size = strlen(term_cmd_buf);

    }

}
