.. _examples:

Examples
========

Librepo usage examples.

[More examples (including C examples)](https://github.com/Tojaj/librepo/tree/master/examples)


Simple download of metadata
---------------------------

::

    """
    Example: Simple download whole yum repository

    This example uses more "pythonic" way of usage.
    Instead of use setopt() method it uses class properties.

    Use case:
    We have a metalink url of a repository and we
    want do download complete repository metadata.
    """

    import librepo

    # Metalink URL
    METALINK_URL = "https://mirrors.fedoraproject.org/metalink?repo=fedora-18&arch=x86_64"

    # Destination directory (note: This directory must exists!)
    DESTDIR = "downloaded_metadata"

    if __name__ == "__main__":
        h = librepo.Handle()
        r = librepo.Result()
        # Set type of repo to Yum
        h.repotype = librepo.LR_YUMREPO
        # Set metalink url
        h.mirrorlist = METALINK_URL
        # Destination directory for metadata
        h.destdir = DESTDIR

        try:
            h.perform(r)
        except librepo.LibrepoException as e:
            # rc - Return code (integer value)
            # msg - Detailed error message (string)
            # general_msg - Error message based on rc (string)
            rc, msg, general_msg  = e
            print "Error: %s" % msg

Metadata localisation
---------------------

::

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

Checksum verification
---------------------

.. note::
    Checksum verification is enabled by default. So command
    ``h.setopt(librepo.LRO_CHECKSUM, True)`` is unnecessary, but
    for illustration it is better to write the command anyway.

::

    """
    Example: Verify checksum of local yum metadata

    Use case:
    We have some incomplete yum metadata localy.
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
        # Yum metadata
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

More complex download
---------------------

::

    """
    librepo - example of usage
    """

    import os
    import sys
    import shutil
    from pprint import pprint

    import librepo

    DESTDIR = "downloaded_metadata"
    PROGRESSBAR_LEN = 50

    def callback(data, total_to_download, downloaded):
        """Progress callback"""
        if total_to_download <= 0:
            return
        completed = int(downloaded / (total_to_download / PROGRESSBAR_LEN))
        print "[%s%s] %8s/%8s (%s)\r" % ('#'*completed, '-'*(PROGRESSBAR_LEN-completed), int(downloaded), int(total_to_download), data),
        sys.stdout.flush()

    if __name__ == "__main__":
        # Prepare destination directory
        if os.path.exists(DESTDIR):
            if not os.path.isdir(DESTDIR):
                raise IOError("%s is not a directory" % DESTDIR)
            shutil.rmtree(DESTDIR)
        os.mkdir(DESTDIR)

        h = librepo.Handle() # Handle represents a download configuration
        r = librepo.Result() # Result represents an existing/downloaded repository

        # --- Mandatory arguments -------------------------------------------

        # URL of repository or URL of metalink/mirrorlist
        h.setopt(librepo.LRO_URLS, ["http://ftp.linux.ncsu.edu/pub/fedora/linux/releases/17/Everything/i386/os/"])
        #h.setopt(librepo.LRO_MIRRORLIST, "https://mirrors.fedoraproject.org/metalink?repo=fedora-source-17&arch=i386")
        # Note: LRO_URLS and LRO_MIRRORLIST could be set and used simultaneously
        #       and if download from LRO_URLS failed, then mirrorlist is used

        # Type of repository
        h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)

        # --- Optional arguments --------------------------------------------

        # Make download interruptible
        h.setopt(librepo.LRO_INTERRUPTIBLE, True)

        # Destination directory for metadata
        h.setopt(librepo.LRO_DESTDIR, DESTDIR)

        # Check checksum of all files (if checksum is available in repomd.xml)
        h.setopt(librepo.LRO_CHECKSUM, True)

        # Callback to display progress of downloading
        h.setopt(librepo.LRO_PROGRESSCB, callback)

        # Set user data for the callback
        h.setopt(librepo.LRO_PROGRESSDATA, {'test': 'dict', 'foo': 'bar'})

        # Download only filelists.xml, prestodelta.xml
        # Note: repomd.xml is downloaded implicitly!
        # Note: If LRO_YUMDLIST is None -> all files are downloaded
        h.setopt(librepo.LRO_YUMDLIST, ["filelists", "prestodelta"])

        h.perform(r)

        # Get and show results
        pprint (r.getinfo(librepo.LRR_YUM_REPO))
        pprint (r.getinfo(librepo.LRR_YUM_REPOMD))

        # Whoops... I forget to download primary.xml.. Lets fix it!
        # Set LRO_UPDATE - only update existing Result
        h.setopt(librepo.LRO_UPDATE, True)
        h.setopt(librepo.LRO_YUMDLIST, ["primary"])
        h.perform(r)

        # List of mirrors
        # (In this case no mirrorlist is used -> list will contain only one url)
        # Example of access info via attr insted of .getinfo() method
        pprint (h.mirrors)

        # Get and show final results
        pprint (r.getinfo(librepo.LRR_YUM_REPO))
        pprint (r.getinfo(librepo.LRR_YUM_REPOMD))

How to get urls in a local mirrorlist
-------------------------------------

::

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
            print "No mirrorlist of downloaded repodata available"
            sys.exit(0)

        # Download only the mirrorlist during perform() call.
        h.setopt(librepo.LRO_FETCHMIRRORS, True)

        h.perform(r)

        print "Urls in mirrorlist:"
        print h.mirrors
        print "Metalink file content:"
        pprint.pprint(h.metalink)

