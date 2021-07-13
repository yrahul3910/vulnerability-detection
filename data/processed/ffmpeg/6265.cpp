int show_license(void *optctx, const char *opt, const char *arg)

{

    printf(

#if CONFIG_NONFREE

    "This version of %s has nonfree parts compiled in.\n"

    "Therefore it is not legally redistributable.\n",

    program_name

#elif CONFIG_GPLV3

    "%s is free software; you can redistribute it and/or modify\n"

    "it under the terms of the GNU General Public License as published by\n"

    "the Free Software Foundation; either version 3 of the License, or\n"

    "(at your option) any later version.\n"

    "\n"

    "%s is distributed in the hope that it will be useful,\n"

    "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"

    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"

    "GNU General Public License for more details.\n"

    "\n"

    "You should have received a copy of the GNU General Public License\n"

    "along with %s.  If not, see <http://www.gnu.org/licenses/>.\n",

    program_name, program_name, program_name

#elif CONFIG_GPL

    "%s is free software; you can redistribute it and/or modify\n"

    "it under the terms of the GNU General Public License as published by\n"

    "the Free Software Foundation; either version 2 of the License, or\n"

    "(at your option) any later version.\n"

    "\n"

    "%s is distributed in the hope that it will be useful,\n"

    "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"

    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"

    "GNU General Public License for more details.\n"

    "\n"

    "You should have received a copy of the GNU General Public License\n"

    "along with %s; if not, write to the Free Software\n"

    "Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA\n",

    program_name, program_name, program_name

#elif CONFIG_LGPLV3

    "%s is free software; you can redistribute it and/or modify\n"

    "it under the terms of the GNU Lesser General Public License as published by\n"

    "the Free Software Foundation; either version 3 of the License, or\n"

    "(at your option) any later version.\n"

    "\n"

    "%s is distributed in the hope that it will be useful,\n"

    "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"

    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"

    "GNU Lesser General Public License for more details.\n"

    "\n"

    "You should have received a copy of the GNU Lesser General Public License\n"

    "along with %s.  If not, see <http://www.gnu.org/licenses/>.\n",

    program_name, program_name, program_name

#else

    "%s is free software; you can redistribute it and/or\n"

    "modify it under the terms of the GNU Lesser General Public\n"

    "License as published by the Free Software Foundation; either\n"

    "version 2.1 of the License, or (at your option) any later version.\n"

    "\n"

    "%s is distributed in the hope that it will be useful,\n"

    "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"

    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU\n"

    "Lesser General Public License for more details.\n"

    "\n"

    "You should have received a copy of the GNU Lesser General Public\n"

    "License along with %s; if not, write to the Free Software\n"

    "Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA\n",

    program_name, program_name, program_name

#endif

    );



    return 0;

}
