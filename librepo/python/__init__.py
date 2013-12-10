"""

Exceptions
==========

Librepo module has only one own exception.

.. class:: LibrepoException

Value of this exception is tuple with three elements:
``(return code, error message, general error message)``

* Return code is a value from: :ref:`error-codes-label`.
* String with a descriptive description of the error.
* General error message based on rc (feel free to ignore this message)

Constants
=========

.. data:: VERSION_MAJOR
          VERSION_MINOR
          VERSION_PATCH
          VERSION

    Constants with version numbers and whole version string.

.. _handle-options-label:

:class:`~.Handle` options
-------------------------

    LRO_ (aka LibRepo Option) prefixed constants are used to set
    :class:`.Handle` options via :meth:`~.Handle.setopt` method.

    This options could be also set by :class:`.Handle` attributes
    with the same names but in lowercase and without ``LRO_`` prefix.

    Example::

            # The command:
            h.setopt(librepo.LRO_URLS, ["http://ftp.linux.ncsu.edu/pub/fedora/linux/releases/17/Everything/i386/os/"])
            # is equivalent to:
            h.urls = "http://ftp.linux.ncsu.edu/pub/fedora/linux/releases/17/Everything/i386/os/"

    .. note:: For detailed description of this options consult :class:`.Handle` page.

.. data:: LRO_UPDATE

    *Boolean*. Set to ``True`` if only want update localised or downloaded
    repository represented by :class:`.Result` object. Update mode is
    meant to download previously omitted repository file(s).

.. data:: LRO_URLS

    *List or None*. Set repository urls (repository url and baseurl
    are interchangeable terms in this context).

.. data:: LRO_MIRRORLIST

    *String or None*. **DEPRECATED** Set mirrorlist url (url could point
    to a metalink mirrorlist or to a simple mirrorlist where each line wihtout
    ``'#'`` is considered as mirror url).

.. data:: LRO_MIRRORLISTURL

    *String or None*. Mirrorlist URL.

.. data:: LRO_METALINKURL

    *String or None*. Metalink URL.

.. data:: LRO_LOCAL

    *Boolean*. If set to True, no local copy of repository is created
    and repository is just localised in its current location.
    When True, url of repository MUST be a local address
    (e.g. '/home/user/repo' or 'file:///home/user/repo').

.. data:: LRO_HTTPAUTH

    *Boolean*. If True, all supported methods of HTTP authentication
    are enabled.

.. data:: LRO_USERPWD

    *String or None*. Set username and password for HTTP authentication.
    Param must be in format 'username:password'.

.. data:: LRO_PROXY

    *String or None*. Set proxy server address. Port could be
    specified by different option or by the :[port] suffix of
    this address. Any protocol prefix (``http://``, ...) will be ignored.

.. data:: LRO_PROXYPORT

    *Integer or None*. Set proxy port number to connect unsless
    it is specified in the proxy address string. None sets default
    value 1080.

.. data:: LRO_PROXYTYPE

    *Boolean*. Set type of proxy - could be one of :ref:`proxy-type-label`

.. data:: LRO_PROXYAUTH

    *Boolean*. If True, all supported proxy authentication methods are enabled.
    If False, only basic authentication method is enabled.

.. data:: LRO_PROXYUSERPWD

    *String or None*. Set username and password for proxy
    authentication in format 'username:password'.

.. data:: LRO_PROGRESSCB

    *Function or None*. Set progress callback. Callback must be in format:
    ``callback(userdata, total_to_download, downloaded)``. If
    total_to_download is 0, then total size is not known.
    Total size (total_to_download) could change (grow) among two callback
    calls (if some download failed and another mirror is tried).

.. data:: LRO_PROGRESSDATA

    *Any object*. Set user data for the progress callback.

.. data:: LRO_MAXSPEED

    *Long or None*. Set maximal allowed speed per download in bytes per second.
    0 = unlimited speed - the default value.

.. data:: LRO_DESTDIR

    *String or None*. Set destination directory for downloaded data
    (metadata or packages).

.. data:: LRO_REPOTYPE

    *Integer*. One of :ref:`repotype-constants-label`. See more
    :meth:`~.Handle.repotype` Set type of repository.

.. data:: LRO_CONNECTTIMEOUT

    *Integer or None. Set maximal timeout in sec for connection phase.
    Default value is 300. None as *val* sets the default value.

.. data:: LRO_IGNOREMISSING

    *Boolean*. If you want to localise (LRO_LOCAL is True) a incomplete local
    repository (eg. only primary and filelists are present but repomd.xml
    contains more files), you could use LRO_YUMDLIST and specify only file
    that are present, or use LRO_YUMBLIST and specify files that are not
    present or use this option.

.. data:: LRO_INTERRUPTIBLE

    *Boolean*. Librepo sets up its own SIGTERM handler. If the SIGTERM signal
    is catched, the current download is interrupted.

.. data:: LRO_USERAGENT

    *String*. String for  User-Agent: header in the http request sent
    to the remote server.

.. data:: LRO_FETCHMIRRORS

    *Boolean*. With this option enabled, only mirrorlist download and parsing
    will be performed during :meth:`librepo.Handle.perform`.

.. data:: LRO_MAXMIRRORTRIES

    *Integer or None*. If download fails, try at most the specified number
    of mirrors. 0 (None) means try all available mirrors.

.. data:: LRO_MAXPARALLELDOWNLOADS

    *Integer or None*. Maximum number of parallel downloads.
    ``None`` sets default value.

.. data:: LRO_MAXDOWNLOADSPERMIRROR

    *Integer or None*. Maximum number of parallel downloads per mirror.
    ``None`` sets default value.

.. data:: LRO_VARSUB

    *[(String, String), ...] or None*. Set list of substitutions
    for variables in ulrs (e.g.: "http://foo/$version/").
    ``[("releasever", "f18"), ("basearch", "i386")]``

.. data:: LRO_FASTESTMIRROR

    *Boolean*. If True, internal mirrorlist is sorted
    by the determined connection speed, after it is constructed.

.. data:: LRO_FASTESTMIRRORCACHE

    *String or None*. Path to the cache file. If cache file it doesn't exists
    it will be created.

.. data:: LRO_FASTESTMIRRORMAXAGE

    *Integer or None*. Max age of cache record. Older records will not be used.

.. data:: LRO_FASTESTMIRRORCB

    *Function or None*. Fastest mirror status callback.
    Its prototype looks like ``callback(userdata, stage, data)``
    Where *userdata* are data passed by user via LRO_FASTESTMIRRORDATA.
    *stage* is value from
    :ref:`fastestmirror-stages-constants-label`. *data* value depends
    on *stage* value. See the list of available stages.

.. data:: LRO_FASTESTMIRRORDATA

    *Any object*. User data for fastest mirror status callback.

.. data:: LRO_LOWSPEEDTIME

    *Integer or None*. The time in seconds that the transfer should be below
    the LRO_LOWSPEEDLIMIT for the library to consider it too slow and abort.
    Default: 10 (sec)

.. data:: LRO_LOWSPEEDLIMIT

    *Integer or None*. The transfer speed in bytes per second that
    the transfer should be below during LRO_LOWSPEEDTIME seconds for
    the library to consider it too slow and abort. Default: 1000 (byte/s)

.. data:: LRO_GPGCHECK

    *Boolean*. Set True to enable gpg check (if available) of downloaded repo.

.. data:: LRO_CHECKSUM

    *Boolean*. Set False/True to disable/enable checksum check.

    .. note:: Checksum check is enabled by default.
    .. note::
        If checksum check is disabled, then even explicitly specified
        checksum related params e.g. in :meth:`~librepo.Handle.download`
        method are ignored and checksum is not checked!

.. data:: LRO_YUMDLIST

    *List of strings*. Some predefined list :ref:`predefined-yumdlists-label`.
    Set list of yum metadata files to download. e.g. ``["primary",
    "filelists", "other", "primary_db", "prestodelta"]`` If *val* is None,
    all metadata files will be downloaded. If *val* is ``[]`` or ``[None]``
    only ``repomd.xml`` will be downloaded.

.. data:: LRO_YUMBLIST

    *List of strings*. Set blacklist of yum metadata files.
    This files will not be downloaded.


.. _handle-info-options-label:

:class:`~.Handle` info options
------------------------------

    LRI (aka LibRepo Information) constants are used to get information
    from :class:`.Handle` via :meth:`~.Handle.getinfo` method.

.. data:: LRI_UPDATE
.. data:: LRI_URLS
.. data:: LRI_MIRRORLIST
.. data:: LRI_MIRRORLISTURL
.. data:: LRI_METALINKURL
.. data:: LRI_LOCAL
.. data:: LRI_PROGRESSCB
.. data:: LRI_PROGRESSDATA
.. data:: LRI_DESTDIR
.. data:: LRI_REPOTYPE
.. data:: LRI_USERAGENT
.. data:: LRI_YUMDLIST
.. data:: LRI_YUMBLIST
.. data:: LRI_FETCHMIRRORS
.. data:: LRI_MAXMIRRORTRIES
.. data:: LRI_VARSUB
.. data:: LRI_MIRRORS
.. data:: LRI_METALINK
.. data:: LRI_FASTESTMIRROR
.. data:: LRI_FASTESTMIRRORCACHE
.. data:: LRI_FASTESTMIRRORMAXAGE

.. _proxy-type-label:

Proxy type constants
--------------------

.. data:: PROXY_HTTP (LR_PROXY_HTTP)
.. data:: PROXY_HTTP_1_0 (LR_PROXY_HTTP_1_0)
.. data:: PROXY_SOCKS4 (LR_PROXY_SOCKS4)
.. data:: PROXY_SOCKS5 (LR_PROXY_SOCKS5)
.. data:: PROXY_SOCKS4A (LR_PROXY_SOCKS4A)
.. data:: PROXY_SOCKS5_HOSTNAME (LR_PROXY_SOCKS5_HOSTNAME)

.. _repotype-constants-label:

Repo type constants
-------------------

.. data:: YUMREPO (LR_YUMREPO)

    Classical yum repository with ``repodata/`` directory.

.. data:: SUSEREPO (LR_SUSEREPO)

    YaST2 repository (http://en.opensuse.org/openSUSE:Standards_YaST2_Repository_Metadata_content).

    .. note:: This option is not supported yet!

.. data:: DEBREPO (LR_DEBREPO)

    Debian repository

    .. note:: This option is not supported yet!

.. _predefined-yumdlists-label:

Predefined yumdlist lists
-------------------------

.. data:: YUM_FULL (LR_YUM_FULL)

    Download all repodata files

.. data:: YUM_REPOMDONLY (LR_YUM_REPOMDONLY)

    Download only repomd.xml file

.. data:: YUM_BASEXML (LR_YUM_BASEXML)

    Download only primary.xml, filelists.xml and other.xml

.. data:: YUM_BASEDB (LR_YUM_BASEDB)

    Download only primary, filelists and other databases.

.. data:: YUM_HAWKEY (LR_YUM_HAWKEY)

    Download only files used by Hawkey (https://github.com/akozumpl/hawkey/).
    (primary, filelists, prestodelta)

.. _error-codes-label:


.. _fastestmirror-stages-constants-label:

Fastest mirror stages
---------------------

Values used by fastest mirror callback (:data:`~.LRO_FASTESTMIRRORCB`):

.. data:: FMSTAGE_INIT

    (0) Fastest mirror detection just started. *data* is None.

.. data:: FMSTAGE_CACHELOADING

    (1) Cache file is specified. *data* is path to the cache file.

.. data:: FMSTAGE_CACHELOADINGSTATUS

    (2) Cache loading finished. If successfull, *data* is None, otherwise
        *data* is string with error message.

.. data:: FMSTAGE_DETECTION

    (3) Detection in progress. *data* is number of mirrors that will be pinged.
    If all times were loaded from cache, this stage is skiped.

.. data:: FMSTAGE_FINISHING

    (4) Detection is done, sorting mirrors, updating cache, etc.
        *data* is None.

.. data:: FMSTAGE_STATUS

    (5) The very last invocation of fastest mirror callback.
        If fastest mirror detection was successfull *data*,
        otherwise *data* contain string with error message.

Error codes
-----------

LibRepo Error codes.

.. data:: LRE_OK

    (0) Everything is ok.

.. data:: LRE_BADFUNCARG

    (1) Bad function argument(s).

.. data:: LRE_BADOPTARG

    (2) Bad argument for the option in :meth:`~.Handle.setopt`.

.. data:: LRE_UNKNOWNOPT

    (3) Unknown option.

.. data:: LRE_CURLSETOPT

    (4) cURL doesn't know an option used by librepo. Probably
        too old cURL version is used.

.. data:: LRE_ALREADYUSEDRESULT

    (5) :class:`.Result` object is not "clean" (it has been already used).

.. data:: LRE_INCOMPLETERESULT

    (6) :class:`.Result` object doesn't contain all what is needed.

.. data:: LRE_CURLDUP

    (7) Cannot duplicate cURL handle. No free memory?

.. data:: LRE_CURL

    (8) A cURL error.

.. data:: LRE_CURLM

    (9) A multi cURL handle error.

.. data:: LRE_BADSTATUS

    (10) Error HTTP or FTP status code.

.. data:: LRE_TEMPORARYERR

    (11) An error that should be temporary (e.g. HTTP status codes 500, 502-504, ..)

.. data:: LRE_NOTLOCAL

    (12) URL is not a local address.
        E.g. in case when :data:`~.LRO_UPDATE` option is ``True`` and URL
        is a remote address.

.. data:: LRE_CANNOTCREATEDIR

    (13) Cannot create directory for downloaded data. Bad permission?

.. data:: LRE_IO

    (14) Input/Output error.

.. data:: LRE_MLBAD

    (15) Bad mirrorlist or metalink file. E.g. metalink doesn't contain
        reference to target file (repomd.xml), mirrorlist is empty, ..

.. data:: LRE_MLXML

    (16) Cannot parse metalink xml. Non-valid metalink file?
        E.g. (metalink doesn't contain needed
        file, mirrorlist doesn't contain urls, ..)

.. data:: LRE_BADCHECKSUM

    (17) Bad checksum.

.. data:: LRE_REPOMDXML

    (18) Cannot parse repomd.xml file. Non-valid repomd.xml file?

.. data:: LRE_NOURL

    (19) No usable URL found. E.g. bad links or no links in metalink.

.. data:: LRE_CANNOTCREATETMP

    (20) Cannot create temporary directory.

.. data:: LRE_UNKNOWNCHECKSUM

    (21) Unknown type of checksum is needed for verification of one
        or more files.

.. data:: LRE_BADURL

    (22) Bad URL specified.

.. data:: LRE_GPGNOTSUPPORTED

    (23) OpenPGP protocol is not supported.

.. data:: LRE_GPGERROR

    (24) GPG error.

.. data:: LRE_BADGPG

    (25) Bad GPG signature.

.. data:: LRE_INCOMPLETEREPO

    (26) Repository metadata are not complete.

.. data:: LRE_INTERRUPTED

    (27) Download was interrupted by SIGTERM signal.

.. data:: LRE_SIGACTION

    (28) Sigaction system call failed.

.. data:: LRE_ALREADYDOWNLOADED

    (29) The file is already downloaded and its checksum matches.

.. data:: LRE_UNFINISHED

    (30) Download wasn't (or cannot be) finished.

.. data:: LRE_SELECT

    (31) select() call failed.

.. data:: LRE_OPENSSL

    (32) OpenSSL library related error.

.. data:: LRE_MEMORY

    (33) Cannot allocate more memory.

.. data:: LRE_XMLPARSER

    (34) XML parser error.

.. data:: LRE_CBINTERRUPTED

    (35) Interrupted by user cb.

.. data:: LRE_UNKNOWNERROR

    An unknown error.

.. _result-options-label:

:class:`~.Result` options
-------------------------

LibRepo Result options for use in :meth:`~.Result.getinfo` method.

.. data:: LRR_YUM_REPO

    Return a dict with local paths to downloaded/localised
    yum repository.

.. data:: LRR_YUM_REPOMD

    Return a dict representing a repomd.xml file of downloaded
    yum repository.

.. data:: LRR_YUM_TIMESTAMP

    Return the highest timestamp from all records in the repomd.
    See: http://yum.baseurl.org/gitweb?p=yum.git;a=commitdiff;h=59d3d67f

Transfer statuses for endcb of :class:`~.PackageTarget`
-------------------------------------------------------

.. data:: TRANSFER_SUCCESSFUL
.. data:: TRANSFER_ALREADYEXISTS
.. data:: TRANSFER_ERROR

"""

