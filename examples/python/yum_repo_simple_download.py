#!/usr/bin/python

"""
Example: Simple download whole yum repository

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
    h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
    # Metalink url
    h.setopt(librepo.LRO_MIRRORLIST, URL)
    # Destination directory for metadata
    h.setopt(librepo.LRO_DESTDIR, DESTDIR)

    try:
        h.perform(r)
    except librepo.LibrepoException as e:
        rc, msg, ext = e
        print "Error: %s" % msg
        sys.exit(1)
    sys.exit(0)

