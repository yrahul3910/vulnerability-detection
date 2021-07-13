static void qvirtio_9p_start(void)

{

    char *args;



    test_share = g_strdup("/tmp/qtest.XXXXXX");

    g_assert_nonnull(mkdtemp(test_share));



    args = g_strdup_printf("-fsdev local,id=fsdev0,security_model=none,path=%s "

                           "-device virtio-9p-pci,fsdev=fsdev0,mount_tag=%s",

                           test_share, mount_tag);



    qtest_start(args);

    g_free(args);

}
