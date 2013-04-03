"""

Module init and cleanup
=======================

.. function:: global_init()

    Initializes curl library wich is internaly used.

.. function:: global_cleanup()

    Reclaims memory that has been obtained through a libcurl call.

Exceptions
==========

Librepo module has only one own exception.

.. class:: LibrepoException

Value of this exception is tuple with three elements:
``(return code, error message, extra information)``

* Return code is a value from: :ref:`error-codes-label`.
* Error message is a string with a description of the error.
* Extra information is `None` for most of errors, but for specific
  errors it contain additional info about the error.

    Error codes with extra information and its format:
        * **LRE_CURL** - ``(Curl easy handle error code, Curl error string)``
        * **LRE_CURLM** - ``(Curl multi handle error code, Curl error string)``
        * **LRE_BADSTATUS** - ``Status code number`` (e.g.: 403, 404, 500, ...)

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

    LRO (aka LibRepo Option) constants are used to set :class:`.Handle`
    options via :meth:`~.Handle.setopt` method.

    This options could be also set by :class:`.Handle` attributes
    with same names in lowercase and without ``LRO_`` prefix.

    Example::

            # The command:
            h.setopt(librepo.LRO_URL, "http://ftp.linux.ncsu.edu/pub/fedora/linux/releases/17/Everything/i386/os/")
            # is equivalent to:
            h.url = "http://ftp.linux.ncsu.edu/pub/fedora/linux/releases/17/Everything/i386/os/"

    .. note:: For detailed description of this options consult :class:`.Handle` page.

.. data:: LRO_UPDATE

    *Boolean*. Set to ``True`` if only want update localised or downloaded
    repository represented by :class:`.Result` object. Update mode is
    meant to download previously omitted repository file(s).

.. data:: LRO_URL

    *String or None*. Set repository url (repository url and baseurl
    are interchangeable terms in this context).

.. data:: LRO_MIRRORLIST

    *String or None*. Set mirrorlist url (url could point to a metalink
    mirrorlist or to a simple mirrorlist where each line wihtout ``'#'``
    is considered as mirror url).

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

    *Function*. Set progress callback. Callback must be in format:
    ``callback(userdata, total_to_download, downloaded)``. If
    total_to_download is 0, then total size is not known.
    Total size (total_to_download) could change (grow) among two callback
    calls (if some download failed and another mirror is tried).

.. data:: LRO_PROGRESSDATA

    *Any object*. Set user data for the progress callback.

.. data:: LRO_RETRIES

    *Integer or None*. Set maximal number of retries for one mirror.
    One try per mirror is default value. None as *va* sets the
    default value.

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
.. data:: LRI_URL
.. data:: LRI_MIRRORLIST
.. data:: LRI_LOCAL
.. data:: LRI_PROGRESSCB
.. data:: LRI_PROGRESSDATA
.. data:: LRI_DESTDIR
.. data:: LRI_REPOTYPE
.. data:: LRI_USERAGENT
.. data:: LRI_YUMDLIST
.. data:: LRI_YUMBLIST
.. data:: LRI_LASTCURLERR
.. data:: LRI_LASTCURLMERR
.. data:: LRI_LASTCURLSTRERR
.. data:: LRI_LASTCURLMSTRERR
.. data:: LRI_LASTBADSTATUSCODE
.. data:: LRI_MIRRORS

.. _proxy-type-label:

Proxy type constants
--------------------

.. data:: LR_PROXY_HTTP
.. data:: LR_PROXY_HTTP_1_0
.. data:: LR_PROXY_SOCKS4
.. data:: LR_PROXY_SOCKS5
.. data:: LR_PROXY_SOCKS4A
.. data:: LR_PROXY_SOCKS5_HOSTNAME

.. _repotype-constants-label:

Repo type constants
-------------------

.. data:: LR_YUMREPO

    Classical yum repository with ``repodata/`` directory.

.. data:: LR_SUSEREPO

    YaST2 repository (http://en.opensuse.org/openSUSE:Standards_YaST2_Repository_Metadata_content).

    .. note:: This option is not supported yet!

.. data:: LR_DEBREPO

    Debian repository

    .. note:: This option is not supported yet!

.. _predefined-yumdlists-label:

Predefined yumdlist lists
-------------------------

.. data:: LR_YUM_FULL

    Download all repodata files

.. data:: LR_REPOMDONLY

    Download only repomd.xml file

.. data:: LR_BASEXML

    Download only primary.xml, filelists.xml and other.xml

.. data:: LR_BASEDB

    Download only primary, filelists and other databases.

.. data:: LR_BASEHAWKEY

    Download only files used by Hawkey (https://github.com/akozumpl/hawkey/).
    (primary, filelists, prestodelta)

.. _error-codes-label:

Error codes
-----------

LibRepo Error codes.

.. data:: LRE_OK

    Everything is ok.

.. data:: LRE_BADFUNCARG

    Bad function argument(s).

.. data:: LRE_BADOPTARG

    Bad argument for the option in :meth:`~.Handle.setopt`.

.. data:: LRE_UNKNOWNOPT

    Unknown option.

.. data:: LRE_CURLSETOPT

    cURL doesn't know an option used by librepo. Probably
    too old cURL version is used.

.. data:: LRE_ALREDYUSEDRESULT

    :class:`.Result` object is not "clean" (it has been already
    used and filled).

.. data:: LRE_INCOMPLETERESULT

    :class:`.Result` object doesn't contain all what is needed.

.. data:: LRE_CURLDUP

    Cannot duplicate cURL handle. No free memory?

.. data:: LRE_CURL

    A cURL error.

.. data:: LRE_CURLM

    A multi cURL handle error.

.. data:: LRE_BADSTATUS

    An error HTTP or FTP status code.

.. data:: LRE_TEMPORARYERR

    An error that should be temporary (e.g. HTTP status codes 500, 502-504, ..)

.. data:: LRE_NOTLOCAL

    If :data:`~.LRO_UPDATE` option is ``True`` and URL is not a local path (address).

.. data:: LRE_CANNOTCREATEDIR

    Cannot create directory for downloaded data. Bad permission?

.. data:: LRE_IO

    Input/Output error. Bad permission?

.. data:: LRE_MLBAD

    Bad mirrorlist or metalink file. E.g. metalink doesn't contain
    reference to target file (repomd.xml), mirrorlist is empty, ..

.. data:: LRE_MLXML

    Cannot parse metalink xml. Non-valid metalink file?

.. data:: LRE_BADCHECKSUM

    A downloaded file has bad checksum.

.. data:: LRE_REPOMDXML

    Cannot parse repomd.xml file. Non-valid repomd.xml file?

.. data:: LRE_NOURL

    No usable URL found. E.g. bad links or no links in metalink.

.. data:: LRE_CANNOTCREATETMP

    Cannot create temporary directory.

.. data:: LRE_UNKNOWNCHECKSUM

    Unknown type of checksum is needed for verification one or more files.

.. data:: LRE_BADURL

    Bad URL specified.

.. data:: LRE_GPGNOTSUPPORTED

    OpenPGP protocol is not supported.

.. data:: LRE_GPGERROR

    GPG error.

.. data:: LRE_BADGPG

    Bad GPG signature.

.. data:: LRE_INCOMPLETEREPO

    Repository metadata are not complete.

.. data:: LRE_INTERRUPTED

    Download was interrupted by SIGTERM signal.

.. data:: LRE_SIGACTION

    Cannot set own signal handler. Sigaction system call failed.

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

"""

