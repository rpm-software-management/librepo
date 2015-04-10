#!/usr/bin/python

"""
Example: Localize metadata files of a local repository

Use case:
We have a local repositository in the rpm-md format
and want to paths to all its metadata files.
Repomd content is just a bonus.
"""

import sys
import librepo
import pprint

METADATA_PATH = "downloaded_metadata"

if __name__ == "__main__":
    h = librepo.Handle()
    r = librepo.Result()
    # Repository with repodata in the rpm-md format
    h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
    # Path to metadata
    h.setopt(librepo.LRO_URLS, [METADATA_PATH])
    # Do not duplicate (copy) metadata, just localise them
    h.setopt(librepo.LRO_LOCAL, True)

    try:
        h.perform(r)
    except librepo.LibrepoException as e:
        rc, msg, general_msg = e
        print "Error: %s" % msg
        sys.exit(1)

    print "Repomd content:"
    pprint.pprint(r.getinfo(librepo.LRR_YUM_REPOMD))

    print "\nPaths to metadata files:"
    for data_type, path in r.getinfo(librepo.LRR_YUM_REPO).iteritems():
        print "%15s: %s" % (data_type, path)

    sys.exit(0)
