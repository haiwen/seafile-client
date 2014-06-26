#include "gen_random_passwd.h"
#include <openssl/rand.h>
#include <stdlib.h>
#include <glib.h>
#include <string.h>

void deprecated_gen_random_passwd(unsigned char *s, const int len)
{
    static const char alphanum[] =
        "`~!@#$%^&*()_+-={}\\:\";'<>?,./"
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    for (int i = 0; i < len; ++i) {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    s[len] = 0;
}

int base64_limit(char* b, int len)
{
    for(int i=0; i<len; i++)
        if (b[i] == '=')
            return i+1;

    return 0;
}

int base64_sanitizer(char* str, int str_len, char* &sanitized)
{
    int limit = base64_limit(str, str_len);
    if(limit == 0)
        return 0;

    sanitized=(char*)malloc(sizeof(char)*limit);
    memcpy(sanitized, str, limit);
    return limit;
}

int gen_random_passwd(char* &s, const int len)
{
    unsigned char* raw_password = new unsigned char[len];
    if(!RAND_bytes(raw_password, len))
        RAND_pseudo_bytes(raw_password, len);

    char* base64 = g_base64_encode(raw_password, len);
    int base64_len = 4*(len/3 +1);
    int sanitized_length = base64_limit(base64, base64_len);

    s = (char*)malloc(sizeof(char)*sanitized_length);
    memcpy(s, base64,sanitized_length);

    g_free(base64);

    return sanitized_length;
}