import _librepo

VERSION_MAJOR = _librepo.VERSION_MAJOR
VERSION_MINOR = _librepo.VERSION_MINOR
VERSION_PATCH = _librepo.VERSION_PATCH
VERSION = u"%d.%d.%d" % (VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH)

LibrepoException = _librepo.LibrepoException

global_init    = _librepo.global_init
global_cleanup = _librepo.global_cleanup

LRO_UPDATE          = _librepo.LRO_UPDATE
LRO_URL             = _librepo.LRO_URL
LRO_MIRRORLIST      = _librepo.LRO_MIRRORLIST
LRO_LOCAL           = _librepo.LRO_LOCAL
LRO_HTTPAUTH        = _librepo.LRO_HTTPAUTH
LRO_USERPWD         = _librepo.LRO_USERPWD
LRO_PROXY           = _librepo.LRO_PROXY
LRO_PROXYPORT       = _librepo.LRO_PROXYPORT
LRO_PROXYTYPE       = _librepo.LRO_PROXYTYPE
LRO_PROXYAUTH       = _librepo.LRO_PROXYAUTH
LRO_PROXYUSERPWD    = _librepo.LRO_PROXYUSERPWD
LRO_PROGRESSCB      = _librepo.LRO_PROGRESSCB
LRO_PROGRESSDATA    = _librepo.LRO_PROGRESSDATA
LRO_RETRIES         = _librepo.LRO_RETRIES
LRO_MAXSPEED        = _librepo.LRO_MAXSPEED
LRO_DESTDIR         = _librepo.LRO_DESTDIR
LRO_REPOTYPE        = _librepo.LRO_REPOTYPE
LRO_CONNECTTIMEOUT  = _librepo.LRO_CONNECTTIMEOUT
LRO_IGNOREMISSING   = _librepo.LRO_IGNOREMISSING
LRO_INTERRUPTIBLE   = _librepo.LRO_INTERRUPTIBLE
LRO_USERAGENT       = _librepo.LRO_USERAGENT
LRO_GPGCHECK        = _librepo.LRO_GPGCHECK
LRO_CHECKSUM        = _librepo.LRO_CHECKSUM
LRO_YUMDLIST        = _librepo.LRO_YUMDLIST
LRO_YUMBLIST        = _librepo.LRO_YUMBLIST
LRO_SENTINEL        = _librepo.LRO_SENTINEL

