/* librepo - A library providing (libcURL like) API to downloading repository
 * Copyright (C) 2023  Jaroslav Rohel <jrohel@redhat.com>
 *
 * Licensed under the GNU Lesser General Public License Version 2.1
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <errno.h>
#include <fcntl.h>
#include <rpm/rpmpgp.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "gpg.h"
#include "gpg_internal.h"
#include "rcodes.h"
#include "util.h"

static const char * const BEGIN_OPENPGP_BLOCK = "-----BEGIN PGP ";
static const char * const BEGIN_OPENPGP_PUBKEY_BLOCK = "-----BEGIN PGP PUBLIC KEY BLOCK-----";
static const char * const BEGIN_OPENPGP_SIGNATURE = "-----BEGIN PGP SIGNATURE-----";

static const char *
search_line_starts_with(const char *haystack, size_t len, const char *needle) {
    const size_t needle_len = strlen(needle);
    const char *line = haystack;
    size_t len_to_end = len;
    while (len_to_end >= needle_len) {
        if (strncmp(line, needle, needle_len) == 0) {
            return line;
        }
        line = memchr(line, '\n', len_to_end);
        if (line == NULL) {
            break;
        }
        ++line;
        len_to_end = len - (line - haystack);
    }
    return NULL;
}

static void
ensure_dir_exists(const char * dir)
{
    const int res = mkdir(dir, 0744);
    if (res != 0 && errno != EEXIST) {
        g_warning("Failed to create \"%s\": %d - %s\n", dir, errno, g_strerror(errno));
    }
}

static inline gchar
nibble_to_hex(guint8 value) {
    return value < 10 ? value + '0' : value - 10 + 'A';
}

static gchar *
bin_to_hex(const guint8 * buffer, gsize buffer_length) {
    gchar * const ret = g_new(gchar, buffer_length * 2 + 1);
    gchar * out_ptr = ret;
    for (gsize i = 0; i < buffer_length; ++i) {
        guint8 value = buffer[i];
        *out_ptr++ = nibble_to_hex(value >> 4);
        *out_ptr++ = nibble_to_hex(value & 0x0F);
    }
    *out_ptr = '\0';
    return ret;
}

static ssize_t
read_file_fd_to_memory(int fd, gchar ** buf, GError **err) {
    const off_t size = lseek(fd, 0, SEEK_END);
    if (size == -1) {
        const gchar * errstr = g_strerror(errno);
        g_debug("%s: Can't seek to end of file: %s", __func__, errstr);
        g_set_error(err, LR_GPG_ERROR, LRE_IO, "Can't seek to end of file: %s", errstr);
        return -1;
    }
    if (lseek(fd, 0, SEEK_SET) == -1) {
        const gchar * errstr = g_strerror(errno);
        g_debug("%s: Can't seek to beginning of file: %s", __func__, errstr);
        g_set_error(err, LR_GPG_ERROR, LRE_IO, "Can't seek to beginning of file: %s", errstr);
        return -1;
    }
    g_autofree gchar * bufp = g_new(gchar, size);
    ssize_t ret = read(fd, bufp, size);
    if (ret == -1) {
        const gchar * errstr = g_strerror(errno);
        g_debug("%s: Error reading from file descriptor %i: %s", __func__, fd, errstr);
        g_set_error(err, LR_GPG_ERROR, LRE_IO, "Error reading from file descriptor %i: %s", fd, errstr);
        return -1;
    }
    if (ret != size) {
        g_debug("%s: Detected file size %li but read %li", __func__, (long)size, (long)ret);
        g_set_error(err, LR_GPG_ERROR, LRE_IO, "Detected file size %li but read %li", (long)size, (long)ret);
        return -1;
    }
    *buf = g_steal_pointer(&bufp);
    return ret;
}

static ssize_t
read_file_to_memory(const char * path, gchar ** buf, GError **err) {
    const int fd = open(path, O_RDONLY);
    if (fd == -1) {
        g_debug("%s: Opening file %s: %s", __func__, path, g_strerror(errno));
        g_set_error(err, LR_GPG_ERROR, LRE_IO, "Error while opening file %s: %s", path, g_strerror(errno));
        return -1;
    }
    ssize_t ret = read_file_fd_to_memory(fd, buf, err);
    close(fd);
    return ret;
}

static ssize_t
write_memory_to_file_fd(int fd, const guint8 * buf, size_t len, GError **err) {
    const ssize_t ret = write(fd, buf, len);
    if (ret == -1) {
        const gchar * errstr = g_strerror(errno);
        g_debug("%s: Error writing to file descriptor %i: %s", __func__, fd, errstr);
        g_set_error(err, LR_GPG_ERROR, LRE_IO, "Error writing to file descriptor %i: %s", fd, errstr);
        return -1;
    }
    if (ret != len) {
        g_debug("%s: Requested to write %li octets, but written %li.", __func__, (long)len, (long)ret);
        g_set_error(err, LR_GPG_ERROR, LRE_IO, "Requested to write %li octets, but written %li", (long)len, (long)ret);
        return -1;
    }
    return ret;
}

static ssize_t
write_memory_to_file(const char * path, const guint8 * buf, size_t len, GError **err) {
    const int fd = creat(path, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd == -1) {
        const gchar * errstr = g_strerror(errno);
        g_debug("%s: Error: Failed to create file \"%s\": %s", __func__, path, errstr);
        g_set_error(err, LR_GPG_ERROR, LRE_IO, "Failed to create file \"%s\": %s", path, errstr);
        return -1;
    }
    const ssize_t ret = write_memory_to_file_fd(fd, buf, len, err);
    close(fd);
    return ret;
}

/* Invoke librpmio pgpPrtParams2(), log error and set GError with forwarded
 * librpmio error, and return pgpPrtParams2() return code. */
