#!/usr/bin/env python

"""
librepo - download packages from repo with fastestmirror and cache enabled
"""

import os
import os.path
import time
import librepo

METALINKURL = "https://mirrors.fedoraproject.org/metalink?repo=fedora-19&arch=x86_64"
PACKAGEPATH = "Packages/l/"
PACKAGE = "librepo-0.0.4-1.fc19.i686.rpm"
CACHE = "fastestmirror.cache"

def fastestmirrorstatuscallback(userdata, stage, data):
    if stage == librepo.FMSTAGE_INIT:
        print "Fastest mirror detection started.."
    elif stage == librepo.FMSTAGE_CACHELOADING:
        print "Fastest mirror cache used: {0}".format(data)
    elif stage == librepo.FMSTAGE_CACHELOADINGSTATUS:
        if data:
            print "Error while cache loading: {0}".format(data)
        else:
            print "Cache loaded successfully"
    elif stage == librepo.FMSTAGE_DETECTION:
        print "Fastest mirror detection in progress.."
    elif stage == librepo.FMSTAGE_FINISHING:
        print "Fastest mirror detection finishing.."
    elif stage == librepo.FMSTAGE_STATUS:
        if data:
            print "Fastest mirror detection error: {0}".format(data)
        else:
            print "Fastest mirror detection successful"
    else:
        print "UNKNOWN STAGE of Fastest mirror detection"

if __name__ == "__main__":

    # Setup logging
    def debug_function(msg, _):
        print msg
    #librepo.set_debug_log_handler(debug_function)

    def remove_cache():
        if os.path.exists(CACHE):
            print "Removing %s" % CACHE
            os.remove(CACHE)

    def remove_package():
        if os.path.exists(PACKAGE):
            print "Removing %s" % PACKAGE
            os.remove(PACKAGE)

    remove_cache()
    remove_package()

    print "\n1st run: Basic download"
    h = librepo.Handle()
    h.interruptible = True
    h.metalinkurl = METALINKURL
    h.repotype = librepo.YUMREPO
    packages = [librepo.PackageTarget(PACKAGEPATH+PACKAGE, handle=h)]
    t = time.time()
    librepo.download_packages(packages)
    t1 = time.time() - t
    print "### %s: %s (Time: %s)" % (
            packages[0].local_path, packages[0].err or "OK", t1)

    remove_package()

    print "\n2nd run: Download with fastestmirror - cache will be builded"
    h = librepo.Handle()
    h.interruptible = True
    h.metalinkurl = METALINKURL
    h.repotype = librepo.YUMREPO
    h.fastestmirror = True
    h.fastestmirrorcache = CACHE
    h.fastestmirrorcb = fastestmirrorstatuscallback
    packages = [librepo.PackageTarget(PACKAGEPATH+PACKAGE, handle=h)]
    t = time.time()
    librepo.download_packages(packages)
    t2 = time.time() - t
    print "### %s: %s (Time: %s)" % (
            packages[0].local_path, packages[0].err or "OK", t2)

    remove_package()

    print "\n3th run: Download with fastestmirror - cache should be used"
    h = librepo.Handle()
    h.interruptible = True
    h.metalinkurl = METALINKURL
    h.repotype = librepo.YUMREPO
    h.fastestmirror = True
    h.fastestmirrorcache = CACHE
    h.fastestmirrorcb = fastestmirrorstatuscallback
    packages = [librepo.PackageTarget(PACKAGEPATH+PACKAGE, handle=h)]
    t = time.time()
    librepo.download_packages(packages)
    t3 = time.time() - t
    print "### %s: %s (Time: %s)" % (
            packages[0].local_path, packages[0].err or "OK", t3)

    remove_package()
    remove_cache()