import librepo._librepo

VERSION_MAJOR = _librepo.VERSION_MAJOR
VERSION_MINOR = _librepo.VERSION_MINOR
VERSION_PATCH = _librepo.VERSION_PATCH
VERSION = u"%d.%d.%d" % (VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH)

LibrepoException = _librepo.LibrepoException

LRO_UPDATE                  = _librepo.LRO_UPDATE
LRO_URLS                    = _librepo.LRO_URLS
LRO_MIRRORLIST              = _librepo.LRO_MIRRORLIST
LRO_MIRRORLISTURL           = _librepo.LRO_MIRRORLISTURL
LRO_METALINKURL             = _librepo.LRO_METALINKURL
LRO_LOCAL                   = _librepo.LRO_LOCAL
LRO_HTTPAUTH                = _librepo.LRO_HTTPAUTH
LRO_USERPWD                 = _librepo.LRO_USERPWD
LRO_PROXY                   = _librepo.LRO_PROXY
LRO_PROXYPORT               = _librepo.LRO_PROXYPORT
LRO_PROXYTYPE               = _librepo.LRO_PROXYTYPE
LRO_PROXYAUTH               = _librepo.LRO_PROXYAUTH
LRO_PROXYUSERPWD            = _librepo.LRO_PROXYUSERPWD
LRO_PROGRESSCB              = _librepo.LRO_PROGRESSCB
LRO_PROGRESSDATA            = _librepo.LRO_PROGRESSDATA
LRO_MAXSPEED                = _librepo.LRO_MAXSPEED
LRO_DESTDIR                 = _librepo.LRO_DESTDIR
LRO_REPOTYPE                = _librepo.LRO_REPOTYPE
LRO_CONNECTTIMEOUT          = _librepo.LRO_CONNECTTIMEOUT
LRO_IGNOREMISSING           = _librepo.LRO_IGNOREMISSING
LRO_INTERRUPTIBLE           = _librepo.LRO_INTERRUPTIBLE
LRO_USERAGENT               = _librepo.LRO_USERAGENT
LRO_FETCHMIRRORS            = _librepo.LRO_FETCHMIRRORS
LRO_MAXMIRRORTRIES          = _librepo.LRO_MAXMIRRORTRIES
LRO_MAXPARALLELDOWNLOADS    = _librepo.LRO_MAXPARALLELDOWNLOADS
LRO_MAXDOWNLOADSPERMIRROR   = _librepo.LRO_MAXDOWNLOADSPERMIRROR
LRO_VARSUB                  = _librepo.LRO_VARSUB
LRO_FASTESTMIRROR           = _librepo.LRO_FASTESTMIRROR
LRO_FASTESTMIRRORCACHE      = _librepo.LRO_FASTESTMIRRORCACHE
LRO_FASTESTMIRRORMAXAGE     = _librepo.LRO_FASTESTMIRRORMAXAGE
LRO_FASTESTMIRRORCB         = _librepo.LRO_FASTESTMIRRORCB
LRO_FASTESTMIRRORDATA       = _librepo.LRO_FASTESTMIRRORDATA
LRO_LOWSPEEDTIME            = _librepo.LRO_LOWSPEEDTIME
LRO_LOWSPEEDLIMIT           = _librepo.LRO_LOWSPEEDLIMIT
LRO_GPGCHECK                = _librepo.LRO_GPGCHECK
LRO_CHECKSUM                = _librepo.LRO_CHECKSUM
LRO_YUMDLIST                = _librepo.LRO_YUMDLIST
LRO_YUMBLIST                = _librepo.LRO_YUMBLIST
LRO_SENTINEL                = _librepo.LRO_SENTINEL

