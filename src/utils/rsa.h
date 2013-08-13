/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef SEAFILE_CLIENT_RSA_H
#define SEAFILE_CLIENT_RSA_H

#include <openssl/rsa.h>

RSA* private_key_to_pub(RSA *priv);

RSA* generate_private_key(u_int bits);

char *id_from_pubkey (RSA *pubkey);

#endif
