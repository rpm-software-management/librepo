#!/usr/bin/env python3

"""
Example: Simple download whole repository

This example uses more "pythonic" way of usage.
Instead of use setopt() method it uses class properties.

Use case:
We have a metalink url of a repository and we
want do download complete repository metadata.
"""

import librepo

# Metalink URL
METALINK_URL = "https://mirrors.fedoraproject.org/metalink?repo=fedora-19&arch=x86_64"

# Destination directory (note: This directory must exists!)
DESTDIR = "downloaded_metadata"

if __name__ == "__main__":
    h = librepo.Handle()
    r = librepo.Result()
    # Repository with repodata in the rpm-md format
    h.repotype = librepo.LR_YUMREPO
    # Set metalink url
    h.mirrorlist = METALINK_URL
    # Destination directory for metadata
    h.destdir = DESTDIR
    # Use the fastest mirror
    h.fastestmirror = True

    try:
        h.perform(r)
    except librepo.LibrepoException as e:
        # rc - Return code (integer value)
        # msg - Detailed error message (string)
        # general_msg - Error message based on rc (string)
        rc, msg, general_msg  = e
        print("Error: {}".format(msg))