ATTR_TO_LRO = {
    "update":               LRO_UPDATE,
    "urls":                 LRO_URLS,
    "mirrorlist":           LRO_MIRRORLIST,
    "mirrorlisturl":        LRO_MIRRORLISTURL,
    "metalinkurl":          LRO_METALINKURL,
    "local" :               LRO_LOCAL,
    "httpauth":             LRO_HTTPAUTH,
    "userpwd":              LRO_USERPWD,
    "proxy":                LRO_PROXY,
    "proxyport":            LRO_PROXYPORT,
    "proxytype":            LRO_PROXYTYPE,
    "proxyauth":            LRO_PROXYAUTH,
    "proxyuserpwd":         LRO_PROXYUSERPWD,
    "progresscb":           LRO_PROGRESSCB,
    "progressdata":         LRO_PROGRESSDATA,
    "maxspeed":             LRO_MAXSPEED,
    "destdir":              LRO_DESTDIR,
    "repotype":             LRO_REPOTYPE,
    "connecttimeout":       LRO_CONNECTTIMEOUT,
    "ignoremissing":        LRO_IGNOREMISSING,
    "interruptible":        LRO_INTERRUPTIBLE,
    "useragent":            LRO_USERAGENT,
    "fetchmirrors":         LRO_FETCHMIRRORS,
    "maxmirrortries":       LRO_MAXMIRRORTRIES,
    "maxparalleldownloads": LRO_MAXPARALLELDOWNLOADS,
    "maxdownloadspermirror":LRO_MAXDOWNLOADSPERMIRROR,
    "varsub":               LRO_VARSUB,
    "fastestmirror":        LRO_FASTESTMIRROR,
    "fastestmirrorcache":   LRO_FASTESTMIRRORCACHE,
    "fastestmirrormaxage":  LRO_FASTESTMIRRORMAXAGE,
    "fastestmirrorcb":      LRO_FASTESTMIRRORCB,
    "fastestmirrordata":    LRO_FASTESTMIRRORDATA,
    "lowspeedtime":         LRO_LOWSPEEDTIME,
    "lowspeedlimit":        LRO_LOWSPEEDLIMIT,
    "gpgcheck":             LRO_GPGCHECK,
    "checksum":             LRO_CHECKSUM,
    "yumdlist":             LRO_YUMDLIST,
    "yumblist":             LRO_YUMBLIST,
}

