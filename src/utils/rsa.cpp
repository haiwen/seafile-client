#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/err.h>

#include <glib.h>
#include <cstring>

#include "rsa.h"
#include "utils.h"

namespace {

/* Forward compatibility functions if libssl < 1.1.0. */

#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)

int RSA_set0_key(RSA *r, BIGNUM *n, BIGNUM *e, BIGNUM *d)
{
   /* If the fields n and e in r are NULL, the corresponding input
    * parameters MUST be non-NULL for n and e.  d may be
    * left NULL (in case only the public key is used).
    */
   if ((r->n == NULL && n == NULL)
       || (r->e == NULL && e == NULL))
       return 0;
   if (n != NULL) {
       BN_free(r->n);
       r->n = n;
   }
   if (e != NULL) {
       BN_free(r->e);
       r->e = e;
   }
   if (d != NULL) {
       BN_free(r->d);
       r->d = d;
   }
   return 1;
}

void RSA_get0_key(const RSA *r,
                 const BIGNUM **n, const BIGNUM **e, const BIGNUM **d)
{
   if (n != NULL)
       *n = r->n;
   if (e != NULL)
       *e = r->e;
   if (d != NULL)
       *d = r->d;
}

#endif

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
    const BIGNUM *n, *e;

    RSA_get0_key (rsa, &n, &e, NULL);
    gsize len = BN_num_bytes(n);
    temp = (unsigned char *)malloc(len);
    BN_bn2bin(n, temp);
    coded = g_base64_encode(temp, len);
    g_string_append (buf, coded);
    g_string_append_c (buf, ' ');
    g_free(coded);

    len = BN_num_bytes(e);
    temp = (unsigned char*)realloc(temp, len);
    BN_bn2bin(e, temp);
    coded = g_base64_encode(temp, len);
    g_string_append (buf, coded);
    g_free(coded);

    free(temp);

    return buf;
}

/*
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
*/

} // namespace


RSA*
private_key_to_pub(RSA *priv)
{
    RSA *pub = RSA_new();
    const BIGNUM *n, *e;

    RSA_get0_key (priv, &n, &e, NULL);
    RSA_set0_key (pub, BN_dup(n), BN_dup(e), NULL);

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
    RSA *priv = RSA_new ();
    BIGNUM *e = BN_new ();

    BN_set_word (e, 35);
    RSA_generate_key_ex (priv, bits, e, NULL);

    BN_free (e);
    return priv;
}
