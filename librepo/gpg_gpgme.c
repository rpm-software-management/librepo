/* librepo - A library providing (libcURL like) API to downloading repository
 * Copyright (C) 2012  Tomas Mlcoch
 * Copyright (C) 2022  Jaroslav Rohel <jrohel@redhat.com>
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

#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <gpgme.h>
#include <stdint.h>
#include <unistd.h>

#if ENABLE_SELINUX
#include <selinux/selinux.h>
#include <selinux/label.h>
#endif

#include "gpg.h"
#include "gpg_internal.h"
#include "rcodes.h"
#include "util.h"

/*
 * Creates the '/run/gnupg/user/$UID' directory if it doesn't exist. If this
 * directory exists, gpgagent will create its sockets under
 * '/run/gnupg/user/$UID/gnupg'.
 *
 * If this directory doesn't exist, gpgagent will create its sockets in gpg
 * home directory, which is under '/var/cache/yum/metadata/' and this was
 * causing trouble with container images, see [1].
 *
 * Previous solution was to send the agent a "KILLAGENT" message, but that
 * would cause a race condition with calling gpgme_release(), see [2], [3].
 *
 * Current solution with precreating /run/user/$UID showed problematic when
 * this library was used out of an systemd-logind session. Then
 * /run/user/$UID, normally maintained by systemd, was assigned a SELinux
 * label unexpected by systemd causing errors on a user logout [4].
 *
 * We remedy it by choosing the label according to a default file context
 * policy (ENABLE_SELINUX macro) or by using a different path supported by
 * some GnuPG configurations (DUSE_RUN_GNUPG_USER_SOCKET macro).
 *
 * Since the agent doesn't clean up its sockets properly, by creating this
 * directory we make sure they are in a place that is not causing trouble with
 * container images.
 *
 * [1] https://bugzilla.redhat.com/show_bug.cgi?id=1650266
 * [2] https://bugzilla.redhat.com/show_bug.cgi?id=1769831
 * [3] https://github.com/rpm-software-management/microdnf/issues/50
 * [4] https://issues.redhat.com/browse/RHEL-6421
 */
static void
lr_gpg_ensure_socket_dir_exists()
{
#ifdef DUSE_RUN_GNUPG_USER_SOCKET
    const char *templates[] = { "/run/gnupg", "/run/gnupg/user", "/run/gnupg/user/%ju", NULL };
    const mode_t modes[] = { 0755, 0755, 0700, 0 };
#else
    const char *templates[] = { "/run/user/%ju", NULL };
    const mode_t modes[] = { 0700, 0 };
#endif
    const uid_t uid = getuid();
    char dirname[32];
    int res;
#if ENABLE_SELINUX
    char *old_default_context = NULL;
    int old_default_context_was_retrieved = 0;
    struct selabel_handle *labeling_handle = NULL;

    if (is_selinux_enabled()) {
       /* A purpose of this piece of code is to deal with applications whose
        * security policy overrides a file context for temporary files but don't
        * know that librepo executes GnuPG which expects a default file context. */
       if (0 == getfscreatecon(&old_default_context)) {
           old_default_context_was_retrieved = 1;
       } else {
           g_debug("Failed to retrieve a default SELinux context");
       }
       labeling_handle = selabel_open(SELABEL_CTX_FILE, NULL, 0);
       if (labeling_handle == NULL) {
           g_debug("Failed to open a SELinux labeling handle: %s", strerror(errno));
       }
    }
#endif

    for (int i = 0; templates[i] != NULL; i++) {
        res = snprintf(dirname, sizeof(dirname), templates[i], (uintmax_t)uid);
        if (res >= sizeof(dirname)) {
            g_debug("Failed to format a GnuPG agent socket path because of a small buffer");
            goto exit;
        }

#if ENABLE_SELINUX
        if (labeling_handle != NULL) {
            char *new_default_context = NULL;
            if (selabel_lookup(labeling_handle, &new_default_context, dirname, modes[i])) {
                /* Here we could hard-code "system_u:object_r:user_tmp_t:s0", but
                 * that value should be really defined in default file context
                 * SELinux policy. Only log that the policy is incomplete. */
                g_debug("Failed to look up a default SELinux label for \"%s\"", dirname);
            } else {
                if (setfscreatecon(new_default_context)) {
                    g_debug("Failed to set default SELinux context to \"%s\"",
                            new_default_context);
                }
                freecon(new_default_context);
            }
        }
#endif

        res = mkdir(dirname, modes[i]);
        if (res != 0 && errno != EEXIST) {
            g_debug("Failed to create \"%s\": %d - %s\n", dirname, errno, strerror(errno));
            goto exit;
        }
    }

exit:
#if ENABLE_SELINUX
    if (labeling_handle != NULL) {
        selabel_close(labeling_handle);
    }
    if (old_default_context_was_retrieved) {
        if (setfscreatecon(old_default_context)) {
            g_debug("Failed to restore a default SELinux context");
        }
    }
    freecon(old_default_context);
#endif
    return;
}