static int
lr_pgpPrtParams2_with_gerror(const uint8_t *pkts, size_t pktlen, unsigned int pkttype,
        pgpDigParams *ret, GError **err) {
    int retval;
    char *message = NULL;

    retval =
#ifdef HAVE_PGPPRTPARAMS2
        pgpPrtParams2(pkts, pktlen, pkttype, ret, &message);
#else
        pgpPrtParams(pkts, pktlen, pkttype, ret);
#endif
    if (retval == -1) {
        if (message == NULL) {
            g_debug("%s: Error during parsing OpenPGP packets", __func__);
            g_set_error(err, LR_GPG_ERROR, LRE_GPGERROR,
                    "Error during parsing OpenPGP packets");
        } else {
            g_debug("%s: Error during parsing OpenPGP packets: %s", __func__, message);
            g_set_error(err, LR_GPG_ERROR, LRE_GPGERROR,
                    "Error during parsing OpenPGP packets: %s", message);
        }
    }
    /* The message can be set on success. */
    if (message != NULL)
        free(message);

    return retval;
}

// Searches for a key with `keyid` in the OpenPGP packet.
static gboolean
search_key_id(const guint8 * keyid, gchar * buf, size_t len, pgpDigParams * dig_params, GError **err) {
    pgpDigParams main_dig_params = NULL;
    if (lr_pgpPrtParams2_with_gerror((const uint8_t *)buf, len, PGPTAG_PUBLIC_KEY, &main_dig_params, err) == -1) {
        return FALSE;
    }
    if (main_dig_params == NULL) {
        g_warning("%s: No key found in the key file.", __func__);
        return FALSE;
    }

    gboolean ret = FALSE;
    int subkeys_count = 0;
    pgpDigParams * subkeys = NULL;
    do {
        if (memcmp(pgpDigParamsSignID(main_dig_params), keyid, sizeof(pgpKeyID_t)) == 0) {
            if (dig_params != NULL) {
                *dig_params = g_steal_pointer(&main_dig_params);
            }
            ret = TRUE;
            break; // key id match main_dig_params
        }

        if (pgpPrtParamsSubkeys((const uint8_t *)buf, len, main_dig_params, &subkeys, &subkeys_count) == -1) {
            g_warning("%s: Parse subkey parameters from OpenPGP packet(s) failed", __func__);
            break;
        }

        for (int idx = 0; idx < subkeys_count; ++idx) {
            if (memcmp(pgpDigParamsSignID(subkeys[idx]), keyid, sizeof(pgpKeyID_t)) == 0) {
                if (dig_params != NULL) {
                    *dig_params = g_steal_pointer(&subkeys[idx]);
                }
                ret = TRUE; // key id match subkey
                break;
            }
        }
    } while (FALSE);

    for (int idx = 0; idx < subkeys_count; ++idx) {
        pgpDigParamsFree(subkeys[idx]);
    }
    pgpDigParamsFree(main_dig_params);

    return ret;
}

static gboolean
is_pubkey_filename(const gchar * filename) {
    const size_t name_len = strlen(filename);
    return name_len == 20 && strcmp(filename + name_len - 4, ".pub") == 0;
}

