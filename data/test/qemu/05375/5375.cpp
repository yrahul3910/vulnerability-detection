static void build_fs_mount_list_from_mtab(FsMountList *mounts, Error **errp)

{

    struct mntent *ment;

    FsMount *mount;

    char const *mtab = "/proc/self/mounts";

    FILE *fp;

    unsigned int devmajor, devminor;



    fp = setmntent(mtab, "r");

    if (!fp) {

        error_setg(errp, "failed to open mtab file: '%s'", mtab);

        return;

    }



    while ((ment = getmntent(fp))) {

        /*

         * An entry which device name doesn't start with a '/' is

         * either a dummy file system or a network file system.

         * Add special handling for smbfs and cifs as is done by

         * coreutils as well.

         */

        if ((ment->mnt_fsname[0] != '/') ||

            (strcmp(ment->mnt_type, "smbfs") == 0) ||

            (strcmp(ment->mnt_type, "cifs") == 0)) {

            continue;

        }

        if (dev_major_minor(ment->mnt_fsname, &devmajor, &devminor) == -2) {

            /* Skip bind mounts */

            continue;

        }



        mount = g_malloc0(sizeof(FsMount));

        mount->dirname = g_strdup(ment->mnt_dir);

        mount->devtype = g_strdup(ment->mnt_type);

        mount->devmajor = devmajor;

        mount->devminor = devminor;



        QTAILQ_INSERT_TAIL(mounts, mount, next);

    }



    endmntent(fp);

}
