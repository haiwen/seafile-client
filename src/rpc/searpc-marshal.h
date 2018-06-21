
static char *
marshal_int__void (void *func, json_t *param_array, gsize *ret_len)
{
    GError *error = NULL;

    int ret = ((int (*)(GError **))func) (&error);

    json_t *object = json_object ();
    searpc_set_int_to_ret_object (object, ret);
    return searpc_marshal_set_ret_common (object, ret_len, error);
}


static char *
marshal_int__string (void *func, json_t *param_array, gsize *ret_len)
{
    GError *error = NULL;
    const char* param1 = json_array_get_string_or_null_element (param_array, 1);

    int ret = ((int (*)(const char*, GError **))func) (param1, &error);

    json_t *object = json_object ();
    searpc_set_int_to_ret_object (object, ret);
    return searpc_marshal_set_ret_common (object, ret_len, error);
}


static char *
marshal_string__void (void *func, json_t *param_array, gsize *ret_len)
{
    GError *error = NULL;

    char* ret = ((char* (*)(GError **))func) (&error);

    json_t *object = json_object ();
    searpc_set_string_to_ret_object (object, ret);
    return searpc_marshal_set_ret_common (object, ret_len, error);
}

static void register_marshals()
{

    {
        searpc_server_register_marshal (searpc_signature_int__void(), marshal_int__void);
    }


    {
        searpc_server_register_marshal (searpc_signature_int__string(), marshal_int__string);
    }


    {
        searpc_server_register_marshal (searpc_signature_string__void(), marshal_string__void);
    }

}
