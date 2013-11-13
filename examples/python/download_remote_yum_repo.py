#!/usr/bin/python

"""
librepo - example of usage
"""

import os
import sys
import shutil
from pprint import pprint

import librepo

DESTDIR = "downloaded_metadata"
PROGRESSBAR_LEN = 50

def callback(data, total_to_download, downloaded):
    """Progress callback"""
    if total_to_download <= 0:
        return
    completed = int(downloaded / (total_to_download / PROGRESSBAR_LEN))
    print "[%s%s] %8s/%8s (%s)\r" % ('#'*completed, '-'*(PROGRESSBAR_LEN-completed), int(downloaded), int(total_to_download), data),
    sys.stdout.flush()

if __name__ == "__main__":
    # Prepare destination directory
    if os.path.exists(DESTDIR):
        if not os.path.isdir(DESTDIR):
            raise IOError("%s is not a directory" % DESTDIR)
        shutil.rmtree(DESTDIR)
    os.mkdir(DESTDIR)

    h = librepo.Handle() # Handle represents a download configuration
    r = librepo.Result() # Result represents an existing/downloaded repository

    # --- Mandatory arguments -------------------------------------------

    # URL of repository or URL of metalink/mirrorlist
    h.setopt(librepo.LRO_URLS, ["http://ftp.linux.ncsu.edu/pub/fedora/linux/releases/19/Everything/i386/os/"])
    #h.setopt(librepo.LRO_MIRRORLIST, "https://mirrors.fedoraproject.org/metalink?repo=fedora-source-17&arch=i386")
    # Note: LRO_URLS and LRO_MIRRORLIST could be set and used simultaneously
    #       and if download from LRO_URLS failed, then mirrorlist is used

    # Type of repository
    h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)

    # --- Optional arguments --------------------------------------------

    # Make download interruptible
    h.setopt(librepo.LRO_INTERRUPTIBLE, True)

    # Destination directory for metadata
    h.setopt(librepo.LRO_DESTDIR, DESTDIR)

    # Check checksum of all files (if checksum is available in repomd.xml)
    h.setopt(librepo.LRO_CHECKSUM, True)

    # Callback to display progress of downloading
    h.setopt(librepo.LRO_PROGRESSCB, callback)

    # Set user data for the callback
    h.setopt(librepo.LRO_PROGRESSDATA, {'test': 'dict', 'foo': 'bar'})

    # Download only filelists.xml, prestodelta.xml
    # Note: repomd.xml is downloaded implicitly!
    # Note: If LRO_YUMDLIST is None -> all files are downloaded
    h.setopt(librepo.LRO_YUMDLIST, ["filelists", "prestodelta"])

    h.perform(r)

    # Get and show results
    pprint (r.getinfo(librepo.LRR_YUM_REPO))
    pprint (r.getinfo(librepo.LRR_YUM_REPOMD))

    # Whoops... I forget to download primary.xml.. Lets fix it!
    # Set LRO_UPDATE - only update existing Result
    h.setopt(librepo.LRO_UPDATE, True)
    h.setopt(librepo.LRO_YUMDLIST, ["primary"])
    h.perform(r)

    # List of mirrors
    # (In this case no mirrorlist is used -> list will contain only one url)
    # Example of access info via attr insted of .getinfo() method
    pprint (h.mirrors)

    # Get and show final results
    pprint (r.getinfo(librepo.LRR_YUM_REPO))
    pprint (r.getinfo(librepo.LRR_YUM_REPOMD))