// Searches for a key with `keyid` in keyring.
static gboolean
keyring_search_key_id(const guint8 * keyid, pgpDigParams * dig_params, gchar ** pkts, size_t * pkts_len, const char * home_dir, GError **err) {
    if (!g_file_test(home_dir, G_FILE_TEST_EXISTS)) {
        return FALSE;
    }

    GDir * const dir = g_dir_open(home_dir, 0, err);
    if (dir == NULL) {
        return FALSE;
    }

    const gchar * filename;
    while ((filename = g_dir_read_name(dir))) {
        if (!is_pubkey_filename(filename)) {
            continue;
        }
        g_autofree gchar * path = g_strconcat(home_dir, "/", filename, NULL);
        g_autofree gchar * buf = NULL;
        const ssize_t size = read_file_to_memory(path, &buf, err);
        if (size == -1) {
            continue;
        }
        if (search_key_id(keyid, buf, size, dig_params, err)) {
            g_dir_close(dir);
            if (pkts) {
                *pkts = g_steal_pointer(&buf);
            }
            if (pkts_len) {
                *pkts_len = size;
            }
            return TRUE;
        }
        if (*err != NULL) {
            break;
        }

    }
    g_dir_close(dir);

    return FALSE;
}

static gboolean
import_raw_key_from_memory(const guint8 *key, size_t key_len, const char *home_dir, GError **err)
{
    ensure_dir_exists(home_dir);

    size_t cert_len;
    if (pgpPubKeyCertLen(key, key_len, &cert_len) != 0) {
        g_debug("%s: Error: Compute cert len failed", __func__);
        g_set_error(err, LR_GPG_ERROR, LRE_GPGERROR, "Compute cert len failed");
        return FALSE;
    }
    do {
        pgpDigParams main_dig_params = NULL;
        if (lr_pgpPrtParams2_with_gerror(key, cert_len, PGPTAG_PUBLIC_KEY, &main_dig_params, err) == -1) {
            return FALSE;
        }
        const guint8 * keyid = pgpDigParamsSignID(main_dig_params);
        const gboolean found = keyring_search_key_id(keyid, NULL, NULL, NULL, home_dir, err);
        if (found) {
            g_info("%s: Key with id \"%s\" already found in keyring", __func__, keyid);
        } else {
            g_autofree gchar * keyid_hex = bin_to_hex(keyid, sizeof(pgpKeyID_t));
            g_autofree gchar * path = g_strconcat(home_dir, "/", keyid_hex, ".pub", NULL);
            if (write_memory_to_file(path, key, key_len, err) <= 0) {
                pgpDigParamsFree(main_dig_params);
                g_debug("%s: Error: Writing file \"%s\" to keyring failed", __func__, path);
                g_set_error(err, LR_GPG_ERROR, LRE_GPGERROR, "Writing file \"%s\" to keyring failed", path);
                return FALSE;
            }
        }
        pgpDigParamsFree(main_dig_params);

        key_len -= cert_len;
        if (key_len <= 0) {
            break;
        }
        key += cert_len;
        if (pgpPubKeyCertLen(key, key_len, &cert_len) != 0) {
            g_debug("%s: Error: Compute cert len failed", __func__);
            g_set_error(err, LR_GPG_ERROR, LRE_GPGERROR, "Compute cert len failed");
            return FALSE;
        }
    } while (TRUE);

    return TRUE;
}