LRI_UPDATE              = _librepo.LRI_UPDATE
LRI_URLS                = _librepo.LRI_URLS
LRI_MIRRORLIST          = _librepo.LRI_MIRRORLIST
LRI_MIRRORLISTURL       = _librepo.LRI_MIRRORLISTURL
LRI_METALINKURL         = _librepo.LRI_METALINKURL
LRI_LOCAL               = _librepo.LRI_LOCAL
LRI_PROGRESSCB          = _librepo.LRI_PROGRESSCB
LRI_PROGRESSDATA        = _librepo.LRI_PROGRESSDATA
LRI_DESTDIR             = _librepo.LRI_DESTDIR
LRI_REPOTYPE            = _librepo.LRI_REPOTYPE
LRI_USERAGENT           = _librepo.LRI_USERAGENT
LRI_YUMDLIST            = _librepo.LRI_YUMDLIST
LRI_YUMBLIST            = _librepo.LRI_YUMBLIST
LRI_FETCHMIRRORS        = _librepo.LRI_FETCHMIRRORS
LRI_MAXMIRRORTRIES      = _librepo.LRI_MAXMIRRORTRIES
LRI_VARSUB              = _librepo.LRI_VARSUB
LRI_MIRRORS             = _librepo.LRI_MIRRORS
LRI_METALINK            = _librepo.LRI_METALINK
LRI_FASTESTMIRROR       = _librepo.LRI_FASTESTMIRROR
LRI_FASTESTMIRRORCACHE  = _librepo.LRI_FASTESTMIRRORCACHE
LRI_FASTESTMIRRORMAXAGE = _librepo.LRI_FASTESTMIRRORMAXAGE