ATTR_TO_LRO = {
    "update":           LRO_UPDATE,
    "url":              LRO_URL,
    "mirrorlist":       LRO_MIRRORLIST,
    "local" :           LRO_LOCAL,
    "httpauth":         LRO_HTTPAUTH,
    "userpwd":          LRO_USERPWD,
    "proxy":            LRO_PROXY,
    "proxyport":        LRO_PROXYPORT,
    "proxytype":        LRO_PROXYTYPE,
    "proxyauth":        LRO_PROXYAUTH,
    "proxyuserpwd":     LRO_PROXYUSERPWD,
    "progresscb":       LRO_PROGRESSCB,
    "progressdata":     LRO_PROGRESSDATA,
    "retries":          LRO_RETRIES,
    "maxspeed":         LRO_MAXSPEED,
    "destdir":          LRO_DESTDIR,
    "repotype":         LRO_REPOTYPE,
    "connecttimeout":   LRO_CONNECTTIMEOUT,
    "ignoremissing":    LRO_IGNOREMISSING,
    "interruptible":    LRO_INTERRUPTIBLE,
    "useragent":        LRO_USERAGENT,
    "gpgcheck":         LRO_GPGCHECK,
    "checksum":         LRO_CHECKSUM,
    "yumdlist":         LRO_YUMDLIST,
    "yumblist":         LRO_YUMBLIST,
}