static gpgme_ctx_t
lr_gpg_context_init(const char *home_dir, GError **err)
{
    assert(!err || *err == NULL);

    lr_gpg_ensure_socket_dir_exists();

    gpgme_ctx_t context;
    gpgme_error_t gpgerr;

    gpgme_check_version(NULL);
    gpgerr = gpgme_engine_check_version(GPGME_PROTOCOL_OpenPGP);
    if (gpgerr != GPG_ERR_NO_ERROR) {
        g_debug("%s: gpgme_engine_check_version: %s",
                 __func__, gpgme_strerror(gpgerr));
        g_set_error(err, LR_GPG_ERROR, LRE_GPGNOTSUPPORTED,
                    "gpgme_engine_check_version() error: %s",
                    gpgme_strerror(gpgerr));
        return NULL;
    }

    gpgerr = gpgme_new(&context);
    if (gpgerr != GPG_ERR_NO_ERROR) {
        g_debug("%s: gpgme_new: %s", __func__, gpgme_strerror(gpgerr));
        g_set_error(err, LR_GPG_ERROR, LRE_GPGERROR,
                    "gpgme_new() error: %s", gpgme_strerror(gpgerr));
        return NULL;
    }

    gpgerr = gpgme_set_protocol(context, GPGME_PROTOCOL_OpenPGP);
    if (gpgerr != GPG_ERR_NO_ERROR) {
        g_debug("%s: gpgme_set_protocol: %s", __func__, gpgme_strerror(gpgerr));
        g_set_error(err, LR_GPG_ERROR, LRE_GPGERROR,
                    "gpgme_set_protocol() error: %s", gpgme_strerror(gpgerr));
        gpgme_release(context);
        return NULL;
    }

    if (home_dir) {
        gpgerr = gpgme_ctx_set_engine_info(context, GPGME_PROTOCOL_OpenPGP,
                                        NULL, home_dir);
        if (gpgerr != GPG_ERR_NO_ERROR) {
            g_debug("%s: gpgme_ctx_set_engine_info: %s", __func__, gpgme_strerror(gpgerr));
            g_set_error(err, LR_GPG_ERROR, LRE_GPGERROR,
                        "gpgme_ctx_set_engine_info() error: %s",
                        gpgme_strerror(gpgerr));
            gpgme_release(context);
            return NULL;
        }
    }

    gpgme_set_armor(context, 1);

    return context;
}