gboolean
lr_gpg_import_key_from_memory(const char *key, size_t key_len, const char *home_dir, GError **err)
{
    const char *block_begin = search_line_starts_with(key, key_len, BEGIN_OPENPGP_BLOCK);
    if (!block_begin) {
        // ASCII Armored OpenPGP block not found. Is binary? Try it.
        return import_raw_key_from_memory((const guint8 *)key, key_len, home_dir, err);
    }

    block_begin = search_line_starts_with(key, key_len, BEGIN_OPENPGP_PUBKEY_BLOCK);
    if (block_begin == NULL) {
        g_debug("%s: Error: Public key not found", __func__);
        g_set_error(err, LR_GPG_ERROR, LRE_GPGERROR, "Public key not found");
        return FALSE;
    }

    // `pgpParsePkts` needs null-terminated input, if null byte not found, make a local null-terminated copy
    g_autofree gchar * key_with_null_byte = NULL;
    if (memchr(block_begin, '\0', key_len) == NULL) {
        key_with_null_byte = g_new(gchar, key_len + 1);
        memcpy(key_with_null_byte, key, key_len);
        key_with_null_byte[key_len] = '\0';

        // set block_begin and key to null byte terminated local copy
        block_begin = key_with_null_byte + (block_begin - key);
        key = key_with_null_byte;
    }

    do {
        guint8 * pkts = NULL;
        size_t pkts_len;
        pgpArmor armor_type = pgpParsePkts(block_begin, &pkts, &pkts_len);
        if (armor_type < 0) {
            g_debug("%s: Error: Parsing armored OpenPGP packet(s) failed", __func__);
            g_set_error(err, LR_GPG_ERROR, LRE_GPGERROR, "Parsing armored OpenPGP packet(s) failed");
            return FALSE;
        }
        if (armor_type != PGPARMOR_PUBKEY) {
            g_debug("%s: Error: Public key not found", __func__);
            g_set_error(err, LR_GPG_ERROR, LRE_GPGERROR, "Public key not found");
            return FALSE;
        }
        if (import_raw_key_from_memory(pkts, pkts_len, home_dir, err) == FALSE) {
            free(pkts);
            return FALSE;
        }
        free(pkts);
        block_begin += strlen(BEGIN_OPENPGP_PUBKEY_BLOCK);
        block_begin = search_line_starts_with(block_begin, key_len - (block_begin - key), BEGIN_OPENPGP_PUBKEY_BLOCK);
    } while (block_begin != NULL);
    return TRUE;
}

gboolean
lr_gpg_import_key_from_fd(int key_fd, const char *home_dir, GError **err)
{
    g_autofree gchar * buf = NULL;
    const ssize_t size = read_file_fd_to_memory(key_fd, &buf, err);
    return lr_gpg_import_key_from_memory((const char *)buf, size, home_dir, err);
}

gboolean
lr_gpg_import_key(const char *key_fn, const char *home_dir, GError **err)
{
    g_autofree gchar * buf = NULL;
    const ssize_t size = read_file_to_memory(key_fn, &buf, err);
    return lr_gpg_import_key_from_memory((const char *)buf, size, home_dir, err);
}

LrGpgKey *
lr_gpg_list_keys(gboolean export_keys, const char *home_dir, GError **err)
{
    if (!g_file_test(home_dir, G_FILE_TEST_EXISTS)) {
        return NULL;
    }

    GDir * const dir = g_dir_open(home_dir, 0, err);
    if (dir == NULL) {
        return NULL;
    }

    GArray * keys = g_array_new(FALSE, FALSE, sizeof(LrGpgKey));
    const gchar * filename;
    while ((filename = g_dir_read_name(dir))) {
        if (!is_pubkey_filename(filename)) {
            continue;
        }

        g_autofree gchar * path = g_strconcat(home_dir, "/", filename, NULL);
        g_autofree gchar * buf = NULL;
        ssize_t len = read_file_to_memory(path, &buf, err);
        if (len == -1) {
            lr_gpg_keys_free((LrGpgKey *)g_array_free(keys, FALSE));
            g_dir_close(dir);
            return NULL;
        }

        pgpDigParams main_dig_params = NULL;
        if (lr_pgpPrtParams2_with_gerror((const uint8_t *)buf, len, PGPTAG_PUBLIC_KEY, &main_dig_params, NULL) == -1) {
            continue;
        }

        int subkeys_count = 0;
        pgpDigParams * subkeys = NULL;
        do {
            if (pgpPrtParamsSubkeys((const uint8_t *)buf, len, main_dig_params, &subkeys, &subkeys_count) != 0) {
                g_debug("%s: Parse subkey parameters from OpenPGP packet(s) failed", __func__);
                break;
            }

            GArray * gsubkeys = g_array_new(FALSE, FALSE, sizeof(LrGpgSubkey));
            // insert main key as first subkey
            pgpDigParams subkey = main_dig_params;
            int idx = 0;
            do {
                LrGpgSubkey lr_subkey;
                lr_subkey.has_next = FALSE;
                lr_subkey.id = bin_to_hex(pgpDigParamsSignID(subkey), PGP_KEYID_LEN);
                if (subkey == main_dig_params) {
                    guint8 * fp;
                    size_t fp_len;
                    if (pgpPubkeyFingerprint((const uint8_t *)buf, len, &fp, &fp_len) == 0) {;
                        lr_subkey.fingerprint = bin_to_hex(fp, fp_len);
                        free(fp);
                    } else {
                        g_warning("%s: Error: Calculate OpenPGP public key fingerprint failed", __func__);
                        lr_subkey.fingerprint = g_strdup("");
                    }
                } else {
                    // TODO[jrohel]: Set fingerprint for subkeys
                    lr_subkey.fingerprint = g_strdup(""); // !!! g_strdup(subkey->fpr);
                }
                lr_subkey.timestamp = pgpDigParamsCreationTime(subkey);
                // TODO[jrohel]: Set the current value instead of TRUE
                lr_subkey.can_sign = TRUE;
                g_array_append_val(gsubkeys, lr_subkey);
                if (idx == subkeys_count) {
                    break;
                }
                subkey = subkeys[idx++];
            } while (TRUE);
            // All subkeys in the list except the last one are followed by another subkey
            if (gsubkeys->len > 1) {
                for (guint i = 0; i < gsubkeys->len - 1; ++i) {
                    g_array_index(gsubkeys, LrGpgSubkey, i).has_next = TRUE;
                }
            }

            LrGpgKey lr_key;
            lr_key.has_next = FALSE;

            GPtrArray * uid_strings = g_ptr_array_new();
            // TODO[jrohel]: only one uid is inserted
            g_ptr_array_add(uid_strings, g_strdup(pgpDigParamsUserID(main_dig_params)));

            g_ptr_array_add(uid_strings, NULL);  // add terminating NULL
            lr_key.uids = (char **)g_ptr_array_free(uid_strings, FALSE);

            lr_key.subkeys = (LrGpgSubkey *)(gsubkeys->len > 0 ? g_array_free(gsubkeys, FALSE) : g_array_free(gsubkeys, TRUE));
            lr_key.raw_key = export_keys ? pgpArmorWrap(PGPARMOR_PUBKEY, (const unsigned char *)buf, len) : NULL;
            g_array_append_val(keys, lr_key);

        } while (FALSE);

        for (int idx = 0; idx < subkeys_count; ++idx) {
            pgpDigParamsFree(subkeys[idx]);
        }
        pgpDigParamsFree(main_dig_params);
    }
    // All keys in the list except the last one are followed by another key
    for (guint i = 0; i + 1 < keys->len; ++i) {
        g_array_index(keys, LrGpgKey, i).has_next = TRUE;
    }
    LrGpgKey *lr_keys = (LrGpgKey *)(keys->len > 0 ? g_array_free(keys, FALSE) : g_array_free(keys, TRUE));

    g_dir_close(dir);

    return lr_keys;
}