LRI_UPDATE              = _librepo.LRI_UPDATE
LRI_URL                 = _librepo.LRI_URL
LRI_MIRRORLIST          = _librepo.LRI_MIRRORLIST
LRI_LOCAL               = _librepo.LRI_LOCAL
LRI_PROGRESSCB          = _librepo.LRI_PROGRESSCB
LRI_PROGRESSDATA        = _librepo.LRI_PROGRESSDATA
LRI_DESTDIR             = _librepo.LRI_DESTDIR
LRI_REPOTYPE            = _librepo.LRI_REPOTYPE
LRI_USERAGENT           = _librepo.LRI_USERAGENT
LRI_YUMDLIST            = _librepo.LRI_YUMDLIST
LRI_YUMBLIST            = _librepo.LRI_YUMBLIST
LRI_LASTCURLERR         = _librepo.LRI_LASTCURLERR
LRI_LASTCURLMERR        = _librepo.LRI_LASTCURLMERR
LRI_LASTCURLSTRERR      = _librepo.LRI_LASTCURLSTRERR
LRI_LASTCURLMSTRERR     = _librepo.LRI_LASTCURLMSTRERR
LRI_LASTBADSTATUSCODE   = _librepo.LRI_LASTBADSTATUSCODE
LRI_MIRRORS             = _librepo.LRI_MIRRORS

ATTR_TO_LRI = {
    "update":               LRI_UPDATE,
    "url":                  LRI_URL,
    "mirrorlist":           LRI_MIRRORLIST,
    "local":                LRI_LOCAL,
    "progresscb":           LRI_PROGRESSCB,
    "progressdata":         LRI_PROGRESSDATA,
    "destdir":              LRI_DESTDIR,
    "repotype":             LRI_REPOTYPE,
    "useragent":            LRI_USERAGENT,
    "yumdlist":             LRI_YUMDLIST,
    "yumblist":             LRI_YUMBLIST,
    "lastcurlerr":          LRI_LASTCURLERR,
    "lastcurlmerr":         LRI_LASTCURLMERR,
    "lastcurlstrerr":       LRI_LASTCURLSTRERR,
    "lastcurlmstrerr":      LRI_LASTCURLMSTRERR,
    "lastbadstatuscode":    LRI_LASTBADSTATUSCODE,
    "mirrors":              LRI_MIRRORS,
}

LR_CHECK_GPG        = _librepo.LR_CHECK_GPG
LR_CHECK_CHECKSUM   = _librepo.LR_CHECK_CHECKSUM

LR_YUMREPO  = _librepo.LR_YUMREPO
LR_SUSEREPO = _librepo.LR_SUSEREPO
LR_DEBREPO  = _librepo.LR_DEBREPO

LR_PROXY_HTTP               = _librepo.LR_PROXY_HTTP
LR_PROXY_HTTP_1_0           = _librepo.LR_PROXY_HTTP_1_0
LR_PROXY_SOCKS4             = _librepo.LR_PROXY_SOCKS4
LR_PROXY_SOCKS5             = _librepo.LR_PROXY_SOCKS5
LR_PROXY_SOCKS4A            = _librepo.LR_PROXY_SOCKS4A
LR_PROXY_SOCKS5_HOSTNAME    = _librepo.LR_PROXY_SOCKS5_HOSTNAME

LR_YUM_FULL         = None
LR_YUM_REPOMDONLY   = [None]
LR_YUM_BASEXML      = ["primary", "filelists", "other", None]
LR_YUM_BASEDB       = ["primary_db", "filelists_db", "other_db", None]
LR_YUM_BASEHAWKEY   = ["primary", "filelists", "prestodelta", None]

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
LRE_UNKNOWNERROR        = _librepo.LRE_UNKNOWNERROR

LRR_YUM_REPO    = _librepo.LRR_YUM_REPO
LRR_YUM_REPOMD  = _librepo.LRR_YUM_REPOMD
LRR_SENTINEL    = _librepo.LRR_SENTINEL

ATTR_TO_LRR = {
    "yum_repo":     LRR_YUM_REPO,
    "yum_repomd":   LRR_YUM_REPOMD,
}

