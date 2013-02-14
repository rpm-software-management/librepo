#!/usr/bin/python

"""
Example: Localise metadata files of local yum repository

Use case:
We have a local yum repositository and want to
paths to all its metadata files.
Repomd content is just a bonus.
"""

import sys
import librepo
import pprint

METADATA_PATH = "downloaded_metadata"

if __name__ == "__main__":
    h = librepo.Handle()
    r = librepo.Result()
    # Yum metadata
    h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
    # Path to metadata
    h.setopt(librepo.LRO_URL, METADATA_PATH)
    # Do not duplicate (copy) metadata, just localise them
    h.setopt(librepo.LRO_LOCAL, True)

    try:
        h.perform(r)
    except librepo.LibrepoException as e:
        rc, msg, ext = e
        print "Error: %s" % msg
        sys.exit(1)

    print "Repomd content:"
    pprint.pprint(r.getinfo(librepo.LRR_YUM_REPOMD))

    print "\nPath to metadata files:"
    for data_type, path in r.getinfo(librepo.LRR_YUM_REPO).iteritems():
        print "%15s: %s" % (data_type, path)

    sys.exit(0)

