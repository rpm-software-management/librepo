import _librepo

VERSION_MAJOR = _librepo.VERSION_MAJOR
VERSION_MINOR = _librepo.VERSION_MINOR
VERSION_PATCH = _librepo.VERSION_PATCH
VERSION = u"%d.%d.%d" % (VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH)

Handle = _librepo.Handle
Result = _librepo.Result

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
LRO_YUMREPOFLAGS    = _librepo.LRO_YUMREPOFLAGS
LRO_SENTINEL        = _librepo.LRO_SENTINEL

LR_CHECK_GPG        = _librepo.LR_CHECK_GPG
LR_CHECK_CHECKSUM   = _librepo.LR_CHECK_CHECKSUM

LR_YUMREPO  = _librepo.LR_YUMREPO
LR_SUSEREPO = _librepo.LR_SUSEREPO
LR_DEBREPO  = _librepo.LR_DEBREPO

LR_YUM_REPOMDONLY   = _librepo.LR_YUM_REPOMDONLY
LR_YUM_PRI          = _librepo.LR_YUM_PRI
LR_YUM_FIL          = _librepo.LR_YUM_FIL
LR_YUM_OTH          = _librepo.LR_YUM_OTH
LR_YUM_PRIDB        = _librepo.LR_YUM_PRIDB
LR_YUM_FILDB        = _librepo.LR_YUM_FILDB
LR_YUM_OTHDB        = _librepo.LR_YUM_OTHDB
LR_YUM_GROUP        = _librepo.LR_YUM_GROUP
LR_YUM_GROUPGZ      = _librepo.LR_YUM_GROUPGZ
LR_YUM_PRESTODELTA  = _librepo.LR_YUM_PRESTODELTA
LR_YUM_DELTAINFO    = _librepo.LR_YUM_DELTAINFO
LR_YUM_UPDATEINFO   = _librepo.LR_YUM_UPDATEINFO
LR_YUM_ORIGIN       = _librepo.LR_YUM_ORIGIN
LR_YUM_BASEXML      = _librepo.LR_YUM_BASEXML
LR_YUM_BASEDB       = _librepo.LR_YUM_BASEDB
LR_YUM_BASEHAWKEY   = _librepo.LR_YUM_BASEHAWKEY
LR_YUM_FULL         = _librepo.LR_YUM_FULL

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

class Handle(_librepo.Handle):
    def update(self, val):
        self.setopt(LRO_UPDATE, val)
    def url(self, val):
        self.setopt(LRO_URL, val)
    def mirrorlist(self, val):
        self.setopt(LRO_MIRRORLIST, val)
    def local(self, val):
        self.setopt(LRO_LOCAL, val)
    def httpauth(self, val):
        self.setopt(LRO_HTTPAUTH, val)
    def userpwd(self, val):
        self.setopt(LRO_USERPWD, val)
    def proxy(self, val):
        self.setopt(LRO_PROXY, val)
    def proxyport(self, val):
        self.setopt(LRO_PROXYPORT, val)
    def proxysock(self, val):
        self.setopt(LRO_PROXYSOCK, val)
    def proxyauth(self, val):
        self.setopt(LRO_PROXYAUTH, val)
    def proxyuserpwd(self, val):
        self.setopt(LRO_PROXYUSERPWD, val)
    def progresscb(self, val):
        self.setopt(LRO_PROGRESSCB, val)
    def progressdata(self, val):
        self.setopt(LRO_PROGRESSDATA, val)
    def retries(self, val):
        self.setopt(LRO_RETRIES, val)
    def maxspeed(self, val):
        self.setopt(LRO_MAXSPEED, val)
    def destdir(self, val):
        self.setopt(LRO_DESTDIR, val)
    def repotype(self, val):
        self.setopt(LRO_REPOTYPE, val)
    def gpgcheck(self, val):
        self.setopt(LRO_GPGCHECK, val)
    def checksum(self, val):
        self.setopt(LRO_CHECKSUM, val)
    def yumrepoflags(self, val):
        self.setopt(LRO_YUMREPOFLAGS, val)
    def download(self, url, dest=None, checksum_type=0, checksum=None, base_url=None, resume=0):
        self.download_package(url, dest, checksum_type, checksum, base_url, resume)

