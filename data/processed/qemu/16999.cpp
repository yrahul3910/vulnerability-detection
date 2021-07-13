static void qmp_input_type_bool(Visitor *v, const char *name, bool *obj,

                                Error **errp)

{

    QmpInputVisitor *qiv = to_qiv(v);

    QObject *qobj = qmp_input_get_object(qiv, name, true, errp);

    QBool *qbool;



    if (!qobj) {

        return;

    }

    qbool = qobject_to_qbool(qobj);

    if (!qbool) {

        error_setg(errp, QERR_INVALID_PARAMETER_TYPE, name ? name : "null",

                   "boolean");

        return;

    }



    *obj = qbool_get_bool(qbool);

}