ATTR_TO_LRI = {
    "update":               LRI_UPDATE,
    "urls":                 LRI_URLS,
    "mirrorlist":           LRI_MIRRORLIST,
    "mirrorlisturl":        LRI_MIRRORLISTURL,
    "metalinkurl":          LRI_METALINKURL,
    "local":                LRI_LOCAL,
    "progresscb":           LRI_PROGRESSCB,
    "progressdata":         LRI_PROGRESSDATA,
    "destdir":              LRI_DESTDIR,
    "repotype":             LRI_REPOTYPE,
    "useragent":            LRI_USERAGENT,
    "yumdlist":             LRI_YUMDLIST,
    "yumblist":             LRI_YUMBLIST,
    "fetchmirrors":         LRI_FETCHMIRRORS,
    "maxmirrortries":       LRI_MAXMIRRORTRIES,
    "varsub":               LRI_VARSUB,
    "mirrors":              LRI_MIRRORS,
    "metalink":             LRI_METALINK,
    "fastestmirror":        LRI_FASTESTMIRROR,
    "fastestmirrorcache":   LRI_FASTESTMIRRORCACHE,
    "fastestmirrormaxage":  LRI_FASTESTMIRRORMAXAGE,
}

LR_CHECK_GPG        = _librepo.LR_CHECK_GPG
LR_CHECK_CHECKSUM   = _librepo.LR_CHECK_CHECKSUM

CHECK_GPG        = LR_CHECK_GPG
CHECK_CHECKSUM   = LR_CHECK_CHECKSUM

LR_YUMREPO  = _librepo.LR_YUMREPO
LR_SUSEREPO = _librepo.LR_SUSEREPO
LR_DEBREPO  = _librepo.LR_DEBREPO

YUMREPO  = LR_YUMREPO
SUSEREPO = LR_SUSEREPO
DEBREPO  = LR_DEBREPO

LR_PROXY_HTTP               = _librepo.LR_PROXY_HTTP
LR_PROXY_HTTP_1_0           = _librepo.LR_PROXY_HTTP_1_0
LR_PROXY_SOCKS4             = _librepo.LR_PROXY_SOCKS4
LR_PROXY_SOCKS5             = _librepo.LR_PROXY_SOCKS5
LR_PROXY_SOCKS4A            = _librepo.LR_PROXY_SOCKS4A
LR_PROXY_SOCKS5_HOSTNAME    = _librepo.LR_PROXY_SOCKS5_HOSTNAME

PROXY_HTTP               = _librepo.LR_PROXY_HTTP
PROXY_HTTP_1_0           = _librepo.LR_PROXY_HTTP_1_0
PROXY_SOCKS4             = _librepo.LR_PROXY_SOCKS4
PROXY_SOCKS5             = _librepo.LR_PROXY_SOCKS5
PROXY_SOCKS4A            = _librepo.LR_PROXY_SOCKS4A
PROXY_SOCKS5_HOSTNAME    = _librepo.LR_PROXY_SOCKS5_HOSTNAME

