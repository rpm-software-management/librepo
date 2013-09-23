#!/usr/bin/env python

"""
librepo - Example of download_packages() function with use of end,
          failure and mirrorfailure callbacks.
"""

import librepo

if __name__ == "__main__":

    # Setup logging
    #def debug_function(msg, _):
    #    print "DEBUG: %s" % msg
    #librepo.set_debug_log_handler(debug_function)

    # Prepare handle
    h = librepo.Handle()
    h.urls = ["http://beaker-project.org/yum/client-testing/Fedora19/"]
    h.repotype = librepo.YUMREPO

    # Callbacks
    def endcb(data):
        print "EndCb: Download of %s finished" % data

    def failurecb(data, msg):
        print "FailureCb: Download of %s failed with error: %s" % (data, msg)

    def mirrorfailurecb(data, msg):
        print "MirrorFailureCb: Download of %s from mirror failed with: %s" % (data, msg)

    # Prepare list of targets
    packages = []

    target = librepo.PackageTarget("beaker-0.14.0-1.fc18.src.rpm",
                                   handle=h,
                                   checksum_type=librepo.SHA256,
                                   checksum="e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
                                   resume=True,
                                   cbdata="beaker-0.14.0-1.fc18.src.rpm",
                                   endcb=endcb,
                                   failurecb=failurecb,
                                   mirrorfailurecb=mirrorfailurecb)
    packages.append(target)

    target = librepo.PackageTarget("beaker-0.13.2-1.fc17.noarch.rpm",
                                   handle=h,
                                   checksum_type=librepo.SHA256,
                                   checksum="foobar",
                                   cbdata="beaker-0.13.2-1.fc17.noarch.rpm (bad checksum)",
                                   endcb=endcb,
                                   failurecb=failurecb,
                                   mirrorfailurecb=mirrorfailurecb)
    packages.append(target)

    target = librepo.PackageTarget("beaker-0.13.2-1.fc17.src.rpm",
                                   handle=h,
                                   checksum_type=librepo.SHA256,
                                   checksum="xyz",
                                   cbdata="beaker-0.13.2-1.fc17.src.rpm (bad checksum)",
                                   endcb=endcb,
                                   failurecb=failurecb,
                                   mirrorfailurecb=mirrorfailurecb)
    packages.append(target)

    target = librepo.PackageTarget("beaker-client-0.14.1-1.fc18.noarch.rpm",
                                   handle=h,
                                   expectedsize=333333333333333,
                                   cbdata="beaker-client-0.14.1-1.fc18.noarch.rpm (bad size)",
                                   endcb=endcb,
                                   failurecb=failurecb,
                                   mirrorfailurecb=mirrorfailurecb)
    packages.append(target)

    target = librepo.PackageTarget("rhts-4.56-1.fc17.src.rpm_bad_filename",
                                   handle=h,
                                   cbdata="rhts-4.56-1.fc17.src.rpm_bad_filename (bad path)",
                                   endcb=endcb,
                                   failurecb=failurecb,
                                   mirrorfailurecb=mirrorfailurecb)
    packages.append(target)

    librepo.download_packages(packages, failfast=False)

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

