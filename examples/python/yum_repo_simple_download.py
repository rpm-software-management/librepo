#!/usr/bin/python

"""
Example: Simple download whole yum repository

This example uses more "pythonic" way of usage.
Instead of use setopt() method it uses class properties.

Use case:
We have a metalink url of a repository and we
want do download complete repository metadata.
"""

import sys
import librepo

METADATA_PATH = "downloaded_metadata"
URL = "https://mirrors.fedoraproject.org/metalink?repo=fedora-18&arch=x86_64"
DESTDIR = "downloaded_metadata"

if __name__ == "__main__":
    h = librepo.Handle()
    r = librepo.Result()
    # Yum metadata
    h.repotype = librepo.LR_YUMREPO
    # Metalink url
    h.mirrorlist = URL
    # Destination directory for metadata
    h.destdir = DESTDIR

    try:
        h.perform(r)
    except librepo.LibrepoException as e:
        rc, msg, ext = e
        print "Error: %s" % msg
