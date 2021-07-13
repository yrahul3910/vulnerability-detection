START_TEST(float_number)

{

    int i;

    struct {

        const char *encoded;

        double decoded;

        int skip;

    } test_cases[] = {

        { "32.43", 32.43 },

        { "0.222", 0.222 },

        { "-32.12313", -32.12313 },

        { "-32.20e-10", -32.20e-10, .skip = 1 },

        { },

    };



    for (i = 0; test_cases[i].encoded; i++) {

        QObject *obj;

        QFloat *qfloat;



        obj = qobject_from_json(test_cases[i].encoded);

        fail_unless(obj != NULL);

        fail_unless(qobject_type(obj) == QTYPE_QFLOAT);



        qfloat = qobject_to_qfloat(obj);

        fail_unless(qfloat_get_double(qfloat) == test_cases[i].decoded);



        if (test_cases[i].skip == 0) {

            QString *str;



            str = qobject_to_json(obj);

            fail_unless(strcmp(qstring_get_str(str), test_cases[i].encoded) == 0);

            QDECREF(str);

        }



        QDECREF(qfloat);

    }

}
