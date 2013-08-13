#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/bio.h>

#include <glib.h>
#include <glib/gstdio.h>

extern "C" {
#include <ccnet/option.h>
}

#include "utils/rsa.h"
#include "ccnet-init.h"

/**
 * Number of bits in the RSA/DSA key. This value can be set on the command
 * line.
 */
#define DEFAULT_BITS 2048

namespace {

guint32 bits = 0;
RSA *peer_privkey, *peer_pubkey;
const char *user_name;
const char *peer_name;

char *peer_id;

int mkdir_if_not_exist (const char *dir)
{
    if (g_file_test(dir, G_FILE_TEST_IS_DIR)) {
        return 0;
    }

    return g_mkdir(dir, 0700);
}

void save_privkey (RSA *key, const char *file)
{
    FILE *f;
    f = g_fopen (file, "wb");
    PEM_write_RSAPrivateKey(f, key, NULL, NULL, 0, NULL, NULL);
    fclose (f);
}

void create_peerkey ()
{
    peer_privkey = generate_private_key (bits);
    peer_pubkey = private_key_to_pub (peer_privkey);
}

char *get_peer_name ()
{
    int ret;
    char buf[512];
    char computer_name[128] = {0};

    gethostname (computer_name, sizeof(computer_name));

    ret = snprintf (buf, sizeof(buf), "%s@%s", user_name, computer_name);
    return g_strdup(buf);
}

int make_configure_file (const char *config_file)
{
    FILE *fp;

    if ((fp = g_fopen(config_file, "wb")) == NULL) {
        fprintf (stderr, "Open config file %s error: %s\n",
                 config_file, strerror(errno));
        return ERR_CONF_FILE;
    }

    fprintf (fp, "[General]\n");
    fprintf (fp, "USER_NAME = %s\n", user_name);
    fprintf (fp, "ID = %s\n", peer_id);
    fprintf (fp, "NAME = %s\n", peer_name);
    fprintf (fp, "\n");

    fprintf (fp, "[Network]\n");
    fprintf (fp, "PORT = 10001\n");
    fprintf (fp, "\n");

    fprintf (fp, "[Client]\n");
    fprintf (fp, "PORT = 13419\n");

    fclose (fp);

    fprintf (stdout, "done\n");
    return 0;
}

int make_config_dir(const char *ccnet_dir)
{
    int err = 0;
    char *identity_file_peer;
    char *config_file;

    create_peerkey ();
    peer_name = get_peer_name();

    peer_id = id_from_pubkey (peer_pubkey);
    identity_file_peer = g_build_filename (ccnet_dir, PEER_KEYFILE, NULL);

    /* create dir */
    if (mkdir_if_not_exist(ccnet_dir) < 0) {
        fprintf (stderr, "Make dir %s error: %s\n",
                 ccnet_dir, strerror(errno));
        return ERR_PERMISSION;
    }

    /* save key */
    save_privkey (peer_privkey, identity_file_peer);

    /* make configure file */
    config_file = g_build_filename (ccnet_dir, CONFIG_FILE_NAME, NULL);
    err = make_configure_file (config_file);
    if (err)
        return err;

    printf ("Successly create ccnet config dir %s.\n", ccnet_dir);
    return 0;
}

} // namespace


int create_ccnet_config (const char *ccnet_dir)
{
    SSLeay_add_all_algorithms();

    g_assert (RAND_status() == 1);

    if (bits == 0)
        bits = DEFAULT_BITS;

    user_name = g_get_user_name();
    return make_config_dir(ccnet_dir);
}
