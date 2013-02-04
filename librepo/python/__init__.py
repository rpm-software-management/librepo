"""

Module init and cleanup
=======================

.. function:: global_init()

    Initializes curl library wich is internaly used.

.. function:: global_cleanup()

    Reclaims memory that has been obtained through a libcurl call.

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

    This options could be also set by :class:`.Handle` class methods
    with same names in lowercase and without ``LRO_`` prefix.

    Example::

            # The command:
            h.setopt(librepo.LRO_URL, "http://ftp.linux.ncsu.edu/pub/fedora/linux/releases/17/Everything/i386/os/")
            # is equivalent to:
            h.url("http://ftp.linux.ncsu.edu/pub/fedora/linux/releases/17/Everything/i386/os/")

    .. note:: For detailed description of this options consult :class:`.Handle` page.

.. data:: LRO_UPDATE

    Boolean. See more: :meth:`~.Handle.update`

.. data:: LRO_URL

    String or None. See more: :meth:`~.Handle.url`

.. data:: LRO_MIRRORLIST

    String or None. See more: :meth:`~.Handle.mirrorlist`

.. data:: LRO_LOCAL

    Boolean. See more: :meth:`~.Handle.local`

.. data:: LRO_HTTPAUTH

    Boolean. See more: :meth:`~.Handle.httpauth`

.. data:: LRO_USERPWD

    String or None. See more: :meth:`~.Handle.userpwd`

.. data:: LRO_PROXY

    String or None. See more: :meth:`~.Handle.proxy`

.. data:: LRO_PROXYPORT

    Integer or None. See more: :meth:`~.Handle.proxyport`

.. data:: LRO_PROXYSOCK

    Boolean. See more: :meth:`~.Handle.proxysock`

.. data:: LRO_PROXYAUTH

    Boolean. See more: :meth:`~.Handle.proxyauth`

.. data:: LRO_PROXYUSERPWD

    String or None. See more: :meth:`~.Handle.proxyuserpwd`

.. data:: LRO_PROGRESSCB

    Function. See more: :meth:`~.Handle.progresscb`

.. data:: LRO_PROGRESSDATA

    Any object. See more: :meth:`~.Handle.progressdata`

.. data:: LRO_RETRIES

    Integer or None. See more: :meth:`~.Handle.retries`

.. data:: LRO_MAXSPEED

    Long or None. See more: :meth:`~.Handle.maxspeed`

.. data:: LRO_DESTDIR

    String or None. See more: :meth:`~.Handle.destdir`

.. data:: LRO_REPOTYPE

    One of :ref:`repotype-constants-label`. See more :meth:`~.Handle.repotype`

.. data:: LRO_CONNECTTIMEOUT

    Integer or None. See more: :meth:`~.Handle.connecttimeout`

.. data:: LRO_GPGCHECK

    Boolean. See more: :meth:`~.Handle.gpgcheck`

.. data:: LRO_CHECKSUM

    Boolean. See more: :meth:`~.Handle.checksum`

.. data:: LRO_YUMDLIST

    List of strings. See more: :meth:`~.Handle.yumdlist`
    Some predefined list :ref:`predefined-yumdlists-label`.


.. _handle-info-options-label:

:class:`~.Handle` info options
------------------------------

    LRI (aka LibRepo Information) constants are used to get information
    from :class:`.Handle` via :meth:`~.Handle.getinfo` method.

.. data:: LRI_UPDATE
.. data:: LRI_URL
.. data:: LRI_MIRRORLIST
.. data:: LRI_LOCAL
.. data:: LRI_DESTDIR
.. data:: LRI_REPOTYPE
.. data:: LRI_YUMDLIST

.. _repotype-constants-label:

Repotype constants
------------------

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
LRO_PROXYSOCK       = _librepo.LRO_PROXYSOCK
LRO_PROXYAUTH       = _librepo.LRO_PROXYAUTH
LRO_PROXYUSERPWD    = _librepo.LRO_PROXYUSERPWD
LRO_PROGRESSCB      = _librepo.LRO_PROGRESSCB
LRO_PROGRESSDATA    = _librepo.LRO_PROGRESSDATA
LRO_RETRIES         = _librepo.LRO_RETRIES
LRO_MAXSPEED        = _librepo.LRO_MAXSPEED
LRO_DESTDIR         = _librepo.LRO_DESTDIR
LRO_REPOTYPE        = _librepo.LRO_REPOTYPE
LRO_CONNECTTIMEOUT  = _librepo.LRO_CONNECTTIMEOUT
LRO_GPGCHECK        = _librepo.LRO_GPGCHECK
LRO_CHECKSUM        = _librepo.LRO_CHECKSUM
LRO_YUMDLIST        = _librepo.LRO_YUMDLIST
LRO_SENTINEL        = _librepo.LRO_SENTINEL

LRI_UPDATE          = _librepo.LRI_UPDATE
LRI_URL             = _librepo.LRI_URL
LRI_MIRRORLIST      = _librepo.LRI_MIRRORLIST
LRI_LOCAL           = _librepo.LRI_LOCAL
LRI_DESTDIR         = _librepo.LRI_DESTDIR
LRI_REPOTYPE        = _librepo.LRI_REPOTYPE
LRI_YUMDLIST        = _librepo.LRI_YUMDLIST

LR_CHECK_GPG        = _librepo.LR_CHECK_GPG
LR_CHECK_CHECKSUM   = _librepo.LR_CHECK_CHECKSUM

LR_YUMREPO  = _librepo.LR_YUMREPO
LR_SUSEREPO = _librepo.LR_SUSEREPO
LR_DEBREPO  = _librepo.LR_DEBREPO

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
LRE_UNKNOWNERROR        = _librepo.LRE_UNKNOWNERROR

LRR_YUM_REPO    = _librepo.LRR_YUM_REPO
LRR_YUM_REPOMD  = _librepo.LRR_YUM_REPOMD
LRR_SENTINEL    = _librepo.LRR_SENTINEL

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
    downloading from the repository."""

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

    def getinfo(self, option):
        """Get information from :class:`.Handle`.

        *option* could be one of :ref:`handle-info-options-label`
        """
        return _librepo.Handle.getinfo(self, option)

    def update(self, val):
        """Set *val* to ``True`` if only want update localised or downloaded
        repository represented by :class:`.Result` object. Update mode is
        meant to download previously omitted repository file(s)."""
        self.setopt(LRO_UPDATE, val)

    def url(self, url):
        """Set repository *url* (repository url and baseurl
        are the same things)."""
        self.setopt(LRO_URL, url)

    def mirrorlist(self, url):
        """Set mirrorlist *url* (url could point to a metalink mirrorlist or
        to simple mirrorlist where each line wihtout ``'#'`` is considered
        as mirror url)."""
        self.setopt(LRO_MIRRORLIST, url)

    def local(self, val):
        """If set to True, no local copy of repository is created
        and repository is just localised in its current location.
        When True, url of repository MUST be a local address
        (e.g. '/home/user/repo' or 'file:///home/user/repo')."""
        self.setopt(LRO_LOCAL, val)

    def httpauth(self, val):
        """If True, all supported methods of HTTP authentication
        are enabled."""
        self.setopt(LRO_HTTPAUTH, val)

    def userpwd(self, val):
        """Set username and password for HTTP authentication. *val* must
        be in format 'username:password'."""
        self.setopt(LRO_USERPWD, val)

    def proxy(self, address):
        """Set proxy server address. Port could be specified by different
        option or by the :[port] suffix of this address.
        Any protocol prefix (http://) will be ignored."""
        self.setopt(LRO_PROXY, address)

    def proxyport(self, val):
        """Set proxy port number to connect unsless it is specified in
        the proxy address string. None sets default value 1080."""
        self.setopt(LRO_PROXYPORT, val)

    def proxysock(self, val):
        """If True proxy type is set to SOCK4."""
        self.setopt(LRO_PROXYSOCK, val)

    def proxyauth(self, val):
        """If True, all supported proxy authentication methods are enabled.
        If False, only basic authentication method is enabled."""
        self.setopt(LRO_PROXYAUTH, val)

    def proxyuserpwd(self, val):
        """Set username and password for proxy authentication in format
        'username:password'."""
        self.setopt(LRO_PROXYUSERPWD, val)

    def progresscb(self, val):
        """Set progress callback. Callback must be in format:
        ``callback(userdata, total_to_download, downloaded)``. If
        total_to_download is 0, then total size is not known.
        Total size (total_to_download) could change (grow) among two calls,
        if download failed and another mirror is tried."""
        self.setopt(LRO_PROGRESSCB, val)

    def progressdata(self, val):
        """Set user data for the progress callback."""
        self.setopt(LRO_PROGRESSDATA, val)

    def retries(self, val):
        """Set maximal number of retries for one mirror.
        One try per mirror is default value. None as *va* sets the
        default value."""
        self.setopt(LRO_RETRIES, val)

    def maxspeed(self, val):
        """Set maximal allowed speed per download in bytes per second.
        0 = unlimited speed - the default value."""
        self.setopt(LRO_MAXSPEED, val)

    def destdir(self, val):
        """Set destination directory for downloaded data
        (metadata or packages)."""
        self.setopt(LRO_DESTDIR, val)

    def repotype(self, val):
        """Set type of repository."""
        self.setopt(LRO_REPOTYPE, val)

    def connecttimeout(self, val):
        """Set maximal timeout in sec for connection phase.
        Default value is 300. None as *val* sets the default value."""
        self.setopt(LRO_CONNECTTIMEOUT, val)

    def gpgcheck(self, val):
        """Set True to enable gpg check (if available) of downloaded repo."""
        self.setopt(LRO_GPGCHECK, val)

    def checksum(self, val):
        """Set False/True to disable/enable checksum check.

        .. note:: Checksum check is enabled by default.
        .. note::
            If checksum check is disabled, then even explicitly specified
            checksum related params e.g. in :meth:`~librepo.Handle.download`
            method are ignored and checksum is not checked!"""
        self.setopt(LRO_CHECKSUM, val)

    def yumdlist(self, val):
        """Set list of yum metadata files to download. e.g. ``["primary",
        "filelists", "other", "primary_db", "prestodelta"]`` If *val* is None,
        all metadata files will be downloaded. If *val* is ``[]`` or ``[None]``
        only ``repomd.xml`` will be downloaded.
        """
        self.setopt(LRO_YUMDLIST, val)

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
    """

    def getinfo(self, option):
        """Returns information about a downloaded/localised repository.

        *option* could be one of: :ref:`result-options-label`
        """
        return _librepo.Result.getinfo(self, option)

    def yum_repo(self):
        """Return a dict with local paths to downloaded/localised
        yum repository."""
        return self.getinfo(LRR_YUM_REPO)

    def yum_repomd(self):
        """Return a dict representing a repomd.xml file of
        downloaded yum repository."""
        return self.getinfo(LRR_YUM_REPOMD)
