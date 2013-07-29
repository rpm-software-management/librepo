/* librepo - A library providing (libcURL like) API to downloading repository
 * Copyright (C) 2012  Tomas Mlcoch
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */

#include <assert.h>
#include <glib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <gpgme.h>
#include <unistd.h>

#include "rcodes.h"
#include "util.h"
#include "gpg.h"

int
lr_gpg_check_signature_fd(int signature_fd,
                          int data_fd,
                          const char *home_dir,
                          GError **err)
{
    gpgme_error_t gpgerr;
    gpgme_ctx_t context;
    gpgme_data_t signature_data;
    gpgme_data_t data_data;
    gpgme_verify_result_t result;
    gpgme_signature_t sig;

    assert(!err || *err == NULL);

    // Initialization
    gpgme_check_version(NULL);
    gpgerr = gpgme_engine_check_version(GPGME_PROTOCOL_OpenPGP);
    if (gpgerr != GPG_ERR_NO_ERROR) {
        g_debug("%s: gpgme_engine_check_version: %s",
                 __func__, gpgme_strerror(gpgerr));
        g_set_error(err, LR_GPG_ERROR, LRE_GPGNOTSUPPORTED,
                    "gpgme_engine_check_version() error: %s",
                    gpgme_strerror(gpgerr));
        return LRE_GPGNOTSUPPORTED;
    }

    gpgerr = gpgme_new(&context);
    if (gpgerr != GPG_ERR_NO_ERROR) {
        g_debug("%s: gpgme_new: %s", __func__, gpgme_strerror(gpgerr));
        g_set_error(err, LR_GPG_ERROR, LRE_GPGERROR,
                    "gpgme_new() error: %s", gpgme_strerror(gpgerr));
        return LRE_GPGERROR;
    }

    gpgerr = gpgme_set_protocol(context, GPGME_PROTOCOL_OpenPGP);
    if (gpgerr != GPG_ERR_NO_ERROR) {
        g_debug("%s: gpgme_set_protocol: %s", __func__, gpgme_strerror(gpgerr));
        g_set_error(err, LR_GPG_ERROR, LRE_GPGERROR,
                    "gpgme_set_protocol() error: %s", gpgme_strerror(gpgerr));
        gpgme_release(context);
        return LRE_GPGERROR;
    }

    if (home_dir) {
        gpgerr = gpgme_ctx_set_engine_info(context, GPGME_PROTOCOL_OpenPGP,
                                        NULL, home_dir);
        if (gpgerr != GPG_ERR_NO_ERROR) {
            g_debug("%s: gpgme_ctx_set_engine_info: %s", __func__,
                    gpgme_strerror(gpgerr));
            g_set_error(err, LR_GPG_ERROR, LRE_GPGERROR,
                        "gpgme_ctx_set_engine_info() error: %s",
                        gpgme_strerror(gpgerr));
            gpgme_release(context);
            return LRE_GPGERROR;
        }
    }

    gpgme_set_armor(context, 1);

    gpgerr = gpgme_data_new_from_fd(&signature_data, signature_fd);
    if (gpgerr != GPG_ERR_NO_ERROR) {
        g_debug("%s: gpgme_data_new_from_fd: %s",
                 __func__, gpgme_strerror(gpgerr));
        g_set_error(err, LR_GPG_ERROR, LRE_GPGERROR,
                    "gpgme_data_new_from_fd(_, %d) error: %s",
                    signature_fd, gpgme_strerror(gpgerr));
        gpgme_release(context);
        return LRE_GPGERROR;
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
        return LRE_GPGERROR;
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
        return LRE_GPGERROR;
    }

    result = gpgme_op_verify_result(context);
    if (!result) {
        g_debug("%s: gpgme_op_verify_result: error", __func__);
        g_set_error(err, LR_GPG_ERROR, LRE_GPGERROR,
                    "gpgme_op_verify_result() error: %s",
                    gpgme_strerror(gpgerr));
        gpgme_release(context);
        return LRE_GPGERROR;
    }

    // Check result of verification
    sig = result->signatures;
    if(!sig) {
        g_debug("%s: signature verify error (no signatures)", __func__);
        g_set_error(err, LR_GPG_ERROR, LRE_BADGPG,
                    "Signature verify error - no signatures");
        gpgme_release(context);
        return LRE_BADGPG;
    }

    // Example of signature usage could be found in gpgme git repository
    // in the gpgme/tests/run-verify.c
    for (; sig; sig = sig->next) {
        if ((sig->summary & GPGME_SIGSUM_VALID) ||  // Valid
            (sig->summary & GPGME_SIGSUM_GREEN) ||  // Valid
            (sig->summary == 0 && sig->status == GPG_ERR_NO_ERROR)) // Valid but key is not certified with a trusted signature
        {
            gpgme_release(context);
            return LRE_OK;
        }
    }

    gpgme_release(context);
    g_debug("%s: Bad GPG signature", __func__);
    g_set_error(err, LR_GPG_ERROR, LRE_BADGPG, "Bad GPG signature");
    return LRE_BADGPG;
}

