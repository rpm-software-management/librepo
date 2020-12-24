#!/usr/bin/env python3

"""
librepo - download a package
"""

import os
import sys
import shutil
from pprint import pprint

import librepo

DESTDIR = "downloaded_metadata"
PROGRESSBAR_LEN = 40
finished = False

def callback(data, total_to_download, downloaded):
    """Progress callback"""
    global finished

    if total_to_download != downloaded:
        finished = False

    if total_to_download <= 0 or finished == True:
        return

    completed = int(downloaded / (total_to_download / PROGRESSBAR_LEN))
    print("%30s: [%s%s] %8s/%8s\r" % (data, '#'*completed, '-'*(PROGRESSBAR_LEN-completed), int(downloaded), int(total_to_download)), )
    sys.stdout.flush()

    if total_to_download == downloaded and not finished:
        print()
        finished = True
        return

if __name__ == "__main__":

    pkgs = [
        ("ImageMagick-djvu", "Packages/i/ImageMagick-djvu-6.7.5.6-3.fc17.i686.rpm"),
        ("i2c-tools-eepromer", "Packages/i/i2c-tools-eepromer-3.1.0-1.fc17.i686.rpm")
    ]

    h = librepo.Handle()
    h.setopt(librepo.LRO_URLS, ["http://ftp.linux.ncsu.edu/pub/fedora/linux/releases/17/Everything/i386/os/"])
    h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
    h.setopt(librepo.LRO_PROGRESSCB, callback)
    h.setopt(librepo.LRO_PROGRESSDATA, "")

    for pkg_name, pkg_url in pkgs:
        h.progressdata = pkg_name
        h.download(pkg_url)