gboolean
lr_gpg_check_signature_fd(int signature_fd,
                          int data_fd,
                          const char *home_dir,
                          GError **err)
{
    gpgme_error_t gpgerr;
    gpgme_data_t signature_data;
    gpgme_data_t data_data;
    gpgme_verify_result_t result;
    gpgme_signature_t sig;

    gpgme_ctx_t context = lr_gpg_context_init(home_dir, err);
    if (!context) {
        return FALSE;
    }

    gpgerr = gpgme_data_new_from_fd(&signature_data, signature_fd);
    if (gpgerr != GPG_ERR_NO_ERROR) {
        g_debug("%s: gpgme_data_new_from_fd: %s",
                 __func__, gpgme_strerror(gpgerr));
        g_set_error(err, LR_GPG_ERROR, LRE_GPGERROR,
                    "gpgme_data_new_from_fd(_, %d) error: %s",
                    signature_fd, gpgme_strerror(gpgerr));
        gpgme_release(context);
        return FALSE;
    }

    gpgerr = gpgme_data_new_from_fd(&data_data, data_fd);
    if (gpgerr != GPG_ERR_NO_ERROR) {
        g_debug("%s: gpgme_data_new_from_fd: %s",
                 __func__, gpgme_strerror(gpgerr));
        g_set_error(err, LR_GPG_ERROR, LRE_GPGERROR,
                    "gpgme_data_new_from_fd(_, %d) error: %s",
                    data_fd, gpgme_strerror(gpgerr));
        gpgme_data_release(signature_data);
        gpgme_release(context);
        return FALSE;
    }

    // Verify
    gpgerr = gpgme_op_verify(context, signature_data, data_data, NULL);
    gpgme_data_release(signature_data);
    gpgme_data_release(data_data);
    if (gpgerr != GPG_ERR_NO_ERROR) {
        g_debug("%s: gpgme_op_verify: %s", __func__, gpgme_strerror(gpgerr));
        g_set_error(err, LR_GPG_ERROR, LRE_GPGERROR,
                    "gpgme_op_verify() error: %s", gpgme_strerror(gpgerr));
        gpgme_release(context);
        return FALSE;
    }

    result = gpgme_op_verify_result(context);
    if (!result) {
        g_debug("%s: gpgme_op_verify_result: error", __func__);
        g_set_error(err, LR_GPG_ERROR, LRE_GPGERROR,
                    "gpgme_op_verify_result() error: %s",
                    gpgme_strerror(gpgerr));
        gpgme_release(context);
        return FALSE;
    }

    // Check result of verification
    sig = result->signatures;
    if(!sig) {
        g_debug("%s: signature verify error (no signatures)", __func__);
        g_set_error(err, LR_GPG_ERROR, LRE_BADGPG,
                    "Signature verify error - no signatures");
        gpgme_release(context);
        return FALSE;
    }

    // Example of signature usage could be found in gpgme git repository
    // in the gpgme/tests/run-verify.c
    for (; sig; sig = sig->next) {
        if ((sig->summary & GPGME_SIGSUM_VALID) ||  // Valid
            (sig->summary & GPGME_SIGSUM_GREEN) ||  // Valid
            (sig->summary == 0 && sig->status == GPG_ERR_NO_ERROR)) // Valid but key is not certified with a trusted signature
        {
            gpgme_release(context);
            return TRUE;
        }
    }

    gpgme_release(context);
    g_debug("%s: Bad GPG signature", __func__);
    g_set_error(err, LR_GPG_ERROR, LRE_BADGPG, "Bad GPG signature");
    return FALSE;
}

gboolean
lr_gpg_check_signature(const char *signature_fn,
                       const char *data_fn,
                       const char *home_dir,
                       GError **err)
{
    gboolean ret;
    int signature_fd, data_fd;

    assert(!err || *err == NULL);

    signature_fd = open(signature_fn, O_RDONLY);
    if (signature_fd == -1) {
        g_debug("%s: Opening signature %s: %s",
                __func__, signature_fn, g_strerror(errno));
        g_set_error(err, LR_GPG_ERROR, LRE_IO,
                    "Error while opening signature %s: %s",
                    signature_fn, g_strerror(errno));
        return FALSE;
    }

    data_fd = open(data_fn, O_RDONLY);
    if (data_fd == -1) {
        g_debug("%s: Opening data %s: %s",
                __func__, data_fn, g_strerror(errno));
        g_set_error(err, LR_GPG_ERROR, LRE_IO,
                    "Error while opening %s: %s",
                    data_fn, g_strerror(errno));
        close(signature_fd);
        return FALSE;
    }

    ret = lr_gpg_check_signature_fd(signature_fd, data_fd, home_dir, err);

    close(signature_fd);
    close(data_fd);

    return ret;
}

gboolean
lr_gpg_import_key_from_memory(const char *key, size_t key_len, const char *home_dir, GError **err)
{
    gpgme_ctx_t context = lr_gpg_context_init(home_dir, err);
    if (!context) {
        return FALSE;
    }

    gpgme_error_t gpgerr;
    gpgme_data_t key_data;

    gpgerr = gpgme_data_new_from_mem(&key_data, key, key_len, 0);
    if (gpgerr != GPG_ERR_NO_ERROR) {
        g_debug("%s: gpgme_data_new_from_mem: %s",
                 __func__, gpgme_strerror(gpgerr));
        g_set_error(err, LR_GPG_ERROR, LRE_GPGERROR,
                    "gpgme_data_new_from_mem(_, _, %ld, 0) error: %s",
                    (unsigned long)key_len, gpgme_strerror(gpgerr));
        gpgme_release(context);
        return FALSE;
    }

    gpgerr = gpgme_op_import(context, key_data);
    if (gpgerr != GPG_ERR_NO_ERROR) {
        g_debug("%s: gpgme_op_import: %s", __func__, gpgme_strerror(gpgerr));
        g_set_error(err, LR_GPG_ERROR, LRE_GPGERROR,
                    "gpgme_op_import() error: %s", gpgme_strerror(gpgerr));
        gpgme_data_release(key_data);
        gpgme_release(context);
        return FALSE;
    }

    gpgme_data_release(key_data);
    gpgme_release(context);

    return TRUE;
}