int
lr_gpg_check_signature(const char *signature_fn,
                       const char *data_fn,
                       const char *home_dir,
                       GError **err)
{
    int rc, signature_fd, data_fd;

    assert(!err || *err == NULL);

    signature_fd = open(signature_fn, O_RDONLY);
    if (signature_fd == -1) {
        g_debug("%s: Opening signature %s: %s",
                __func__, signature_fn, strerror(errno));
        g_set_error(err, LR_GPG_ERROR, LRE_IO,
                    "Error while opening signature %s: %s",
                    signature_fn, strerror(errno));
        return LRE_IO;
    }

    data_fd = open(data_fn, O_RDONLY);
    if (signature_fd == -1) {
        g_debug("%s: Opening data %s: %s",
                __func__, data_fn, strerror(errno));
        g_set_error(err, LR_GPG_ERROR, LRE_IO,
                    "Error while opening %s: %s",
                    data_fn, strerror(errno));
        close(signature_fd);
        return LRE_IO;
    }

     rc = lr_gpg_check_signature_fd(signature_fd, data_fd, home_dir, err);

     close(signature_fd);
     close(data_fd);

     return rc;
}

int
lr_gpg_import_key(const char *key_fn, const char *home_dir, GError **err)
{
    gpgme_error_t gpgerr;
    int key_fd;
    gpgme_ctx_t context;
    gpgme_data_t key_data;

    assert(!err || *err == NULL);

    // Initialization
    gpgme_check_version(NULL);
    gpgerr = gpgme_engine_check_version(GPGME_PROTOCOL_OpenPGP);
    if (gpgerr != GPG_ERR_NO_ERROR) {
        g_debug("%s: gpgme_engine_check_version: %s",
                 __func__, gpgme_strerror(gpgerr));
        g_set_error(err, LR_GPG_ERROR, LRE_GPGNOTSUPPORTED,
                    "gpgme_engine_check_version() error: %s",
                    gpgme_strerror(gpgerr));
        return LRE_GPGNOTSUPPORTED;
    }

    gpgerr = gpgme_new(&context);
    if (gpgerr != GPG_ERR_NO_ERROR) {
        g_debug("%s: gpgme_new: %s", __func__, gpgme_strerror(gpgerr));
        g_set_error(err, LR_GPG_ERROR, LRE_GPGERROR,
                    "gpgme_new() error: %s", gpgme_strerror(gpgerr));
        return LRE_GPGERROR;
    }

    gpgerr = gpgme_set_protocol(context, GPGME_PROTOCOL_OpenPGP);
    if (gpgerr != GPG_ERR_NO_ERROR) {
        g_debug("%s: gpgme_set_protocol: %s", __func__, gpgme_strerror(gpgerr));
        g_set_error(err, LR_GPG_ERROR, LRE_GPGERROR,
                    "gpgme_set_protocol() error: %s", gpgme_strerror(gpgerr));
        gpgme_release(context);
        return LRE_GPGERROR;
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
            return LRE_GPGERROR;
        }
    }

    gpgme_set_armor(context, 1);

    // Key import

    key_fd = open(key_fn, O_RDONLY);
    if (key_fd == -1) {
        g_debug("%s: Opening key: %s", __func__, strerror(errno));
        g_set_error(err, LR_GPG_ERROR, LRE_IO,
                    "Error while opening key %s: %s",
                    key_fn, strerror(errno));
        gpgme_release(context);
        return LRE_IO;
    }

    gpgerr = gpgme_data_new_from_fd(&key_data, key_fd);
    if (gpgerr != GPG_ERR_NO_ERROR) {
        g_debug("%s: gpgme_data_new_from_fd: %s",
                 __func__, gpgme_strerror(gpgerr));
        g_set_error(err, LR_GPG_ERROR, LRE_GPGERROR,
                    "gpgme_data_new_from_fd(_, %d) error: %s",
                    key_fd, gpgme_strerror(gpgerr));
        gpgme_release(context);
        close(key_fd);
        return LRE_GPGERROR;
    }

    gpgerr = gpgme_op_import(context, key_data);
    gpgme_data_release(key_data);
    if (gpgerr != GPG_ERR_NO_ERROR) {
        g_debug("%s: gpgme_op_import: %s", __func__, gpgme_strerror(gpgerr));
        g_set_error(err, LR_GPG_ERROR, LRE_GPGERROR,
                    "gpgme_op_import() error: %s", gpgme_strerror(gpgerr));
        gpgme_release(context);
        close(key_fd);
        return LRE_GPGERROR;
    }

    close(key_fd);
    gpgme_release(context);

    return LRE_OK;
}
