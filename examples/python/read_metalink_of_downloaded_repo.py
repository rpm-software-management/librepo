#!/usr/bin/env python3

"""
librepo - example of usage
"""

import os
import sys
import librepo
import pprint

DESTDIR = "downloaded_metadata"

if __name__ == "__main__":
    h = librepo.Handle()
    r = librepo.Result()

    # Correct repotype is important. Without repotype
    # metalink parser doesn't know suffix which should
    # be stripped off from the mirrors urls.
    h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)

    # Set local mirrorlist file as mirrorlist
    if os.path.isfile(os.path.join(DESTDIR, "mirrorlist")):
        h.mirrorlist = os.path.join(DESTDIR, "mirrorlist")
    elif os.path.isfile(os.path.join(DESTDIR, "metalink.xml")):
        h.mirrorlist = os.path.join(DESTDIR, "metalink.xml")
    else:
        print("No mirrorlist of downloaded repodata available")
        sys.exit(0)

    # Download only the mirrorlist during perform() call.
    h.setopt(librepo.LRO_FETCHMIRRORS, True)

    h.perform(r)

    print("Urls in mirrorlist:")
    print(h.mirrors)
    print("Metalink file content:")
    pprint.pprint(h.metalink)
