#!/usr/bin/env python3

"""
librepo - download packages
"""

import os
import os.path
import time
import librepo

CACHE = "fastestmirror.cache"

LIBREPOPKG = "librepo-1.2.1-2.fc20.x86_64.rpm"
LAMEPKG = "lame-3.99.5-2.fc19.x86_64.rpm"

if __name__ == "__main__":

    # Setup logging
    def debug_function(msg, _):
        print(msg)
    #librepo.set_debug_log_handler(debug_function)

    # Remove packages if already exists
    def remove_pkg(filename):
        if os.path.exists(filename):
            os.remove(filename)
    remove_pkg(LIBREPOPKG)
    remove_pkg(LAMEPKG)

    # Prepare list of targets
    packages = []

    # Prepare first target
    h1 = librepo.Handle()
    h1.metalinkurl = "https://mirrors.fedoraproject.org/metalink?repo=fedora-20&arch=x86_64"
    h1.repotype = librepo.YUMREPO
    h1.fastestmirror = True
    h1.fastestmirrorcache = CACHE
    target = librepo.PackageTarget("Packages/l/"+LIBREPOPKG, handle=h1)
    packages.append(target)

    # Prepare second target
    h2 = librepo.Handle()
    h2.mirrorlisturl = "http://mirrors.rpmfusion.org/mirrorlist?repo=free-fedora-19&arch=x86_64"
    h2.repotype = librepo.YUMREPO
    h2.fastestmirror = True
    h2.fastestmirrorcache = CACHE
    target = librepo.PackageTarget(LAMEPKG, handle=h2)
    packages.append(target)

    t = time.time()
    librepo.download_packages(packages)
    print("Download duration: {0}s\n".format((time.time() - t)))

    for target in packages:
        print("### %s: %s" % (target.local_path, target.err or "OK"))
        print("Local path:        ", target.local_path)
        print("Error:             ", target.err)
        print()
