#!/usr/bin/env python3

"""
librepo - download packages
"""

import librepo

if __name__ == "__main__":

    # Setup logging
    def debug_function(msg, _):
        print(msg)
    librepo.set_debug_log_handler(debug_function)

    # Prepare handle
    h = librepo.Handle()
    h.urls = ["http://mirror.karneval.cz/pub/linux/fedora/linux/releases/20/Fedora/x86_64/os/"]
    h.repotype = librepo.YUMREPO

    # Prepare list of targets
    packages = []

    target = librepo.PackageTarget("Packages/a/abrt-2.1.9-1.fc20.x86_64.rpm",
                                   handle=h,
                                   dest="abrt.header",
                                   byterangestart=1384,
                                   byterangeend=100204)
    packages.append(target)

    librepo.download_packages(packages)

    for target in packages:
        print("### %s: %s" % (target.local_path, target.err or "OK"))
        print("Relative URL:      ", target.relative_url)
        print("Destination:       ", target.dest)
        print("Base URL:          ", target.base_url)
        print("Checksum type:     ", target.checksum_type)
        print("Expected checksum: ", target.checksum)
        print("Resume:            ", bool(target.resume))
        print("Local path:        ", target.local_path)
        print("Error:             ", target.err)
        print()

