# librepo - A library providing (libcURL like) API to downloading repository
# Copyright (C) 2012-2014 Tomas Mlcoch
#
# Licensed under the GNU Lesser General Public License Version 2.1
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
#

"""
Version contants
----------------

.. data:: VERSION_MAJOR
          VERSION_MINOR
          VERSION_PATCH
          VERSION

    Constants with version numbers and whole version string.

.. _handle-options-label:

:class:`~.Handle` options
-------------------------

    **LRO_** (aka LibRepo Option) prefixed constants are used to set
    :class:`.Handle` options via :meth:`~.Handle.setopt` method.

    This options could be also set by :class:`.Handle` attributes
    with the same names but in lowercase and without ``LRO_`` prefix.

    Example::

            # The command:
            h.setopt(librepo.LRO_URLS, ["http://ftp.linux.ncsu.edu/pub/fedora/linux/releases/17/Everything/i386/os/"])
            # is equivalent to:
            h.urls = ["http://ftp.linux.ncsu.edu/pub/fedora/linux/releases/17/Everything/i386/os/"]

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

    *Long*. Set type of proxy - could be one of :ref:`proxy-type-label`

.. data:: LRO_PROXYAUTH

    *Boolean*. If True, all supported proxy authentication methods are enabled.
    If False, only basic authentication method is enabled.

.. data:: LRO_PROXYUSERPWD

    *String or None*. Set username and password for proxy
    authentication in format 'username:password'.

.. data:: LRO_PROGRESSCB

    *Function or None*. (See: :ref:`callback-progresscb-label`)
    Set progress callback. Callback must be in format:
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
    Default value is 30. None as *val* sets the default value.

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

    *Function or None*. (See: :ref:`callback-fastestmirrorcb-label`)
    Fastest mirror status callback.
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
    Default: 30 (sec)

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

.. data:: LRO_YUMSLIST

    *[(String, String), ...] or None*. Set list of substitutions
    for repomd records.
    ``[("group_gz", "group")]`

.. data:: LRO_RPMMDDLIST

    See LRO_YUMDLIST

.. data:: LRO_YUMBLIST

    *List of strings*. Set blacklist of yum metadata files.
    This files will not be downloaded.

.. data:: LRO_RPMMDBLIST

    See LRO_YUMBLIST

.. data:: LRO_HMFCB

    *Function or None* (See: :ref:`callback-handlemirrorfailurecb-label`)
    The Handle Mirror Failure Callback is called
    when a metadata download fails. It provides
    a detailed information about what exactly failed.
    Call of this callback doesn't mean that whole downloading failed.
    If there are other mirrors on the list, these mirrors will be tried.

.. data:: LRO_SSLVERIFYPEER

    *Boolean*. This option determines whether librepo verifies the
    authenticity of the peer's certificate. This trust is based on a chain
    of digital signatures, rooted in certification authority
    (CA) certificates.

.. data:: LRO_SSLVERIFYHOST

    *Boolean*. This option determines whether librepo verifies that
    the server cert is for the server it is known as.

.. data:: LRO_SSLCLIENTCERT

    *String or None*. Path to the PEM format SSL client certificate to use
    when talking to the server.

.. data:: LRO_SSLCLIENTKEY

    *String or None*. Path to the PEM format SSL client key to use when
    talking to the server.

.. data:: LRO_SSLCACERT

    *String or None*. Path to a file containing the list of PEM format
    trusted CA certificates.

.. data:: LRO_IPRESOLVE

    *Integer or None* Sets kind of IP addresses to use when resolving host
    names. Could be one of: :ref:`ipresolve-type-label`

.. data:: LRO_ALLOWEDMIRRORFAILURES

    *Integer or None* If all transfers from a mirror failed (no successful transfer
    from the mirror exists) and the number
    of failed downloads is higher or equal to this value
    the mirror will be skipped (ignored) for all next downloads.

    **Note:** Number of failed transfers for a single mirror can
    outreach this number! For example, if you set this value to 1
    but you allow 3 parallel downloads it is possible that all
    three downloads start from the mirror,
    before any of them can fail. Then, if all three transfers
    fail, the number of failures for the mirror
    will be 3, even if this option was set to 1.

.. data:: LRO_ADAPTIVEMIRRORSORTING

    *Integer or None* If enabled, internal list of mirrors for each
    handle is re-sorted after each finished transfer.
    The the sorting is based on mirror error rate etc.

.. data:: LRO_GNUPGHOMEDIR

    *String or None* set own GNUPG configuration directory (a dir with keyring).

.. data:: LRO_FASTESTMIRRORTIMEOUT

    *Float of None* Max length of fastest mirror measurement in seconds.
    Default value is 2.0sec.

.. data:: LRO_HTTPHEADER

    *List of strings or None* List of strings that represent http headers.
    Each header has format "header_name: content". If you add a header
    with no content as in 'Accept:' (no data on the right side of the colon),
    the internally used header will get disabled. With this option you can
    add new headers, replace internal headers and remove internal headers.
    To add a header with no content (nothing to the right side of the colon),
    use the form 'MyHeader;' (note the ending semicolon).
    Note: Headers must not be CRLF-terminated!

.. data:: LRO_OFFLINE

    *Boolean* Make the handle work only locally, all remote URLs are
    ignored. Remote mirrorlists/metalinks (if they are specified)
    are ignored. Fastest mirror check (if enabled) is skiped.

.. data:: LRO_HTTPAUTHMETHODS

    *Long (bitmask)* Bitmask which tell Librepo which auth metods to use.
    See: :ref:`auth-methods-label`

.. data:: LRO_PROXYAUTHMETHODS

    *Long (bitmask)* Bitmask which tell Librepo which auth metods to use
    for proxy authentication.
    See: :ref:`auth-methods-label`

.. data:: LRO_FTPUSEEPSV

    *Boolean* Enable/Disable EPSV (Extended Passive mode) for FTP.

.. data:: LRO_PRESERVETIME

    *Boolean* If enabled, librepo will try to keep timestamps of the downloaded files
    in sync with that on the remote side.

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
.. data:: LRI_YUMSLIST
.. data:: LRI_RPMMDDLIST
.. data:: LRI_YUMBLIST
.. data:: LRI_RPMMDBLIST
.. data:: LRI_FETCHMIRRORS
.. data:: LRI_MAXMIRRORTRIES
.. data:: LRI_VARSUB
.. data:: LRI_MIRRORS
.. data:: LRI_METALINK
.. data:: LRI_FASTESTMIRROR
.. data:: LRI_FASTESTMIRRORCACHE
.. data:: LRI_FASTESTMIRRORMAXAGE
.. data:: LRI_HMFCB
.. data:: LRI_SSLVERIFYPEER
.. data:: LRI_SSLVERIFYHOST
.. data:: LRI_SSLCLIENTCERT
.. data:: LRI_SSLCLIENTKEY
.. data:: LRI_SSLCACERT
.. data:: LRI_IPRESOLVE
.. data:: LRI_ALLOWEDMIRRORFAILURES
.. data:: LRI_ADAPTIVEMIRRORSORTING
.. data:: LRI_GNUPGHOMEDIR
.. data:: LRI_FASTESTMIRRORTIMEOUT
.. data:: LRI_HTTPHEADER
.. data:: LRI_OFFLINE
.. data:: LRI_HTTPAUTHMETHODS
.. data:: LRI_PROXYAUTHMETHODS
.. data:: LRI_FTPUSEEPSV

.. _proxy-type-label:

Proxy type constants
--------------------

.. data:: PROXY_HTTP (LR_PROXY_HTTP)
.. data:: PROXY_HTTP_1_0 (LR_PROXY_HTTP_1_0)
.. data:: PROXY_SOCKS4 (LR_PROXY_SOCKS4)
.. data:: PROXY_SOCKS5 (LR_PROXY_SOCKS5)
.. data:: PROXY_SOCKS4A (LR_PROXY_SOCKS4A)
.. data:: PROXY_SOCKS5_HOSTNAME (LR_PROXY_SOCKS5_HOSTNAME)

.. _ipresolve-type-label:

Supported IP resolving
----------------------

.. data:: IPRESOLVE_WHATEVER

    Default value, resolves addresses to all IP versions that
    your system allows.

.. data:: IPRESOLVE_V4

    Resolve to IPv4 addresses.

.. data:: IPRESOLVE_V6

    Resolve to IPv6 addresses.

.. _repotype-constants-label:

Repo type constants
-------------------

.. data:: RPMMDREPO (LR_RPMMDREPO)

    Classical repository in repo-md format with ``repodata/`` directory.

.. data:: YUMREPO (LR_YUMREPO)

    See RPMMDREPO

.. data:: SUSEREPO (LR_SUSEREPO)

    YaST2 repository (http://en.opensuse.org/openSUSE:Standards_YaST2_Repository_Metadata_content).

    .. note:: This option is not supported yet!

.. data:: DEBREPO (LR_DEBREPO)

    Debian repository

    .. note:: This option is not supported yet!

.. _predefined-yumdlists-label:

Predefined yumdlist lists
-------------------------

.. data:: RPMMD_FULL (YUM_FULL, LR_YUM_FULL)

    Download all repodata files

.. data:: RPMMD_REPOMDONLY (YUM_REPOMDONLY, LR_YUM_REPOMDONLY)

    Download only repomd.xml file

.. data:: RPMMD_BASEXML (YUM_BASEXML, LR_YUM_BASEXML)

    Download only primary.xml, filelists.xml and other.xml

.. data:: RPMMD_BASEDB (YUM_BASEDB, LR_YUM_BASEDB)

    Download only primary, filelists and other databases.

.. data:: RPMMD_HAWKEY (YUM_HAWKEY, LR_YUM_HAWKEY)

    Download only files used by Hawkey (https://github.com/akozumpl/hawkey/).
    (primary, filelists, prestodelta)


.. _auth-methods-label:

Auth methods
------------

Supported auth methods for :data:`~.LRO_HTTPAUTHMETHODS` and
:data:`~.LRO_PROXYAUTHMETHODS` options.

.. data:: LR_AUTH_NONE

    No auth method enabled.

.. data:: LR_AUTH_BASIC

    HTTP Basic authentication (Default).

.. data:: LR_AUTH_DIGEST

    HTTP Digest authentication.

.. data:: LR_AUTH_NEGOTIATE

    HTTP Negotiate (SPNEGO) authentication.

.. data:: LR_AUTH_NTLM

    HTTP NTLM authentication.

.. data:: LR_AUTH_DIGEST_IE

    HTTP Digest authentication with an IE flavor.

.. data:: LR_AUTH_NTLM_WB

    NTLM delegating to winbind helper.

.. data:: LR_AUTH_ONLY

    This is a meta symbol. OR this value
    together with a single specific auth
    value to force libcurl to probe for
    un-restricted auth and if not, only
    that single auth algorithm is
    acceptable.

.. data:: LR_AUTH_ANY

    All suitable methods.


.. _fastestmirror-stages-constants-label:

Fastest mirror stages
---------------------

Values used by fastest mirror callback (:data:`~.LRO_FASTESTMIRRORCB`):

.. data:: FMSTAGE_INIT

    (0) Fastest mirror detection just started. *data* is None.

.. data:: FMSTAGE_CACHELOADING

    (1) Cache file is specified. *data* is path to the cache file.

.. data:: FMSTAGE_CACHELOADINGSTATUS

    (2) Cache loading finished. If successful, *data* is None, otherwise
        *data* is string with error message.

.. data:: FMSTAGE_DETECTION

    (3) Detection in progress. *data* is number of mirrors that will be pinged.
    If all times were loaded from cache, this stage is skiped.

.. data:: FMSTAGE_FINISHING

    (4) Detection is done, sorting mirrors, updating cache, etc.
        *data* is None.

.. data:: FMSTAGE_STATUS

    (5) The very last invocation of fastest mirror callback.
        If fastest mirror detection was successful *data*,
        otherwise *data* contain string with error message.

.. _error-codes-label:

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

.. data:: LRR_YUM_REPO (deprecated - use LRR_RPMMD_REPO instead)

    Returns a flat dictionary with local paths to downloaded/localised
    rpmmd repository and basic repository's information.::

        {'repomd': u'/tmp/librepotest-jPMmX5/repodata/repomd.xml',
         'url': u'http://127.0.0.1:5000/yum/static/01/'
         'destdir': u'/tmp/librepotest-jPMmX5',
         'metalink': None,
         'mirrorlist': None,
         'signature': None,
         'primary': u'/tmp/librepotest-jPMmX5/repodata/4543ad62e4d86337cd1949346f9aec976b847b58-primary.xml.gz',
         'primary_db': u'/tmp/librepotest-jPMmX5/repodata/735cd6294df08bdf28e2ba113915ca05a151118e-primary.sqlite.bz2',
         'filelists': u'/tmp/librepotest-jPMmX5/repodata/aeca08fccd3c1ab831e1df1a62711a44ba1922c9-filelists.xml.gz',
         'filelists_db': u'/tmp/librepotest-jPMmX5/repodata/4034dcea76c94d3f7a9616779539a4ea8cac288f-filelists.sqlite.bz2',
         'other': u'/tmp/librepotest-jPMmX5/repodata/a8977cdaa0b14321d9acfab81ce8a85e869eee32-other.xml.gz',
         'other_db': u'/tmp/librepotest-jPMmX5/repodata/fd96942c919628895187778633001cff61e872b8-other.sqlite.bz2',
        }

.. data:: LRR_RPMMD_REPO

    Returns a dict with local paths to downloaded/localised
    rpmmd repository and basic repository's information.::

        {'repomd': u'/tmp/librepotest-jPMmX5/repodata/repomd.xml',
         'url': u'http://127.0.0.1:5000/yum/static/01/'
         'destdir': u'/tmp/librepotest-jPMmX5',
         'metalink': None,
         'mirrorlist': None,
         'signature': None,
         'paths': {'primary': u'/tmp/librepotest-jPMmX5/repodata/4543ad62e4d86337cd1949346f9aec976b847b58-primary.xml.gz',
                   'primary_db': u'/tmp/librepotest-jPMmX5/repodata/735cd6294df08bdf28e2ba113915ca05a151118e-primary.sqlite.bz2',
                   'filelists': u'/tmp/librepotest-jPMmX5/repodata/aeca08fccd3c1ab831e1df1a62711a44ba1922c9-filelists.xml.gz',
                   'filelists_db': u'/tmp/librepotest-jPMmX5/repodata/4034dcea76c94d3f7a9616779539a4ea8cac288f-filelists.sqlite.bz2',
                   'other': u'/tmp/librepotest-jPMmX5/repodata/a8977cdaa0b14321d9acfab81ce8a85e869eee32-other.xml.gz',
                   'other_db': u'/tmp/librepotest-jPMmX5/repodata/fd96942c919628895187778633001cff61e872b8-other.sqlite.bz2'}
        }

.. data:: LRR_YUM_REPOMD (deprecated - use LRR_RPMMD_REPOMD instead)

    Returns a flat dict representing a repomd.xml file of downloaded
    rpmmd repository.::

        {'revision': u'1347459931',
         'repo_tags': [],
         'content_tags': [],
         'distro_tags': [],
         'primary': {'checksum': u'4543ad62e4d86337cd1949346f9aec976b847b58',
                     'checksum_open': u'68457ceb8e20bda004d46e0a4dfa4a69ce71db48',
                     'checksum_open_type': u'sha1',
                     'checksum_type': u'sha1',
                     'db_version': 0L,
                     'location_href': u'repodata/4543ad62e4d86337cd1949346f9aec976b847b58-primary.xml.gz',
                     'size': 936L,
                     'size_open': 3385L,
                     'timestamp': 1347459930L},
         'primary_db': {'checksum': u'735cd6294df08bdf28e2ba113915ca05a151118e',
                        'checksum_open': u'ba636386312e1b597fc4feb182d04c059b2a77d5',
                        'checksum_open_type': u'sha1',
                        'checksum_type': u'sha1',
                        'db_version': 10L,
                        'location_href': u'repodata/735cd6294df08bdf28e2ba113915ca05a151118e-primary.sqlite.bz2',
                        'size': 2603L,
                        'size_open': 23552L,
                        'timestamp': 1347459931L},
         'filelists': {'checksum': u'aeca08fccd3c1ab831e1df1a62711a44ba1922c9',
                       'checksum_open': u'52d30ae3162ca863c63c345ffdb7f0e10c1414a5',
                       'checksum_open_type': u'sha1',
                       'checksum_type': u'sha1',
                       'db_version': 0L,
                       'location_href': u'repodata/aeca08fccd3c1ab831e1df1a62711a44ba1922c9-filelists.xml.gz',
                       'size': 43310L,
                       'size_open': 735088L,
                       'timestamp': 1347459930L},
         'filelists_db': {'checksum': u'4034dcea76c94d3f7a9616779539a4ea8cac288f',
                          'checksum_open': u'949c6b7b605b2bc66852630c841a5003603ca5b2',
                          'checksum_open_type': u'sha1',
                          'checksum_type': u'sha1',
                          'db_version': 10L,
                          'location_href': u'repodata/4034dcea76c94d3f7a9616779539a4ea8cac288f-filelists.sqlite.bz2',
                          'size': 22575L,
                          'size_open': 201728L,
                          'timestamp': 1347459931L},
         'other': {'checksum': u'a8977cdaa0b14321d9acfab81ce8a85e869eee32',
                   'checksum_open': u'4b5b8874fb233a626b03b3260a1aa08dce90e81a',
                   'checksum_open_type': u'sha1',
                   'checksum_type': u'sha1',
                   'db_version': 0L,
                   'location_href': u'repodata/a8977cdaa0b14321d9acfab81ce8a85e869eee32-other.xml.gz',
                   'size': 807L,
                   'size_open': 1910L,
                   'timestamp': 1347459930L},
         'other_db': {'checksum': u'fd96942c919628895187778633001cff61e872b8',
                      'checksum_open': u'c5262f62b6b3360722b9b2fb5d0a9335d0a51112',
                      'checksum_open_type': u'sha1',
                      'checksum_type': u'sha1',
                      'db_version': 10L,
                      'location_href': u'repodata/fd96942c919628895187778633001cff61e872b8-other.sqlite.bz2',
                      'size': 1407L,
                      'size_open': 8192L,
                      'timestamp': 1347459931L},
        }

.. data:: LRR_RPMMD_REPOMD

    Returns a dict representing a repomd.xml file of downloaded
    rpmmd repository.::

        {'revision': u'1347459931',
         'repo_tags': [],
         'content_tags': [],
         'distro_tags': [],
         'records': {'primary': {'checksum': u'4543ad62e4d86337cd1949346f9aec976b847b58',
                                 'checksum_open': u'68457ceb8e20bda004d46e0a4dfa4a69ce71db48',
                                 'checksum_open_type': u'sha1',
                                 'checksum_type': u'sha1',
                                 'db_version': 0L,
                                 'location_href': u'repodata/4543ad62e4d86337cd1949346f9aec976b847b58-primary.xml.gz',
                                 'size': 936L,
                                 'size_open': 3385L,
                                 'timestamp': 1347459930L},
                     'primary_db': {'checksum': u'735cd6294df08bdf28e2ba113915ca05a151118e',
                                    'checksum_open': u'ba636386312e1b597fc4feb182d04c059b2a77d5',
                                    'checksum_open_type': u'sha1',
                                    'checksum_type': u'sha1',
                                    'db_version': 10L,
                                    'location_href': u'repodata/735cd6294df08bdf28e2ba113915ca05a151118e-primary.sqlite.bz2',
                                    'size': 2603L,
                                    'size_open': 23552L,
                                    'timestamp': 1347459931L}},
                     'filelists': {'checksum': u'aeca08fccd3c1ab831e1df1a62711a44ba1922c9',
                                   'checksum_open': u'52d30ae3162ca863c63c345ffdb7f0e10c1414a5',
                                   'checksum_open_type': u'sha1',
                                   'checksum_type': u'sha1',
                                   'db_version': 0L,
                                   'location_href': u'repodata/aeca08fccd3c1ab831e1df1a62711a44ba1922c9-filelists.xml.gz',
                                   'size': 43310L,
                                   'size_open': 735088L,
                                   'timestamp': 1347459930L},
                     'filelists_db': {'checksum': u'4034dcea76c94d3f7a9616779539a4ea8cac288f',
                                      'checksum_open': u'949c6b7b605b2bc66852630c841a5003603ca5b2',
                                      'checksum_open_type': u'sha1',
                                      'checksum_type': u'sha1',
                                      'db_version': 10L,
                                      'location_href': u'repodata/4034dcea76c94d3f7a9616779539a4ea8cac288f-filelists.sqlite.bz2',
                                      'size': 22575L,
                                      'size_open': 201728L,
                                      'timestamp': 1347459931L},
                     'other': {'checksum': u'a8977cdaa0b14321d9acfab81ce8a85e869eee32',
                               'checksum_open': u'4b5b8874fb233a626b03b3260a1aa08dce90e81a',
                               'checksum_open_type': u'sha1',
                               'checksum_type': u'sha1',
                               'db_version': 0L,
                               'location_href': u'repodata/a8977cdaa0b14321d9acfab81ce8a85e869eee32-other.xml.gz',
                               'size': 807L,
                               'size_open': 1910L,
                               'timestamp': 1347459930L},
                     'other_db': {'checksum': u'fd96942c919628895187778633001cff61e872b8',
                                  'checksum_open': u'c5262f62b6b3360722b9b2fb5d0a9335d0a51112',
                                  'checksum_open_type': u'sha1',
                                  'checksum_type': u'sha1',
                                  'db_version': 10L,
                                  'location_href': u'repodata/fd96942c919628895187778633001cff61e872b8-other.sqlite.bz2',
                                  'size': 1407L,
                                  'size_open': 8192L,
                                  'timestamp': 1347459931L},
        }

.. data:: LRR_RPMMD_TIMESTAMP (LRR_YUM_TIMESTAMP)

    Returns the highest timestamp from all records in the repomd.
    See: http://yum.baseurl.org/gitweb?p=yum.git;a=commitdiff;h=59d3d67f

.. _endcb-statuses-label:

Transfer statuses for endcb of :class:`~.PackageTarget`
-------------------------------------------------------

.. data:: TRANSFER_SUCCESSFUL
.. data:: TRANSFER_ALREADYEXISTS
.. data:: TRANSFER_ERROR

Callbacks prototypes
--------------------

Some librepo's functions and classes can take a callbacks in their arguments.
This section contain the callbacks prototypes and explanation of their
arguments.

By default all callbacks should return *None* (this is default behaviour
of python function if it doesn't have a specified return statement -
it returns None).

But there are some callbacks that can return a specific values defined
in :ref:`callbacks-return-values`.

.. _callback-progresscb-label:

Progress callback - progresscb
------------------------------

``progresscb(userdata, totalsize, downloaded)``

Callback frequently called during a download.

:userdata: User specified data or *None*
:totalsize: Total size (in bytes) of the target (float).
:downloaded: Currently downloaded size (in bytes).
:returns: This callback can return values from :ref:`callbacks-return-values`

.. _callback-endcb-label:

End callback - endcb
--------------------

``endcb(userdata, status, msg)``

Callback called when a transfer is done either successfully or unsuccessfully.

:userdata: User specified data or *None*
:status: :ref:`endcb-statuses-label`
:msg: String with error message or *None*
:returns: This callback can return values from :ref:`callbacks-return-values`

.. _callback-mirrorfailurecb-label:

Mirror Failure Callback - mirrorfailurecb
-----------------------------------------

``mirrorfailurecb(userdata, msg, url)``

Callback called when a transfer failed.

:userdata: User specified data or *None*
:msg: String with error message
:url: String with mirror URL
:returns: This callback can return values from :ref:`callbacks-return-values`

.. _callback-fastestmirrorcb-label:

Fastestmirror callback - fastestmirrorcb
----------------------------------------

``fastestmirrorcb(userdata, stage, data)``

:userdata: User specified data or *None*
:stage: :ref:`fastestmirror-stages-constants-label`
:data: Content of *data* is different for different stages.
       See :ref:`fastestmirror-stages-constants-label`
:returns: This callback must return *None* each other value will be ignored.


.. _callback-handlemirrorfailurecb-label:

Handle Mirror Failure Callback - hmfcb
--------------------------------------

``hmfcb(userdata, msg, url, metadata)``

Callback called when a download of a metadata during
:meth:`~.Handle.perform()` fails.

:userdata: User specified data or *None*
:msg: String with error message
:url: String with mirror URL
:metadata: String with metadata name ("primary", "filelists", ...)
:returns: This callback can return values from :ref:`callbacks-return-values`

.. _callbacks-return-values:

Callbacks return values
-----------------------

Each callback can safely return ``None`` (this is implicit return value
when no return statement is defined).

``return None`` is equivalent for ``return librepo.CB_OK``.

.. data:: CB_OK

    Everything is OK

.. data:: CB_ABORT

    Abort just the current transfer.
    Note: If failfast option for the downloading is enabled,
    the whole downloading will be aborted.

.. data:: CB_ERROR

    Abort the whole downloading (all transfers).
    (Note: This code is automatically internally returned when
    an exception is raised in the callback.)

.. _checksum-constants-label:

Checksum (hash) type constants
------------------------------

.. data:: MD5 (CHECKSUM_MD5)
.. data:: SHA1 (CHECKSUM_SHA1)
.. data:: SHA224 (CHECKSUM_SHA224)
.. data:: SHA256 (CHECKSUM_SHA256)
.. data:: SHA384 (CHECKSUM_SHA384)
.. data:: SHA512 (CHECKSUM_SHA512)

"""

