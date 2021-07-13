static void test_qemu_strtoul_full_correct(void)

{

    const char *str = "123";

    unsigned long res = 999;

    int err;



    err = qemu_strtoul(str, NULL, 0, &res);



    g_assert_cmpint(err, ==, 0);

    g_assert_cmpint(res, ==, 123);

}