static gboolean
check_signature(const gchar * sig_buf, ssize_t sig_buf_len, const gchar * data, ssize_t data_len, const char * home_dir, GError **err) {
    uint8_t * pkts = NULL;
    size_t pkts_len;
    const char *block_begin = search_line_starts_with(sig_buf, sig_buf_len, BEGIN_OPENPGP_BLOCK);
    if (block_begin != NULL) {
        block_begin = search_line_starts_with(block_begin, sig_buf_len - (block_begin - sig_buf), BEGIN_OPENPGP_SIGNATURE);
        if (block_begin == NULL) {
            g_debug("%s: Error: Signature not found in armored packets", __func__);
            g_set_error(err, LR_GPG_ERROR, LRE_GPGERROR, "%s: Signature not found in armored packets", __func__);
            return FALSE;
        }

        // `pgpParsePkts` needs null-terminated input, if null byte not found, make a local null-terminated copy
        g_autofree gchar * sig_buf_with_null_byte = NULL;
        if (memchr(block_begin, '\0', sig_buf_len) == NULL) {
            sig_buf_with_null_byte = g_new(gchar, sig_buf_len + 1);
            memcpy(sig_buf_with_null_byte, sig_buf, sig_buf_len);
            sig_buf_with_null_byte[sig_buf_len] = '\0';

            // set block_begin and key to null byte terminated local copy
            block_begin = sig_buf_with_null_byte + (block_begin - sig_buf);
            sig_buf = sig_buf_with_null_byte;
        }

        pgpArmor ret_pgparmor = pgpParsePkts((const char *)block_begin, &pkts, &pkts_len);
        if (ret_pgparmor < 0) {
            g_debug("%s: Error: Parsing armored OpenPGP packet(s) failed", __func__);
            g_set_error(err, LR_GPG_ERROR, LRE_GPGERROR, "Parsing armored OpenPGP packet(s) failed");
            return FALSE;
        }
    } else {
        // ASCII Armored OpenPGP block not found. Is binary? Try it.
        pkts_len = sig_buf_len;
        pkts = malloc(sig_buf_len);
        memcpy(pkts, sig_buf, sig_buf_len);
    }

    pgpDigParams signature_dig_params = NULL;
    if (lr_pgpPrtParams2_with_gerror(pkts, pkts_len, PGPTAG_SIGNATURE, &signature_dig_params, err) == -1) {
        free(pkts);
        return FALSE;
    }
    const guint8 * const signing_keyid = pgpDigParamsSignID(signature_dig_params);
    gchar * signing_key_pkts;
    size_t signing_key_pkts_len;
    pgpDigParams signing_key_dig_params = NULL;
    gboolean ret = FALSE;
    if (keyring_search_key_id(signing_keyid, &signing_key_dig_params, &signing_key_pkts, &signing_key_pkts_len, home_dir, err))
    {
        g_debug("%s: Signing key found", __func__);

        /* Do the key parameters match the signature? */
	    if ((pgpDigParamsAlgo(signature_dig_params, PGPVAL_PUBKEYALGO)
                 != pgpDigParamsAlgo(signing_key_dig_params, PGPVAL_PUBKEYALGO)) ||
                memcmp(pgpDigParamsSignID(signature_dig_params), pgpDigParamsSignID(signing_key_dig_params),
                       PGP_KEYID_LEN))
        {
            g_debug("%s: Signature and public key parameters does not match", __func__);
            g_set_error(err, LR_GPG_ERROR, LRE_BADGPG, "Signature and public key parameters does not match");
            pgpDigParamsFree(signing_key_dig_params);
            pgpDigParamsFree(signature_dig_params);
            free(pkts);
            return FALSE;
        }

        const unsigned int hash_algo = pgpDigParamsAlgo(signature_dig_params, PGPVAL_HASHALGO);
        DIGEST_CTX hashctx = rpmDigestInit(hash_algo, RPMDIGEST_NONE);
        char *message = NULL;
        rpmDigestUpdate(hashctx, data, data_len);
        rpmRC ret_verify =
#ifdef HAVE_PGPVERIFYSIGNATURE2
            pgpVerifySignature2(signing_key_dig_params, signature_dig_params, hashctx, &message);
#else
            pgpVerifySignature(signing_key_dig_params, signature_dig_params, hashctx);
#endif
        rpmDigestFinal(hashctx, NULL, NULL, 0);
        pgpDigParamsFree(signing_key_dig_params);

        ret = ret_verify == RPMRC_OK || ret_verify == RPMRC_NOTTRUSTED;
        if (!ret) {
            if (message == NULL) {
                g_debug("%s: Bad PGP signature", __func__);
                g_set_error(err, LR_GPG_ERROR, LRE_BADGPG, "Bad PGP signature");
            } else {
                g_debug("%s: Bad PGP signature: %s", __func__, message);
                g_set_error(err, LR_GPG_ERROR, LRE_BADGPG, "Bad PGP signature: %s", message);
            }
        }
        if (message != NULL)
            free(message);
    } else {
        g_debug("%s: Signing key not found", __func__);
        g_set_error(err, LR_GPG_ERROR, LRE_BADGPG, "Signing key not found");
    }

    pgpDigParamsFree(signature_dig_params);
    free(pkts);

    return ret;
}