CHECKSUM_UNKNOWN    = _librepo.CHECKSUM_UNKNOWN
CHECKSUM_MD2        = _librepo.CHECKSUM_MD2
CHECKSUM_MD5        = _librepo.CHECKSUM_MD5
CHECKSUM_SHA        = _librepo.CHECKSUM_SHA
CHECKSUM_SHA1       = _librepo.CHECKSUM_SHA1
CHECKSUM_SHA224     = _librepo.CHECKSUM_SHA224
CHECKSUM_SHA256     = _librepo.CHECKSUM_SHA256
CHECKSUM_SHA384     = _librepo.CHECKSUM_SHA384
CHECKSUM_SHA512     = _librepo.CHECKSUM_SHA512

_CHECKSUM_STR_TO_VAL_MAP = {
    'md2':      CHECKSUM_MD2,
    'md5':      CHECKSUM_MD5,
    'sha':      CHECKSUM_SHA,
    'sha1':     CHECKSUM_SHA1,
    'sha224':   CHECKSUM_SHA224,
    'sha256':   CHECKSUM_SHA256,
    'sha384':   CHECKSUM_SHA384,
    'sha512':   CHECKSUM_SHA512,
}

def checksum_str_to_type(name):
    name = name.lower()
    return _CHECKSUM_STR_TO_VAL_MAP.get(name, CHECKSUM_UNKNOWN)

class Handle(_librepo.Handle):
    """Librepo handle class.
    Handle hold information about a repository and configuration for
    downloading from the repository.

    **Attributes:**

    .. attribute:: update:

        See: :data:`.LRO_UPDATE`

    .. attribute:: url:

        See: :data:`.LRO_URL`

    .. attribute:: mirrorlist:

        See: :data:`.LRO_MIRRORLIST`

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

    .. attribute:: retries:

        See: :data:`.LRO_RETRIES`

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
            h.setopt(librepo.LRO_URL, "http://ftp.linux.ncsu.edu/pub/fedora/linux/releases/17/Everything/i386/os/")
            # is equivalent to:
            h.url("http://ftp.linux.ncsu.edu/pub/fedora/linux/releases/17/Everything/i386/os/")
        """
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
                 checksum=None, base_url=None, resume=0):
        """Download package from repository specified by
        :meth:`~librepo.Handle.url()` or :meth:`~librepo.Handle.mirrorlist()`
        method. If *base_url* is specified, url and mirrorlist in handle
        are ignored.

        :param url: Relative path to the package in the repository.
        :param dest: Destination for package. Could be absolute/relative
                     path to directory or filename.
        :param checksum_type: Type of used checksum.
        :param checksum: Checksum value.
        :param base_url: Instead of repositories specified in ``Handle``
                         use repository on this url.
        :param resume: ``True`` enables resume. Resume means that if local
                       destination file exists, just try to
                       resume download, if not or resume download failed
                       than download whole file again.

        Example::

            h = librepo.Handle()
            h.setopt(librepo.LRO_URL, "http://ftp.linux.ncsu.edu/pub/fedora/linux/releases/17/Everything/i386/os/")
            h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
            h.download("Packages/s/sl-3.03-12.fc17.i686.rpm",
                checksum="0ec8535d0fc00b497d8aef491c3f8b3955f2d84846325ee44851d9de8a36d12c",
                checksum_type=librepo.CHECKSUM_SHA256)

        .. note::
            If checksum check is disabled in the current ``Handle``, then
            checksum is NOT checked even if *checksum* and *checksum_type*
            params are specified!

        """
        if isinstance(checksum_type, basestring):
            checksum_type = checksum_str_to_type(checksum_type)
        self.download_package(url, dest, checksum_type, checksum, base_url, resume)


class Result(_librepo.Result):
    """Librepo result class

    This class holds information about a downloaded/localised repository.

    **Attributes:**

    .. attribute:: yum_repo

        See: :data:`.LRR_YUM_REPO`

    .. attribute:: yum_repomd

        See: :data:`.LRR_YUM_REPOMD`
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