LR_YUM_FULL         = None
LR_YUM_REPOMDONLY   = [None]
LR_YUM_BASEXML      = ["primary", "filelists", "other", None]
LR_YUM_BASEDB       = ["primary_db", "filelists_db", "other_db", None]
LR_YUM_HAWKEY       = ["primary", "filelists", "prestodelta", None]

YUM_FULL         = LR_YUM_FULL
YUM_REPOMDONLY   = LR_YUM_REPOMDONLY
YUM_BASEXML      = LR_YUM_BASEXML
YUM_BASEDB       = LR_YUM_BASEDB
YUM_HAWKEY       = LR_YUM_HAWKEY

LRE_OK                  = _librepo.LRE_OK
LRE_BADFUNCARG          = _librepo.LRE_BADFUNCARG
LRE_BADOPTARG           = _librepo.LRE_BADOPTARG
LRE_UNKNOWNOPT          = _librepo.LRE_UNKNOWNOPT
LRE_CURLSETOPT          = _librepo.LRE_CURLSETOPT
LRE_ALREADYUSEDRESULT   = _librepo.LRE_ALREADYUSEDRESULT
LRE_INCOMPLETERESULT    = _librepo.LRE_INCOMPLETERESULT
LRE_CURLDUP             = _librepo.LRE_CURLDUP
LRE_CURL                = _librepo.LRE_CURL
LRE_CURLM               = _librepo.LRE_CURLM
LRE_BADSTATUS           = _librepo.LRE_BADSTATUS
LRE_TEMPORARYERR        = _librepo.LRE_TEMPORARYERR
LRE_NOTLOCAL            = _librepo.LRE_NOTLOCAL
LRE_CANNOTCREATEDIR     = _librepo.LRE_CANNOTCREATEDIR
LRE_IO                  = _librepo.LRE_IO
LRE_MLBAD               = _librepo.LRE_MLBAD
LRE_MLXML               = _librepo.LRE_MLXML
LRE_BADCHECKSUM         = _librepo.LRE_BADCHECKSUM
LRE_REPOMDXML           = _librepo.LRE_REPOMDXML
LRE_NOURL               = _librepo.LRE_NOURL
LRE_CANNOTCREATETMP     = _librepo.LRE_CANNOTCREATETMP
LRE_UNKNOWNCHECKSUM     = _librepo.LRE_UNKNOWNCHECKSUM
LRE_BADURL              = _librepo.LRE_BADURL
LRE_GPGNOTSUPPORTED     = _librepo.LRE_GPGNOTSUPPORTED
LRE_GPGERROR            = _librepo.LRE_GPGERROR
LRE_BADGPG              = _librepo.LRE_BADGPG
LRE_INCOMPLETEREPO      = _librepo.LRE_INCOMPLETEREPO
LRE_INTERRUPTED         = _librepo.LRE_INTERRUPTED
LRE_SIGACTION           = _librepo.LRE_SIGACTION
LRE_ALREADYDOWNLOADED   = _librepo.LRE_ALREADYDOWNLOADED
LRE_UNFINISHED          = _librepo.LRE_UNFINISHED
LRE_SELECT              = _librepo.LRE_SELECT
LRE_OPENSSL             = _librepo.LRE_OPENSSL
LRE_MEMORY              = _librepo.LRE_MEMORY
LRE_XMLPARSER           = _librepo.LRE_XMLPARSER
LRE_CBINTERRUPTED       = _librepo.LRE_CBINTERRUPTED
LRE_UNKNOWNERROR        = _librepo.LRE_UNKNOWNERROR

LRR_YUM_REPO        = _librepo.LRR_YUM_REPO
LRR_YUM_REPOMD      = _librepo.LRR_YUM_REPOMD
LRR_YUM_TIMESTAMP   = _librepo.LRR_YUM_TIMESTAMP
LRR_SENTINEL        = _librepo.LRR_SENTINEL

ATTR_TO_LRR = {
    "yum_repo":         LRR_YUM_REPO,
    "yum_repomd":       LRR_YUM_REPOMD,
    "yum_timestamp":    LRR_YUM_TIMESTAMP,
}

CHECKSUM_UNKNOWN    = _librepo.CHECKSUM_UNKNOWN
CHECKSUM_MD5        = _librepo.CHECKSUM_MD5
CHECKSUM_SHA1       = _librepo.CHECKSUM_SHA1
CHECKSUM_SHA224     = _librepo.CHECKSUM_SHA224
CHECKSUM_SHA256     = _librepo.CHECKSUM_SHA256
CHECKSUM_SHA384     = _librepo.CHECKSUM_SHA384
CHECKSUM_SHA512     = _librepo.CHECKSUM_SHA512

MD5        = _librepo.CHECKSUM_MD5
SHA1       = _librepo.CHECKSUM_SHA1
SHA224     = _librepo.CHECKSUM_SHA224
SHA256     = _librepo.CHECKSUM_SHA256
SHA384     = _librepo.CHECKSUM_SHA384
SHA512     = _librepo.CHECKSUM_SHA512

_CHECKSUM_STR_TO_VAL_MAP = {
    'md5':      CHECKSUM_MD5,
    'sha':      CHECKSUM_SHA1,
    'sha1':     CHECKSUM_SHA1,
    'sha224':   CHECKSUM_SHA224,
    'sha256':   CHECKSUM_SHA256,
    'sha384':   CHECKSUM_SHA384,
    'sha512':   CHECKSUM_SHA512,
}

