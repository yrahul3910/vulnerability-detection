static void do_change_vnc(const char *target, const char *arg)

{

    if (strcmp(target, "passwd") == 0 ||

	strcmp(target, "password") == 0) {

	char password[9];

	if (arg) {

	    strncpy(password, arg, sizeof(password));

	    password[sizeof(password) - 1] = '\0';

	} else

	    monitor_readline("Password: ", 1, password, sizeof(password));

	if (vnc_display_password(NULL, password) < 0)

	    term_printf("could not set VNC server password\n");

    } else {

	if (vnc_display_open(NULL, target) < 0)

	    term_printf("could not start VNC server on %s\n", target);

    }

}
