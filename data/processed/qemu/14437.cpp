void json_start_array(QJSON *json, const char *name)

{

    json_emit_element(json, name);

    qstring_append(json->str, "[ ");

    json->omit_comma = true;

}
