#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/err.h>

#include <glib.h>
#include <cstring>

#include "rsa.h"

namespace {

int calculate_sha1 (unsigned char *sha1, const char *msg)
{
    SHA_CTX c;

    SHA1_Init(&c);
    SHA1_Update(&c, msg, strlen(msg));
	SHA1_Final(sha1, &c);
    return 0;
}

void
rawdata_to_hex (const unsigned char *rawdata, char *hex_str, int n_bytes)
{
    static const char hex[] = "0123456789abcdef";
    int i;

    for (i = 0; i < n_bytes; i++) {
        unsigned int val = *rawdata++;
        *hex_str++ = hex[val >> 4];
        *hex_str++ = hex[val & 0xf];
    }
    *hex_str = '\0';
}

#define sha1_to_hex(sha1, hex) rawdata_to_hex((sha1), (hex), 20)

GString* public_key_to_gstring(const RSA *rsa)
{
    GString *buf = g_string_new(NULL);
    unsigned char *temp;
    char *coded;

    gsize len = BN_num_bytes(rsa->n);
    temp = (unsigned char *)malloc(len);
    BN_bn2bin(rsa->n, temp);
    coded = g_base64_encode(temp, len);
    g_string_append (buf, coded);
    g_string_append_c (buf, ' ');
    g_free(coded);

    len = BN_num_bytes(rsa->e);
    temp = (unsigned char*)realloc(temp, len);
    BN_bn2bin(rsa->e, temp);
    coded = g_base64_encode(temp, len);
    g_string_append (buf, coded);
    g_free(coded);

    free(temp);

    return buf;
}

RSA* public_key_from_string(char *str)
{
    char *p;
    unsigned char *num;
    gsize len;
    if (!str)
        return NULL;

    if ( !(p = strchr(str, ' ')) )
        return NULL;
    *p = '\0';

    RSA *key = RSA_new();

    num = g_base64_decode(str, &len);
    key->n = BN_bin2bn(num, len, NULL);
    if (!key->n)
        goto err;
    g_free(num);

    num = g_base64_decode(p+1, &len);
    key->e = BN_bin2bn(num, len, NULL);
    if (!key->e)
        goto err;
    g_free(num);

    *p = ' ';
    return key;
err:
    *p = ' ';
    RSA_free (key);
    g_free(num);
    return NULL;
}

} // namespace


RSA*
private_key_to_pub(RSA *priv)
{
    RSA *pub = RSA_new();

    pub->n = BN_dup(priv->n);
    pub->e = BN_dup(priv->e);

    return pub;
}


char *
id_from_pubkey (RSA *pubkey)
{
    GString *buf;
    unsigned char sha1[20];
    char *id = (char *)g_malloc(41);

    buf = public_key_to_gstring (pubkey);
    calculate_sha1 (sha1, buf->str);
    sha1_to_hex (sha1, id);
    g_string_free (buf, TRUE);

    return id;
}

RSA *
generate_private_key(u_int bits)
{
	RSA *priv = NULL;

	priv = RSA_generate_key(bits, 35, NULL, NULL);
	return priv;
}