import sys
import librepo._librepo
current_module = sys.modules[__name__]

LibrepoException = _librepo.LibrepoException

ATTR_TO_LRO = {}
ATTR_TO_LRI = {}
ATTR_TO_LRR = {}
_CHECKSUM_STR_TO_VAL_MAP = {}

# Create local aliases for contants from _librepo C module
for attr in dir(_librepo):
    if not attr.isupper():
        # Only constants should be imported automatically
        continue

    # Create local alias
    val = getattr(_librepo, attr)
    setattr(current_module, attr, val)

    if attr.endswith("_SENTINEL"):
        # Ignore sentinel values
        continue

    if attr.startswith("LR_"):
        setattr(current_module, attr[3:], val)
    if attr.startswith("LRO_"):
        ATTR_TO_LRO[attr.lower()[4:]] = val
    if attr.startswith("LRI_"):
        ATTR_TO_LRI[attr.lower()[4:]] = val
    if attr.startswith("LRR_"):
        ATTR_TO_LRR[attr.lower()[4:]] = val
    if attr.startswith("LR_CHECKSUM_"):
        setattr(current_module, attr[12:], val)
        _CHECKSUM_STR_TO_VAL_MAP[attr[12:].lower()] = val


RPMMD_FULL      = YUM_FULL        = LR_YUM_FULL         = None
RPMMD_REPOMDONLY= YUM_REPOMDONLY  = LR_YUM_REPOMDONLY   = [None]
RPMMD_BASEXML   = YUM_BASEXML     = LR_YUM_BASEXML      = ["primary", "filelists", "other", None]
RPMMD_BASEDB    = YUM_BASEDB      = LR_YUM_BASEDB       = ["primary_db", "filelists_db", "other_db", None]
RPMMD_HAWKEY    = YUM_HAWKEY      = LR_YUM_HAWKEY       = ["primary", "filelists", "prestodelta", None]


