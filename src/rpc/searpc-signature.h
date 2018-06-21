
inline static gchar *
searpc_signature_int__void()
{
    return searpc_compute_signature ("int", 0);
}


inline static gchar *
searpc_signature_int__string()
{
    return searpc_compute_signature ("int", 1, "string");
}


inline static gchar *
searpc_signature_string__void()
{
    return searpc_compute_signature ("string", 0);
}

