#!/usr/bin/python

"""
Example: Verify checksum of local metadata

Use case:
We have some incomplete metadata localy.
They are incomplete because they doesn't
contain all files specified in repomd.xml.
They contains only primary.xml and filelists.xml.
We want to check checksum of this metadata.
"""

import sys
import librepo

METADATA_PATH = "downloaded_metadata"

if __name__ == "__main__":
    h = librepo.Handle()
    r = librepo.Result()
    # Repository with repodata in the rpm-md format
    h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
    # Path to the metadata
    h.setopt(librepo.LRO_URLS, [METADATA_PATH])
    # Do not duplicate (copy) the metadata
    h.setopt(librepo.LRO_LOCAL, True)
    # Check checksum of metadata
    h.setopt(librepo.LRO_CHECKSUM, True)
    # Ignore missing metadata files
    h.setopt(librepo.LRO_IGNOREMISSING, True)

    try:
        h.perform(r)
    except librepo.LibrepoException as e:
        rc, msg, general_msg = e
        if rc == librepo.LRE_BADCHECKSUM:
            print "Corrupted metadata! (%s)" % msg
        else:
            print "Other error: %s" % msg
        sys.exit(1)

    print "Metadata are fine!"