TRANSFER_SUCCESSFUL     = _librepo.TRANSFER_SUCCESSFUL
TRANSFER_ALREADYEXISTS  = _librepo.TRANSFER_ALREADYEXISTS
TRANSFER_ERROR          = _librepo.TRANSFER_ERROR

FMSTAGE_INIT                = _librepo.FMSTAGE_INIT
FMSTAGE_CACHELOADING        = _librepo.FMSTAGE_CACHELOADING
FMSTAGE_CACHELOADINGSTATUS  = _librepo.FMSTAGE_CACHELOADINGSTATUS
FMSTAGE_DETECTION           = _librepo.FMSTAGE_DETECTION
FMSTAGE_FINISHING           = _librepo.FMSTAGE_FINISHING
FMSTAGE_STATUS              = _librepo.FMSTAGE_STATUS

def checksum_str_to_type(name):
    name = name.lower()
    return _CHECKSUM_STR_TO_VAL_MAP.get(name, CHECKSUM_UNKNOWN)

class PackageTarget(_librepo.PackageTarget):
    """
    Represent a single package that will be downloaded by
    :meth:`~librepo.Handle.download_packages()`.
    """

    def __init__(self, relative_url, dest=None, checksum_type=CHECKSUM_UNKNOWN,
                 checksum=None, expectedsize=0, base_url=None, resume=False,
                 progresscb=None, cbdata=None, handle=None, endcb=None,
                 mirrorfailurecb=None):
        _librepo.PackageTarget.__init__(self, handle, relative_url, dest,
                                        checksum_type, checksum, expectedsize,
                                        base_url, resume, progresscb, cbdata,
                                        endcb, mirrorfailurecb)


class Handle(_librepo.Handle):
    """Librepo handle class.
    Handle hold information about a repository and configuration for
    downloading from the repository.

    **Attributes:**

    .. attribute:: update:

        See: :data:`.LRO_UPDATE`

    .. attribute:: urls:

        See: :data:`.LRO_URLS`

    .. attribute:: mirrorlist:

        See: :data:`.LRO_MIRRORLIST`

    .. attribute:: mirrorlisturl:

        See: :data:`.LRO_MIRRORLISTURL`

    .. attribute:: metalinkurl:

        See: :data:`.LRO_METALINKURL`

    .. attribute:: local:

        See: :data:`.LRO_LOCAL`

    .. attribute:: httpauth:

        See: :data:`.LRO_HTTPAUTH`

    .. attribute:: userpwd:

        See: :data:`.LRO_USERPWD`

    .. attribute:: proxy:

        See: :data:`.LRO_PROXY`

    .. attribute:: proxyport:

        See: :data:`.LRO_PROXYPORT`

    .. attribute:: proxytype:

        See: :data:`.LRO_PROXYTYPE`

    .. attribute:: proxyauth:

        See: :data:`.LRO_PROXYAUTH`

    .. attribute:: proxyuserpwd:

        See: :data:`.LRO_PROXYUSERPWD`

    .. attribute:: progresscb:

        See: :data:`.LRO_PROGRESSCB`

    .. attribute:: progressdata:

        See: :data:`.LRO_PROGRESSDATA`

    .. attribute:: maxspeed:

        See: :data:`.LRO_MAXSPEED`

    .. attribute:: destdir:

        See: :data:`.LRO_DESTDIR`

    .. attribute:: repotype:

        See: :data:`.LRO_REPOTYPE`

    .. attribute:: connecttimeout:

        See: :data:`.LRO_CONNECTTIMEOUT`

    .. attribute:: ignoremissing:

        See: :data:`.LRO_IGNOREMISSING`

    .. attribute:: interruptible:

        See: :data:`.LRO_INTERRUPTIBLE`

    .. attribute:: useragent:

        See: :data:`.LRO_USERAGENT`

    .. attribute:: fetchmirrors:

        See: :data:`.LRO_FETCHMIRRORS`

    .. attribute:: maxmirrortries:

        See: :data:`.LRO_MAXMIRRORTRIES`

    .. attribute:: maxparalleldownloads:

        See: :data:`.LRO_MAXPARALLELDOWNLOADS`

    .. attribute:: maxdownloadspermirror:

        See: :data:`.LRO_MAXDOWNLOADSPERMIRROR`

    .. attribute:: varsub:

        See: :data:`.LRO_VARSUB`

    .. attribute:: fastestmirror:

        See: :data:`.LRO_FASTESTMIRROR`

    .. attribute:: fastestmirrorcache:

        See: :data:`.LRO_FASTESTMIRRORCACHE`

    .. attribute:: fastestmirrormaxage:

        See: :data:`.LRO_FASTESTMIRRORMAXAGE`

    .. attribute:: fastestmirrorcb:

        See: :data:`.LRO_FASTESTMIRRORCB`

    .. attribute:: fastestmirrordata:

        See: :data:`.LRO_FASTESTMIRRORDATA`

    .. attribute:: lowspeedtime:

        See: :data:`.LRO_LOWSPEEDTIME`

    .. attribute:: lowspeedlimit:

        See: :data:`.LRO_LOWSPEEDLIMIT`

    .. attribute:: gpgcheck:

        See: :data:`.LRO_GPGCHECK`

    .. attribute:: checksum:

        See: :data:`.LRO_CHECKSUM`

    .. attribute:: yumdlist:

        See: :data:`.LRO_YUMDLIST`

    .. attribute:: yumblist:

        See: :data:`.LRO_YUMBLIST`
    """

    def setopt(self, option, val):
        """Set option to :class:`.Handle` directly.

        *option* could be one of: :ref:`handle-options-label`

        Example::

            # The command:
            h.setopt(librepo.LRO_URLS, ["http://ftp.linux.ncsu.edu/pub/fedora/linux/releases/17/Everything/i386/os/"])
            # is equivalent to:
            h.urls(["http://ftp.linux.ncsu.edu/pub/fedora/linux/releases/17/Everything/i386/os/"])
        """

        if (option == LRO_URLS and (not isinstance(val, list) and val is not None)):
            import warnings
            warnings.warn("Using string value for LRO_URLS is deprecated, " \
                          "use list of strings instead", DeprecationWarning)
            val = [val]

        _librepo.Handle.setopt(self, option, val)

    def __setattr__(self, attr, val):
        if attr not in ATTR_TO_LRO and attr in ATTR_TO_LRI:
            raise AttributeError("Set of attribute '%s' is not supported" % attr)
        elif attr not in ATTR_TO_LRO:
            raise AttributeError("'%s' object has no attribute '%s'" % \
                                 (self.__class__.__name__, attr))
        self.setopt(ATTR_TO_LRO[attr], val)

    def __getattr__(self, attr):
        if attr not in ATTR_TO_LRI and attr in ATTR_TO_LRO:
            raise AttributeError("Read of attribute '%s' is not supported" % attr)
        elif attr not in ATTR_TO_LRI:
            raise AttributeError("'%s' object has no attribute '%s'" % \
                                 (self.__class__.__name__, attr))
        return _librepo.Handle.getinfo(self, ATTR_TO_LRI[attr])

    def getinfo(self, option):
        """Get information from :class:`.Handle`.

        *option* could be one of :ref:`handle-info-options-label`
        """
        return _librepo.Handle.getinfo(self, option)

    def download(self, url, dest=None, checksum_type=CHECKSUM_UNKNOWN,
                 checksum=None, expectedsize=0, base_url=None, resume=0):
        """Download package from repository specified by
        :meth:`~librepo.Handle.url()` or :meth:`~librepo.Handle.mirrorlist()`
        method. If *base_url* is specified, url and mirrorlist in handle
        are ignored.

        Note: If resume is True and checksum_type and checksum are specified
        and downloaded package already exists, then checksum of the
        existing package is checked. If checksum matches, then no downloading
        is done and exception with LRE_ALREADYDOWNLOADED return code is raised.

        :param url: Relative path to the package in the repository.
        :param dest: Destination for package. Could be absolute/relative
                     path to directory or filename.
        :param checksum_type: Type of used checksum.
        :param checksum: Checksum value.
        :param expectedsize: Expected size of the file. If server reports
                             different size, then no download is preformed.
        :param base_url: Instead of repositories specified in ``Handle``
                         use repository on this url.
        :param resume: ``True`` enables resume. Resume means that if local
                       destination file exists, just try to
                       resume download, if not or resume download failed
                       than download whole file again.

        Example::

            h = librepo.Handle()
            h.setopt(librepo.LRO_URLS, ["http://ftp.linux.ncsu.edu/pub/fedora/linux/releases/17/Everything/i386/os/"])
            h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
            h.download("Packages/s/sl-3.03-12.fc17.i686.rpm",
                checksum="0ec8535d0fc00b497d8aef491c3f8b3955f2d84846325ee44851d9de8a36d12c",
                checksum_type=librepo.CHECKSUM_SHA256)

        .. note::
            If checksum check is disabled in the current ``Handle``, then
            checksum is NOT checked even if *checksum* and *checksum_type*
            params are specified!

        """
        if isinstance(checksum_type, str):
            checksum_type = checksum_str_to_type(checksum_type)
        self.download_package(url, dest, checksum_type, checksum,
                              expectedsize, base_url, resume)

    def perform(self, result=None):
        if result is None:
            result = Result()
        _librepo.Handle.perform(self, result)
        return result

    def new_packagetarget(self, relative_url, **kwargs):
        return PackageTarget(relative_url, handle=self, **kwargs)