def checksum_str_to_type(name):
    """
    Convert string with name of hash function to numeric value
    that represents the hash in createrepo_c.

    :param name: Checksum name (e.g. "sha256", "sha512", ...)
    :returns: Integer value (from :ref:`checksum-constants-label`)
    """
    name = name.lower()
    return _CHECKSUM_STR_TO_VAL_MAP.get(name, CHECKSUM_UNKNOWN)

class MetadataTarget(_librepo.MetadataTarget):

    def __init__(self, handle=None, cbdata=None, progresscb=None, mirrorfailurecb=None, endcb=None, gnupghomedir=None):
        _librepo.MetadataTarget.__init__(self, handle, cbdata, progresscb, mirrorfailurecb, endcb, gnupghomedir)

class PackageTarget(_librepo.PackageTarget):
    """
    Represent a single package that will be downloaded by
    :func:`~librepo.download_packages`.
    """

    def __init__(self, relative_url, dest=None, checksum_type=CHECKSUM_UNKNOWN,
                 checksum=None, expectedsize=0, base_url=None, resume=False,
                 progresscb=None, cbdata=None, handle=None, endcb=None,
                 mirrorfailurecb=None, byterangestart=0, byterangeend=0):
        """
        :param relative_url: Target URL. If *handle* or *base_url* specified,
            the *url* can be (and logically should be) only a relative part of path.
        :param dest: Destination filename or directory (file basename will
            be derived from the relative_url). If *None* current
            working directory will be used.
        :param checksum_type: :ref:`checksum-constants-label`
        :param checksum: Expected checksum value.
        :param expectedsize: Expected size of the target. If server reports
            different size, then download won't be performed.
        :param base_url: Base part of URL
        :param resume: If True then downloader will try to resume download
            if the destination file already exists. If the file doesn't exist
            yet, it will be downloaded.
        :param progresscb: :ref:`callback-progresscb-label`
        :param cbdata: User data for the callback.
        :param handle: :class:`~librepo.Handle`
        :param endcb: :ref:`callback-endcb-label`
        :param mirrorfailurecb: See :ref:`callback-mirrorfailurecb-label`
        :param byterangestart: Start downloading from the specified byte.
        :param byterangeend: Stop downloading at the specified byte.
            *Note: If the byterangeend is less or equal to byterangestart,
            then it is ignored!*
        """
        _librepo.PackageTarget.__init__(self, handle, relative_url, dest,
                                        checksum_type, checksum, expectedsize,
                                        base_url, resume, progresscb, cbdata,
                                        endcb, mirrorfailurecb, byterangestart,
                                        byterangeend)


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

    .. attribute:: rpmmddlist:

        See: :data:`.LRO_RPMMDDLIST`

    .. attribute:: yumdlist:

        See: :data:`.LRO_YUMDLIST`

    .. attribute:: yumslist:

        See: :data:`.LRO_YUMSLIST`

    .. attribute:: rpmmdblist:

        See: :data:`.LRO_RPMMDBLIST`

    .. attribute:: yumblist:

        See: :data:`.LRO_YUMBLIST`

    .. attribute:: hmfcb:

        See: :data:`.LRO_HMFCB`

    .. attribute:: sslverifypeer:

        See :data:`.LRO_SSLVERIFYPEER`

    .. attribute:: sslverifyhost:

        See :data:`.LRO_SSLVERIFYHOST`

    .. attribute:: sslclientcert:

        See :data:`.LRO_SSLCLIENTCERT`

    .. attribute:: sslclientkey:

        See :data:`.LRO_SSLCLIENTKEY`

    .. attribute:: sslcacert:

        See :data:`.LRO_SSLCACERT`

    .. attribute:: ipresolve:

        See :data:`.LRO_IPRESOLVE`

    .. attribute:: allowedmirrorfailures:

        See :data:`.LRO_ALLOWEDMIRRORFAILURES`

    .. attribute:: adaptivemirrorsorting:

        See :data:`.LRO_ADAPTIVEMIRRORSORTING`

    .. attribute:: gnupghomedir:

        See :data:`.LRO_GNUPGHOMEDIR`

    .. attribute:: fastestmirrortimeout:

        See :data:`.LRO_FASTESTMIRRORTIMEOUT`

    .. attribute:: httpheader:

        See :data:`.LRO_HTTPHEADER`

    .. attribute:: offline:

        See :data:`.LRO_OFFLINE`

    .. attribute:: httpauthmethods

        See :data:`.LRO_HTTPAUTHMETHODS`

    .. attribute:: proxyauthmethods

        See :data:`.LRO_PROXYAUTHMETHODS`

    .. attribute:: ftpuseepsv

        See :data:`.LRO_FTPUSEEPSV`

    .. attribute:: preservetime

        See :data:`.LRO_PRESERVETIME`

    """

    def setopt(self, option, val):
        """Set option to :class:`.Handle` directly.

        :param option: One of: :ref:`handle-options-label`
        :returns: *None*

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

        :param option: One of :ref:`handle-info-options-label`
        :returns: Value for the specified option or
                  *None* if the option is not set.
        """
        return _librepo.Handle.getinfo(self, option)

    def download(self, url, dest=None, checksum_type=CHECKSUM_UNKNOWN,
                 checksum=None, expectedsize=0, base_url=None, resume=0):
        """
        **This method is deprecated** - Use :func:`~librepo.download_packages`
        instead.

        Download package from the repository specified in the
        :Class:`~librepo.Handle`. If *base_url* is specified,
        urls and mirrors specified in the Handle are ignored.

        Note: If resume is True and checksum_type and checksum are specified
        and downloaded package already exists, then checksum of the
        existing package is checked. If checksum matches, then no downloading
        is done and exception with LRE_ALREADYDOWNLOADED return code is raised.

        :param url: Relative path to the package in the repository.
        :param dest: Destination for package. Could be absolute/relative
                     path to directory or filename.
        :param checksum_type: :ref:`checksum-constants-label`.
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
        """
        Perform the specified action - download/locate a repository.

        :param result: :Class:`~librepo.Result` object or *None*
        :returns: :Class:`~librepo.Result` object that was passed by
                  *result* parameter or the new one if the parameter was not
                  specified or was *None*.
        """
        if result is None:
            result = Result()
        _librepo.Handle.perform(self, result)
        return result

    def new_packagetarget(self, relative_url, **kwargs):
        """
        Shortcut for creating a new :Class:`~librepo.PackageTarget` objects.
        Targets created by this way have automatically setted handle to the
        current :Class:`~librepo.Handle` object.

        :param relative_url: Relative par of target (package) URL
        :returns: New :Class:`~librepo.PackageTarget`
        """
        return PackageTarget(relative_url, handle=self, **kwargs)

