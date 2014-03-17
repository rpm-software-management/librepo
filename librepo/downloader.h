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
 * @param targets   GSList with one or more ::LrDownloadTarget.
 *                  Could be NULL. Then return immediately with LRE_OK
 *                  return code.
 * @param failfast  If True, fail imediatelly after first download failed
 *                  (after max allowed number of retries or all available
 *                  mirrors are tried without success).
 * @param err       GError **
 * @return          If FALSE then err is set.
 *                  Note: If failfast is FALSE, then return value TRUE
 *                  doesn't have to mean that all targets was
 *                  downloaded successfully - You have to check
 *                  status of all downloaded targets.
 */
gboolean
lr_download(GSList *targets, gboolean failfast, GError **err);

/** Wrapper over ::lr_download that takes only single ::LrDownloadTarget.
 * Note: failfast is TRUE, so if download failed, then this function returns
 * FALSE (There is no need to check status of download itself).
 * @param target    single LrDownloadTarget object or NULL.
 * @param err       GError **
 * @return          See ::lr_download
 */
gboolean
lr_download_target(LrDownloadTarget *target, GError **err);

/** Wrapper over the ::lr_download_target that takes only url and fd.
 * After download fd is seeked to the begin of the file.
 * Note: failfast is TRUE, so if download failed, then this function returns
 * FALSE (There is no need to check status of download itself).
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
 * @param targets   See ::lr_download
 * @param failfast  See ::lr_download
 * @param cb        Callback
 * @param err       GError **
 * @return          See ::lr_download
 */
gboolean
lr_download_single_cb(GSList *targets,
                      gboolean failfast,
                      LrProgressCb cb,
                      GError **err);

/** @} */

G_END_DECLS

#endif