class Result(_librepo.Result):
    """Librepo result class

    This class holds information about a downloaded/localised repository.

    **Attributes:**

    .. attribute:: yum_repo

        See: :data:`.LRR_YUM_REPO`

    .. attribute:: yum_repomd

        See: :data:`.LRR_YUM_REPOMD`

    .. attribute:: yum_timestamp

        See: :data:`.LRR_YUM_TIMESTAMP`
    """

    def getinfo(self, option):
        """Returns information about a downloaded/localised repository.

        *option* could be one of: :ref:`result-options-label`
        """
        return _librepo.Result.getinfo(self, option)

    def __getattr__(self, attr):
        if attr not in ATTR_TO_LRR:
            raise AttributeError("'%s' object has no attribute '%s'" % \
                                 (self.__class__.__name__, attr))
        return self.getinfo(ATTR_TO_LRR[attr])

# Functions

yum_repomd_get_age    = _librepo.yum_repomd_get_age
set_debug_log_handler = _librepo.set_debug_log_handler

def download_packages(list, failfast=False):
    """
    Download list of packages. *list* is a list of PackageTarget objects.
    If the *failfast* is True, then whole downloading is stoped
    immediately when any of download fails (and exception is raised).
    If the failfast is False, then this function returns after all
    downloads finish (no matter if successfully or unsuccessfully)
    and exception is raised only if a nonrecoverable
    error related to the function itself is meet
    (Errors related to individual downloads are
    reported via corresponding PackageTarget objects)
    """
    return _librepo.download_packages(list, failfast)

def download_url(url, fd, handle=None):
    return _librepo.download_url(handle, url, fd)
