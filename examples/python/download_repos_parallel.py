#!/usr/bin/env python3

"""
librepo - example of usage of download_metadata API
"""

import os
import sys
import shutil

import librepo

DESTDIR = "downloaded_metadata"
PROGRESSBAR_LEN = 50

def hmf_callback_handle(data, msg, url, metadata):
    """Handle mirror failure callback"""
    print("%s: %s: %s (%s)" % (metadata, msg, url, data))
    sys.stdout.flush()

def hmf_callback(data, msg, url):
    """Handle mirror failure callback"""
    print("%s: %s (%s)" % (msg, url, data))
    sys.stdout.flush()

def end_callback(data, status, msg):
    """End callback"""
    print("End status: %s: %s (%s)" % (str(status), str(msg), str(data)))
    sys.stdout.flush()

def progress_callback(data, total_to_download, downloaded):
    """Progress callback"""
    if total_to_download <= 0:
        return
    completed = int(downloaded / (total_to_download / PROGRESSBAR_LEN))
    print(
        "[%s%s] %8s/%8s (%s)\r" % ('#'*completed, '-'*(PROGRESSBAR_LEN-completed), int(downloaded), int(total_to_download), str(data)),
    )
    sys.stdout.flush()

def create_handle(repo):
    # Handle represents a download configuration
    h = librepo.Handle()

    # --- Mandatory arguments -------------------------------------------
    # URL of metalink
    h.setopt(librepo.LRO_METALINKURL, "https://mirrors.fedoraproject.org/metalink?repo=" + repo + "&arch=x86_64")
    # Type of repository
    h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)

    # --- Optional arguments --------------------------------------------
    # Make download interruptible
    h.setopt(librepo.LRO_INTERRUPTIBLE, True)
    # Destination directory for metadata
    h.setopt(librepo.LRO_DESTDIR, DESTDIR + "/" + repo)
    # Check checksum of all files (if checksum is available in repomd.xml)
    h.setopt(librepo.LRO_CHECKSUM, True)
    # Download only primary.xml, comps.xml and updateinfo
    # Note: repomd.xml is downloaded implicitly!
    # Note: If LRO_YUMDLIST is None -> all files are downloaded
    h.setopt(librepo.LRO_YUMDLIST, ["primary", "group", "updateinfo"])

    # Callback to display progress of downloading
    h.setopt(librepo.LRO_PROGRESSCB, progress_callback)
    # Callback to call handle mirror failure
    h.setopt(librepo.LRO_HMFCB, hmf_callback_handle)
    # Set user data for the callback
    h.setopt(librepo.LRO_PROGRESSDATA, "Callback data set in handle")

    return h


if __name__ == "__main__":
    # Prepare destination directory
    if os.path.exists(DESTDIR):
        shutil.rmtree(DESTDIR)
    os.mkdir(DESTDIR)

    cbdata = "Callback data set in metadata target"
    m1 = librepo.MetadataTarget(create_handle("fedora-41"), cbdata, progress_callback, hmf_callback, end_callback)
    m2 = librepo.MetadataTarget(create_handle("updates-released-f41"), cbdata, progress_callback, hmf_callback, end_callback)

    ret = librepo.download_metadata([m1, m2])
    if (ret):
        print(ret, file=sys.stderr)

    if (m1.err):
        print(m1.err, file=sys.stderr)
    if (m2.err):
        print(m2.err, file=sys.stderr)