gboolean
lr_gpg_import_key_from_fd(int key_fd, const char *home_dir, GError **err)
{
    gpgme_ctx_t context = lr_gpg_context_init(home_dir, err);
    if (!context) {
        return FALSE;
    }

    gpgme_error_t gpgerr;
    gpgme_data_t key_data;

    gpgerr = gpgme_data_new_from_fd(&key_data, key_fd);
    if (gpgerr != GPG_ERR_NO_ERROR) {
        g_debug("%s: gpgme_data_new_from_fd: %s",
                 __func__, gpgme_strerror(gpgerr));
        g_set_error(err, LR_GPG_ERROR, LRE_GPGERROR,
                    "gpgme_data_new_from_fd(_, %d) error: %s",
                    key_fd, gpgme_strerror(gpgerr));
        gpgme_release(context);
        return FALSE;
    }

    gpgerr = gpgme_op_import(context, key_data);
    if (gpgerr != GPG_ERR_NO_ERROR) {
        g_debug("%s: gpgme_op_import: %s", __func__, gpgme_strerror(gpgerr));
        g_set_error(err, LR_GPG_ERROR, LRE_GPGERROR,
                    "gpgme_op_import() error: %s", gpgme_strerror(gpgerr));
        gpgme_data_release(key_data);
        gpgme_release(context);
        return FALSE;
    }

    gpgme_data_release(key_data);
    gpgme_release(context);

    return TRUE;
}

gboolean
lr_gpg_import_key(const char *key_fn, const char *home_dir, GError **err)
{
    assert(!err || *err == NULL);

    int key_fd = open(key_fn, O_RDONLY);
    if (key_fd == -1) {
        g_debug("%s: Opening key: %s", __func__, g_strerror(errno));
        g_set_error(err, LR_GPG_ERROR, LRE_IO,
                    "Error while opening key %s: %s",
                    key_fn, g_strerror(errno));
        return FALSE;
    }

    gboolean ret = lr_gpg_import_key_from_fd(key_fd, home_dir, err);

    close(key_fd);

    return ret;
}