gboolean
lr_gpg_check_signature_fd(int signature_fd,
                          int data_fd,
                          const char *home_dir,
                          GError **err)
{
    g_autofree gchar * sig_buf = NULL;
    const ssize_t sig_buf_len = read_file_fd_to_memory(signature_fd, &sig_buf, err);
    if (sig_buf_len == -1) {
        return FALSE;
    }

    g_autofree gchar * data_buf = NULL;
    const ssize_t data_buf_len = read_file_fd_to_memory(data_fd, &data_buf, err);
    if (data_buf_len == -1) {
        return FALSE;
    }

    return check_signature(sig_buf, sig_buf_len, data_buf, data_buf_len, home_dir, err);
}

gboolean
lr_gpg_check_signature(const char *signature_fn,
                       const char *data_fn,
                       const char *home_dir,
                       GError **err)
{
    g_autofree gchar * sig_buf = NULL;
    const ssize_t sig_buf_len = read_file_to_memory(signature_fn, &sig_buf, err);
    if (sig_buf_len == -1) {
        return FALSE;
    }

    g_autofree gchar * data_buf = NULL;
    const ssize_t data_buf_len = read_file_to_memory(data_fn, &data_buf, err);
    if (data_buf_len == -1) {
        return FALSE;
    }

    return check_signature(sig_buf, sig_buf_len, data_buf, data_buf_len, home_dir, err);
}
