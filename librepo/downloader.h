/* librepo - A library providing (libcURL like) API to downloading repository
 * Copyright (C) 2013  Tomas Mlcoch
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

#ifndef LR_DOWNLOADER_H
#define LR_DOWNLOADER_H

#include <glib.h>

#include "handle.h"
#include "downloadtarget.h"

G_BEGIN_DECLS

/** \defgroup   downloader    Downloading API
 *  \addtogroup downloader
 *  @{
 */

/** Global variable signalizing if SIGINT was catched.
 */
extern volatile sig_atomic_t lr_interrupt;

/** SIGINT Signal handler.
 * @param sig       Signal number.
 */
void
lr_sigint_handler(int sig);

/** Main download function.
 * @param handle    ::LrHandle with a user network configuration.
 *                  Could be NULL.
 * @param targets   GSList with one or more ::LrDownloadTarget.
 *                  Could be NULL. Then return immediately with LRE_OK
 *                  return code.
 * @param err       GError **
 * @return          Return TRUE if downloading was completed without error.
 *                  Note: TRUE doesn't mean that all targets was
 *                  downloaded successfully! You have to check
 *                  status of all downloaded targets.
 */
gboolean
lr_download(LrHandle *handle, GSList *targets, GError **err);

/** Wrapper over ::lr_download that takes only single ::LrDownloadTarget.
 * @param handle    See ::lr_download
 * @param target    single LrDownloadTarget object or NULL.
 * @param err       GError **
 * @return          See ::lr_download
 */
gboolean
lr_download_target(LrHandle *handle, LrDownloadTarget *target, GError **err);

/** Wrapper over the ::lr_download_target that takes only url and fd.
 * @param handle    See ::lr_download
 * @param url       URL (absolute or relative)
 * @param fd        Opened file descriptor.
 * @param err       GError **
 * @return          See ::lr_download
 */
gboolean
lr_download_url(LrHandle *handle, const char *url, int fd, GError **err);

/** Wrapper over the ::lr_download that calculate collective statistics of
 * all downloads and repord them via callback. Note: All callbacks and
 * userdata setted in targets will be replaced and don't be used.
 * @param handle    See ::lr_download
 * @param targets   See ::lr_download
 * @param cb        Callback
 * @param cbdata    Callback data
 * @param err       GError **
 * @return          See ::lr_download
 */
gboolean
lr_download_single_cb(LrHandle *handle,
                      GSList *targets,
                      LrProgressCb cb,
                      void *cbdata,
                      GError **err);

/** @} */

G_END_DECLS

#endif
