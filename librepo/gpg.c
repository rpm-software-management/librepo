/* librepo - A library providing (libcURL like) API to downloading repository
 * Copyright (C) 2012  Tomas Mlcoch
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

static void
kill_gpg_agent(gpgme_ctx_t context, const char *home_dir)
{
    gpgme_error_t gpgerr;

    gpgerr = gpgme_set_protocol(context, GPGME_PROTOCOL_ASSUAN);
    if (gpgerr != GPG_ERR_NO_ERROR) {
        g_warning("%s: gpgme_set_protocol: %s", __func__, gpgme_strerror(gpgerr));
        return;
    }
    if (home_dir) {
        gchar * gpg_agent_sock = g_build_filename(home_dir, "S.gpg-agent", NULL);
        gpgerr = gpgme_ctx_set_engine_info(context, GPGME_PROTOCOL_ASSUAN, gpg_agent_sock, home_dir);
        g_free(gpg_agent_sock);
        if (gpgerr != GPG_ERR_NO_ERROR) {
            g_warning("%s: gpgme_ctx_set_engine_info: %s", __func__, gpgme_strerror(gpgerr));
            return;
        }
    }
    gpgerr = gpgme_op_assuan_transact_ext(context, "KILLAGENT", NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    if (gpgerr != GPG_ERR_NO_ERROR)
        g_debug("%s: gpgme_op_assuan_transact_ext: %s", __func__, gpgme_strerror(gpgerr));
}

gboolean
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
        return FALSE;
    }

    gpgerr = gpgme_new(&context);
    if (gpgerr != GPG_ERR_NO_ERROR) {
        g_debug("%s: gpgme_new: %s", __func__, gpgme_strerror(gpgerr));
        g_set_error(err, LR_GPG_ERROR, LRE_GPGERROR,
                    "gpgme_new() error: %s", gpgme_strerror(gpgerr));
        return FALSE;
    }

    gpgerr = gpgme_set_protocol(context, GPGME_PROTOCOL_OpenPGP);
    if (gpgerr != GPG_ERR_NO_ERROR) {
        g_debug("%s: gpgme_set_protocol: %s", __func__, gpgme_strerror(gpgerr));
        g_set_error(err, LR_GPG_ERROR, LRE_GPGERROR,
                    "gpgme_set_protocol() error: %s", gpgme_strerror(gpgerr));
        gpgme_release(context);
        return FALSE;
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
            return FALSE;
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
        return FALSE;
    }

    gpgerr = gpgme_new(&context);
    if (gpgerr != GPG_ERR_NO_ERROR) {
        g_debug("%s: gpgme_new: %s", __func__, gpgme_strerror(gpgerr));
        g_set_error(err, LR_GPG_ERROR, LRE_GPGERROR,
                    "gpgme_new() error: %s", gpgme_strerror(gpgerr));
        return FALSE;
    }

    gpgerr = gpgme_set_protocol(context, GPGME_PROTOCOL_OpenPGP);
    if (gpgerr != GPG_ERR_NO_ERROR) {
        g_debug("%s: gpgme_set_protocol: %s", __func__, gpgme_strerror(gpgerr));
        g_set_error(err, LR_GPG_ERROR, LRE_GPGERROR,
                    "gpgme_set_protocol() error: %s", gpgme_strerror(gpgerr));
        gpgme_release(context);
        return FALSE;
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
            return FALSE;
        }
    }

    gpgme_set_armor(context, 1);

    // Key import

    key_fd = open(key_fn, O_RDONLY);
    if (key_fd == -1) {
        g_debug("%s: Opening key: %s", __func__, g_strerror(errno));
        g_set_error(err, LR_GPG_ERROR, LRE_IO,
                    "Error while opening key %s: %s",
                    key_fn, g_strerror(errno));
        gpgme_release(context);
        return FALSE;
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
        return FALSE;
    }

    gpgerr = gpgme_op_import(context, key_data);
    gpgme_data_release(key_data);
    if (gpgerr != GPG_ERR_NO_ERROR) {
        g_debug("%s: gpgme_op_import: %s", __func__, gpgme_strerror(gpgerr));
        g_set_error(err, LR_GPG_ERROR, LRE_GPGERROR,
                    "gpgme_op_import() error: %s", gpgme_strerror(gpgerr));
        gpgme_release(context);
        close(key_fd);
        return FALSE;
    }

    close(key_fd);

    // Running gpg-agent kept opened sockets on the system.
    // It tries to exit gpg-agent. Path to the communication socket is derived from homedir.
    // The gpg-agent automaticaly removes all its socket before exit.
    // Newer gpg-agent creates sockets under [/var]/run/user/{pid}/... if directory exists.
    // In this case gpg-agent will not be exited.
    kill_gpg_agent(context, home_dir);

    gpgme_release(context);

    return TRUE;
}
