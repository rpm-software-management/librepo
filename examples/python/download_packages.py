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
    h.urls = ["http://beaker-project.org/yum/client-testing/Fedora21/"]
    h.repotype = librepo.YUMREPO

    # Prepare list of targets
    packages = []

    target = librepo.PackageTarget("beaker-19.1-1.fc19.src.rpm",
                                   handle=h,
                                   checksum_type=librepo.SHA256,
                                   checksum="3a07327702d15de2707518eb777c0837b2fc7c0cf4cb757331982f5ce6053867",
                                   resume=True)
    packages.append(target)

    target = librepo.PackageTarget("beaker-client-19.1-1.fc19.noarch.rpm",
                                   handle=h,
                                   expectedsize=381148)
    packages.append(target)

    target = librepo.PackageTarget("rhts-4.65-1.fc19.src.rpm", handle=h)
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
