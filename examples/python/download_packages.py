#!/usr/bin/env python

"""
librepo - download packages
"""

import librepo

if __name__ == "__main__":

    # Setup logging
    def debug_function(msg, _):
        print msg
    librepo.set_debug_log_handler(debug_function)

    # Prepare handle
    h = librepo.Handle()
    h.url = ["http://beaker-project.org/yum/client-testing/Fedora19/"]
    h.repotype = librepo.YUMREPO

    # Prepare list of targets
    packages = []

    target = librepo.PackageTarget("beaker-0.14.0-1.fc18.src.rpm",
                                   checksum_type=librepo.SHA256,
                                   checksum="737c974110914a073fb6c736cd7021b0d844c9e47e7d21e37d687dbc86d36538",
                                   resume=True)
    packages.append(target)

    target = librepo.PackageTarget("beaker-client-0.14.1-1.fc18.noarch.rpm")
    packages.append(target)

    target = librepo.PackageTarget("rhts-4.56-1.fc17.src.rpm")
    packages.append(target)

    h.download_packages(packages)

    for target in packages:
        print "### %s: %s" % (target.local_path, target.err or "OK")
        print "Relative URL:      ", target.relative_url
        print "Destination:       ", target.dest
        print "Base URL:          ", target.base_url
        print "Checksum type:     ", target.checksum_type
        print "Expected checksum: ", target.checksum
        print "Resume:            ", bool(target.resume)
        print "Local path:        ", target.local_path
        print "Error:             ", target.err
        print