class Result(_librepo.Result):
    """Librepo result class

    This class holds information about a downloaded/localised repository.

    **Attributes:**

    .. attribute:: rpmmd_repo

        See: :data:`.LRR_RPMMD_REPO`

    .. attribute:: yum_repo

        See: :data:`.LRR_YUM_REPO`

    .. attribute:: rpmmd_repomd

        See: :data:`.LRR_RPMMD_REPOMD`

    .. attribute:: yum_repomd

        See: :data:`.LRR_YUM_REPOMD`

    .. attribute:: rpmmd_timestamp

        See: :data:`.LRR_RPMMD_TIMESTAMP`

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

def download_metadata(list):
    """
    Download metadata. *list* is a list of
    :class:`~librepo.MetadataTarget` objects.
    Exception is raised only if a nonrecoverable
    error related to the function itself is meet
    (Errors related to individual downloads are
    reported via corresponding MetadataTarget objects)

    :param list: List of :class:`~.librepo.MetadataTarget` objects.
    :returns: *None*
    """
    return _librepo.download_metadata(list)

def download_packages(list, failfast=False):
    """
    Download list of packages. *list* is a list of
    :class:`~librepo.PackageTarget` objects.
    If the *failfast* is True, then whole downloading is stopped
    immediately when any of download fails (and exception is raised).
    If the failfast is False, then this function returns after all
    downloads finish (no matter if successfully or unsuccessfully)
    and exception is raised only if a nonrecoverable
    error related to the function itself is meet
    (Errors related to individual downloads are
    reported via corresponding PackageTarget objects)

    :param list: List of :class:`~.librepo.PackageTarget` objects.
    :param failfast: If *True*, stop whole downloading immediately when any
                     of downloads fails. If *False*, ignore failed download(s)
                     and continue with other downloads.
    :returns: *None*
    """
    return _librepo.download_packages(list, failfast)

def download_url(url, fd, handle=None):
    """
    Download specified URL and write it content to opened file descriptor.

    :param url: Target URL
    :param fd: Opened file descriptor (To get a file descriptor
               use for example **os.open()**.
    :param handle: :Class:`~librepo.Handle` object or *None*
    :returns: *None*
    """
    return _librepo.download_url(handle, url, fd)

def yum_repomd_get_age(result_object):
    """
    Get the highest timestamp of the repo's repomd.xml.

    :param result_object: Used (filled) :Class:`~librepo.Result`
    :returns: The highest timestamp from repomd (Float) or *0.0* on error.
    """
    return _librepo.yum_repomd_get_age(result_object)

def set_debug_log_handler(log_function, user_data=None):
    """
    ONLY FOR DEVELOPMENT (DEBUGGING) PURPOSES!

    (Deprecated, use :func:`~log_set_file` instead)

    When python debug log handler is used, the librepo is **THREAD-UNSAFE**!

    If used, it overrides logging set by :func:`~log_set_file` and vice versa.

    :param log_function: Function that will handle the debug messages.
    :param user_data: An data you want to be passed to the log_function
                      during call.
    :returns: *None*

    Example::

        def debug_function(msg, _):
            print msg
        librepo.set_debug_log_handler(debug_function)

    """
    return _librepo.set_debug_log_handler(log_function, user_data)

def log_set_file(filename):
    """Set a filename of a file where logs are going to be written.

    Note: Only one file could be set at a time.

    Note: If the LIBREPO_DEBUG environ variable is set and this
    function is used then the LIBREPO_DEBUG effect will be suppressed.
    (All debug output will be redirected to the specified file)

    :param filename: Filename
    :returns: Id of the handler
    """
    return _librepo.log_set_file(filename)

def log_remove_handler(handler_id):
    """Remove handler.

    :param handler_id: id
    """
    return _librepo.log_remove_handler(handler_id)