LrGpgKey *
lr_gpg_list_keys(gboolean export_keys, const char *home_dir, GError **err)
{
    gpgme_error_t gpgerr;

    gpgme_ctx_t context = lr_gpg_context_init(home_dir, err);
    if (!context) {
        return NULL;
    }

    GArray * keys = g_array_new(FALSE, FALSE, sizeof(LrGpgKey));

    gpgerr = gpgme_op_keylist_start(context, NULL, 0);
    while (gpg_err_code(gpgerr) == GPG_ERR_NO_ERROR) {
        gpgme_key_t key;
        gpgerr = gpgme_op_keylist_next(context, &key);
        if (gpgerr) {
            break;
        }

        GArray * subkeys = g_array_new(FALSE, FALSE, sizeof(LrGpgSubkey));
        gpgme_subkey_t subkey = key->subkeys;
        while (subkey) {
            LrGpgSubkey lr_subkey;
            lr_subkey.has_next = FALSE;
            lr_subkey.id = g_strdup(subkey->keyid);
            lr_subkey.fingerprint = g_strdup(subkey->fpr);
            lr_subkey.timestamp = subkey->timestamp;
            lr_subkey.can_sign = subkey->can_sign;
            g_array_append_val(subkeys, lr_subkey);
            subkey = subkey->next;
        }
        // Mark all subkeys in the list except the last one that they are followed by another subkey
        if (subkeys->len > 1) {
            for (guint i = 0; i < subkeys->len - 1; ++i) {
                g_array_index(subkeys, LrGpgSubkey, i).has_next = TRUE;
            }
        }

        LrGpgKey lr_key;
        lr_key.has_next = FALSE;

        GPtrArray * uid_strings = g_ptr_array_new();
        for (gpgme_user_id_t uids = key->uids; uids; uids = uids->next) {
            if (!uids->uid) {
                continue;
            }
            g_ptr_array_add(uid_strings, g_strdup(uids->uid));
        }

        gpgme_key_release(key);

        g_ptr_array_add(uid_strings, NULL);  // add terminating NULL
        lr_key.uids = (char **)g_ptr_array_free(uid_strings, FALSE);

        lr_key.subkeys = (LrGpgSubkey *)(subkeys->len > 0 ? g_array_free(subkeys, FALSE) : g_array_free(subkeys, TRUE));
        lr_key.raw_key = NULL;
        g_array_append_val(keys, lr_key);
    }
    // Mark all keys in the list except the last one that they are followed by another key
    if (keys->len > 1) {
        for (guint i = 0; i < keys->len - 1; ++i) {
            g_array_index(keys, LrGpgKey, i).has_next = TRUE;
        }
    }

    if (gpg_err_code(gpgerr) != GPG_ERR_EOF) {
        g_debug("%s: gpgme_op_keylist_: %s",
                 __func__, gpgme_strerror(gpgerr));
        g_set_error(err, LR_GPG_ERROR, LRE_GPGERROR,
                    "gpgme_op_keylist_ error: %s",
                    gpgme_strerror(gpgerr));
        lr_gpg_keys_free((LrGpgKey *)g_array_free(keys, FALSE));
        gpgme_release(context);
        return NULL;
    }

    gpgme_op_keylist_end(context);

    LrGpgKey *lr_keys = (LrGpgKey *)(keys->len > 0 ? g_array_free(keys, FALSE) : g_array_free(keys, TRUE));

    if (export_keys) {
        for (LrGpgKey *lr_key = lr_keys; lr_key; ++lr_key) {
            LrGpgSubkey *lr_subkey = lr_key->subkeys;
            if (!lr_subkey) {
                g_info("%s: Missing data to export key. Damaged key? Skipping the key", __func__);
                if (!lr_key->has_next) {
                    break;
                }
                continue;
            }

            gpgme_data_t key_data;
            gpgerr = gpgme_data_new(&key_data);
            if (gpgerr != GPG_ERR_NO_ERROR) {
                g_debug("%s: gpgme_data_new: %s", __func__, gpgme_strerror(gpgerr));
                g_set_error(err, LR_GPG_ERROR, LRE_GPGERROR,
                            "gpgme_data_new() error: %s", gpgme_strerror(gpgerr));
                lr_gpg_keys_free(lr_keys);
                gpgme_release(context);
                return NULL;
            }

            gpgerr = gpgme_op_export(context, lr_subkey->fingerprint, 0, key_data);
            if (gpgerr != GPG_ERR_NO_ERROR) {
                g_debug("%s: gpgme_op_export: %s", __func__, gpgme_strerror(gpgerr));
                g_set_error(err, LR_GPG_ERROR, LRE_GPGERROR,
                            "gpgme_op_export() error: %s", gpgme_strerror(gpgerr));
                gpgme_data_release(key_data);
                lr_gpg_keys_free(lr_keys);
                gpgme_release(context);
                return NULL;
            }

            off_t key_size = gpgme_data_seek(key_data, 0, SEEK_CUR);
            gpgerr = gpgme_data_rewind(key_data);
            if (gpgerr != GPG_ERR_NO_ERROR) {
                g_debug("%s: gpgme_data_rewind: %s", __func__, gpgme_strerror(gpgerr));
                g_set_error(err, LR_GPG_ERROR, LRE_GPGERROR,
                            "gpgme_data_rewind() error: %s", gpgme_strerror(gpgerr));
                gpgme_data_release(key_data);
                lr_gpg_keys_free(lr_keys);
                gpgme_release(context);
                return NULL;
            }

            lr_key->raw_key = g_malloc0(key_size + 1);
            ssize_t readed = gpgme_data_read(key_data, lr_key->raw_key, key_size);
            if (readed == -1) {
                g_debug("%s: gpgme_data_read: %s", __func__, gpgme_strerror(gpgerr));
                g_set_error(err, LR_GPG_ERROR, LRE_GPGERROR,
                            "gpgme_data_read() error: %s", gpgme_strerror(gpgerr));
                gpgme_data_release(key_data);
                lr_gpg_keys_free(lr_keys);
                gpgme_release(context);
                return NULL;
            }
            if (readed != key_size) {
                g_warning("%s: Error exporting key \"%s\": gpgme_data_read: Key size is %ld but readed %ld. "
                          "Skipping the key",
                          __func__, lr_key->subkeys->fingerprint, (long)key_size, (long)readed);
                g_free(lr_key->raw_key);
                lr_key->raw_key = NULL;
            }

            gpgme_data_release(key_data);

            if (!lr_key->has_next) {
                break;
            }
        }
    }

    gpgme_release(context);
    return lr_keys;
}
