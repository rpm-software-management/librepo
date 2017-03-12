import unittest
import librepo

def foo_cb(data, total_to_download, downloaded):
    pass

def foo_hmfcb(data, msg, url, metadata):
    pass

class TestCaseHandle(unittest.TestCase):

    def test_handle_exceptions(self):
        h = librepo.Handle()
        self.assertTrue(h)

        self.assertRaises(AttributeError, getattr, h, 'foobar_attr')
        self.assertRaises(AttributeError, setattr, h, 'foobar_attr', 'xyz')

        self.assertRaises(ValueError, h.getinfo, 999999)
        self.assertRaises(ValueError, h.setopt, 999999, 'xyz')

    def test_handle_setopt_getinfo(self):
        """No exception should be raised."""
        h = librepo.Handle()

        self.assertFalse(h.getinfo(librepo.LRI_UPDATE))
        h.setopt(librepo.LRO_UPDATE,  True)
        self.assertTrue(h.getinfo(librepo.LRI_UPDATE))
        h.setopt(librepo.LRO_UPDATE,  False)
        self.assertFalse(h.getinfo(librepo.LRI_UPDATE))
        h.setopt(librepo.LRO_UPDATE,  1)
        self.assertTrue(h.getinfo(librepo.LRI_UPDATE))
        h.setopt(librepo.LRO_UPDATE,  0)
        self.assertFalse(h.getinfo(librepo.LRI_UPDATE))

        self.assertEqual(h.getinfo(librepo.LRI_URLS), [])
        h.setopt(librepo.LRO_URLS, ["http://foo"])
        self.assertEqual(h.getinfo(librepo.LRI_URLS), ["http://foo"])
        h.setopt(librepo.LRO_URLS, [""])
        self.assertEqual(h.getinfo(librepo.LRI_URLS), [""])
        h.setopt(librepo.LRO_URLS, None)
        self.assertEqual(h.getinfo(librepo.LRI_URLS), [])

        self.assertEqual(h.getinfo(librepo.LRI_MIRRORLIST), None)
        h.setopt(librepo.LRO_MIRRORLIST, "http://ml")
        self.assertEqual(h.getinfo(librepo.LRI_MIRRORLIST), "http://ml")
        h.setopt(librepo.LRO_MIRRORLIST, "")
        self.assertEqual(h.getinfo(librepo.LRI_MIRRORLIST), "")
        h.setopt(librepo.LRO_MIRRORLIST, None)
        self.assertEqual(h.getinfo(librepo.LRI_MIRRORLIST), None)

        self.assertFalse(h.getinfo(librepo.LRI_LOCAL))
        h.setopt(librepo.LRO_LOCAL,  1)
        self.assertTrue(h.getinfo(librepo.LRI_LOCAL))
        h.setopt(librepo.LRO_LOCAL,  0)
        self.assertFalse(h.getinfo(librepo.LRI_LOCAL))

        self.assertFalse(h.getinfo(librepo.LRI_PROGRESSCB))
        h.setopt(librepo.LRO_PROGRESSCB, foo_cb)
        self.assertEqual(h.getinfo(librepo.LRI_PROGRESSCB), foo_cb)
        h.setopt(librepo.LRO_PROGRESSCB, None)
        self.assertFalse(h.getinfo(librepo.LRI_PROGRESSCB))

        data = {'a':'foo'}

        self.assertFalse(h.getinfo(librepo.LRI_PROGRESSDATA))
        h.setopt(librepo.LRO_PROGRESSDATA, data)
        self.assertEqual(h.getinfo(librepo.LRI_PROGRESSDATA), data)
        h.setopt(librepo.LRO_PROGRESSDATA, None)
        self.assertFalse(h.getinfo(librepo.LRI_PROGRESSDATA))

        self.assertEqual(h.getinfo(librepo.LRI_DESTDIR), None)
        h.setopt(librepo.LRO_DESTDIR,  "foodir")
        self.assertEqual(h.getinfo(librepo.LRI_DESTDIR), "foodir")
        h.setopt(librepo.LRO_DESTDIR,  None)
        self.assertEqual(h.getinfo(librepo.LRI_DESTDIR), None)
        h.setopt(librepo.LRO_DESTDIR,  "")
        self.assertEqual(h.getinfo(librepo.LRI_DESTDIR), "")

        self.assertEqual(h.getinfo(librepo.LRI_USERAGENT), None)
        h.setopt(librepo.LRO_USERAGENT,  "librepo/0.0")
        self.assertEqual(h.getinfo(librepo.LRI_USERAGENT), "librepo/0.0")
        h.setopt(librepo.LRO_USERAGENT,  None)
        self.assertEqual(h.getinfo(librepo.LRI_USERAGENT), None)
        h.setopt(librepo.LRO_USERAGENT,  "")
        self.assertEqual(h.getinfo(librepo.LRI_USERAGENT), "")

        self.assertEqual(h.getinfo(librepo.LRI_YUMDLIST), None)
        self.assertEqual(h.getinfo(librepo.LRI_RPMMDDLIST), None)
        h.setopt(librepo.LRO_YUMDLIST,  ["primary", "other"])
        self.assertEqual(h.getinfo(librepo.LRI_YUMDLIST), ["primary", "other"])
        self.assertEqual(h.getinfo(librepo.LRI_RPMMDDLIST), ["primary", "other"])
        h.setopt(librepo.LRO_YUMDLIST,  [])
        self.assertEqual(h.getinfo(librepo.LRI_YUMDLIST), [])
        self.assertEqual(h.getinfo(librepo.LRI_RPMMDDLIST), [])
        h.setopt(librepo.LRO_YUMDLIST,  [None])
        self.assertEqual(h.getinfo(librepo.LRI_YUMDLIST), [])
        self.assertEqual(h.getinfo(librepo.LRI_RPMMDDLIST), [])
        h.setopt(librepo.LRO_YUMDLIST,  None)
        self.assertEqual(h.getinfo(librepo.LRI_YUMDLIST), None)
        self.assertEqual(h.getinfo(librepo.LRI_RPMMDDLIST), None)

        h.setopt(librepo.LRO_RPMMDDLIST,  ["primary", "other"])
        self.assertEqual(h.getinfo(librepo.LRI_YUMDLIST), ["primary", "other"])
        self.assertEqual(h.getinfo(librepo.LRI_RPMMDDLIST), ["primary", "other"])

        self.assertEqual(h.getinfo(librepo.LRI_YUMBLIST), None)
        self.assertEqual(h.getinfo(librepo.LRI_RPMMDBLIST), None)
        h.setopt(librepo.LRO_YUMBLIST,  ["primary", "other"])
        self.assertEqual(h.getinfo(librepo.LRI_YUMBLIST), ["primary", "other"])
        self.assertEqual(h.getinfo(librepo.LRI_RPMMDBLIST), ["primary", "other"])
        h.setopt(librepo.LRO_YUMBLIST,  [])
        self.assertEqual(h.getinfo(librepo.LRI_YUMBLIST), [])
        self.assertEqual(h.getinfo(librepo.LRI_RPMMDBLIST), [])
        h.setopt(librepo.LRO_YUMBLIST,  [None])
        self.assertEqual(h.getinfo(librepo.LRI_YUMBLIST), [])
        self.assertEqual(h.getinfo(librepo.LRI_RPMMDBLIST), [])

        self.assertEqual(h.getinfo(librepo.LRI_MAXMIRRORTRIES), 0)
        h.setopt(librepo.LRO_MAXMIRRORTRIES, 1)
        self.assertEqual(h.getinfo(librepo.LRI_MAXMIRRORTRIES), 1)
        h.setopt(librepo.LRO_MAXMIRRORTRIES, None)
        self.assertEqual(h.getinfo(librepo.LRI_MAXMIRRORTRIES), 0)

        self.assertEqual(h.getinfo(librepo.LRI_VARSUB), None)
        h.setopt(librepo.LRO_VARSUB, [("bar", "foo")])
        self.assertEqual(h.getinfo(librepo.LRI_VARSUB), [("bar", "foo")])
        h.setopt(librepo.LRO_VARSUB, None)
        self.assertEqual(h.getinfo(librepo.LRI_VARSUB), None)

        self.assertEqual(h.getinfo(librepo.LRI_FASTESTMIRROR), False)
        h.setopt(librepo.LRO_FASTESTMIRROR, True)
        self.assertEqual(h.getinfo(librepo.LRI_FASTESTMIRROR), True)
        h.setopt(librepo.LRO_FASTESTMIRROR, False)
        self.assertEqual(h.getinfo(librepo.LRI_FASTESTMIRROR), False)

        self.assertEqual(h.getinfo(librepo.LRI_LOWSPEEDTIME), 120)
        h.setopt(librepo.LRO_LOWSPEEDTIME, 30)
        self.assertEqual(h.getinfo(librepo.LRI_LOWSPEEDTIME), 30)
        h.setopt(librepo.LRO_LOWSPEEDTIME, None)
        self.assertEqual(h.getinfo(librepo.LRI_LOWSPEEDTIME), 120)

        self.assertEqual(h.getinfo(librepo.LRI_LOWSPEEDLIMIT), 1000)
        h.setopt(librepo.LRO_LOWSPEEDLIMIT, 123)
        self.assertEqual(h.getinfo(librepo.LRI_LOWSPEEDLIMIT), 123)
        h.setopt(librepo.LRO_LOWSPEEDLIMIT, None)
        self.assertEqual(h.getinfo(librepo.LRI_LOWSPEEDLIMIT), 1000)

        self.assertFalse(h.getinfo(librepo.LRI_HMFCB))
        h.setopt(librepo.LRO_HMFCB, foo_hmfcb)
        self.assertEqual(h.getinfo(librepo.LRI_HMFCB), foo_hmfcb)
        h.setopt(librepo.LRO_HMFCB, None)
        self.assertFalse(h.getinfo(librepo.LRI_HMFCB))

        self.assertTrue(h.getinfo(librepo.LRI_SSLVERIFYPEER))
        h.setopt(librepo.LRO_SSLVERIFYPEER, 0)
        self.assertEqual(h.getinfo(librepo.LRI_SSLVERIFYPEER), False)
        h.setopt(librepo.LRO_SSLVERIFYPEER, None)
        self.assertTrue(h.getinfo(librepo.LRI_SSLVERIFYPEER))

        self.assertTrue(h.getinfo(librepo.LRI_SSLVERIFYHOST))
        h.setopt(librepo.LRO_SSLVERIFYHOST, 0)
        self.assertEqual(h.getinfo(librepo.LRI_SSLVERIFYHOST), False)
        h.setopt(librepo.LRO_SSLVERIFYHOST, None)
        self.assertTrue(h.getinfo(librepo.LRI_SSLVERIFYHOST))

        self.assertEqual(h.getinfo(librepo.LRI_SSLCLIENTCERT), None)
        h.setopt(librepo.LRO_SSLCLIENTCERT, "/etc/cert.pem")
        self.assertEqual(h.getinfo(librepo.LRI_SSLCLIENTCERT), "/etc/cert.pem")
        h.setopt(librepo.LRO_SSLCLIENTCERT, "")
        self.assertEqual(h.getinfo(librepo.LRI_SSLCLIENTCERT), "")
        h.setopt(librepo.LRO_SSLCLIENTCERT, None)
        self.assertEqual(h.getinfo(librepo.LRI_SSLCLIENTCERT), None)

        self.assertEqual(h.getinfo(librepo.LRI_SSLCLIENTKEY), None)
        h.setopt(librepo.LRO_SSLCLIENTKEY, "/etc/cert.key")
        self.assertEqual(h.getinfo(librepo.LRI_SSLCLIENTKEY), "/etc/cert.key")
        h.setopt(librepo.LRO_SSLCLIENTKEY, "")
        self.assertEqual(h.getinfo(librepo.LRI_SSLCLIENTKEY), "")
        h.setopt(librepo.LRO_SSLCLIENTKEY, None)
        self.assertEqual(h.getinfo(librepo.LRI_SSLCLIENTKEY), None)

        self.assertEqual(h.getinfo(librepo.LRI_SSLCACERT), None)
        h.setopt(librepo.LRO_SSLCACERT, "/etc/ca.pem")
        self.assertEqual(h.getinfo(librepo.LRI_SSLCACERT), "/etc/ca.pem")
        h.setopt(librepo.LRO_SSLCACERT, "")
        self.assertEqual(h.getinfo(librepo.LRI_SSLCACERT), "")
        h.setopt(librepo.LRO_SSLCACERT, None)
        self.assertEqual(h.getinfo(librepo.LRI_SSLCACERT), None)

        self.assertEqual(h.getinfo(librepo.LRI_IPRESOLVE), librepo.IPRESOLVE_WHATEVER)
        h.setopt(librepo.LRO_IPRESOLVE, librepo.IPRESOLVE_V6)
        self.assertEqual(h.getinfo(librepo.LRI_IPRESOLVE), librepo.IPRESOLVE_V6)
        h.setopt(librepo.LRO_IPRESOLVE, None)
        self.assertEqual(h.getinfo(librepo.LRI_IPRESOLVE), librepo.IPRESOLVE_WHATEVER)

        self.assertEqual(h.getinfo(librepo.LRI_ALLOWEDMIRRORFAILURES), 4)
        h.setopt(librepo.LRO_ALLOWEDMIRRORFAILURES, 1)
        self.assertEqual(h.getinfo(librepo.LRI_ALLOWEDMIRRORFAILURES), 1)
        h.setopt(librepo.LRO_ALLOWEDMIRRORFAILURES, None)

        self.assertEqual(h.getinfo(librepo.LRI_ADAPTIVEMIRRORSORTING), 1)
        h.setopt(librepo.LRO_ADAPTIVEMIRRORSORTING, 0)
        self.assertEqual(h.getinfo(librepo.LRI_ADAPTIVEMIRRORSORTING), 0)
        h.setopt(librepo.LRO_ADAPTIVEMIRRORSORTING, None)
        self.assertEqual(h.getinfo(librepo.LRI_ADAPTIVEMIRRORSORTING), 1)

        self.assertEqual(h.getinfo(librepo.LRI_GNUPGHOMEDIR), None)
        h.setopt(librepo.LRO_GNUPGHOMEDIR,  "/tmp/keyring/")
        self.assertEqual(h.getinfo(librepo.LRI_GNUPGHOMEDIR), "/tmp/keyring/")
        h.setopt(librepo.LRO_GNUPGHOMEDIR,  None)
        self.assertEqual(h.getinfo(librepo.LRI_GNUPGHOMEDIR), None)
        h.setopt(librepo.LRO_GNUPGHOMEDIR,  "")
        self.assertEqual(h.getinfo(librepo.LRI_GNUPGHOMEDIR), "")

        self.assertEqual(h.getinfo(librepo.LRI_FASTESTMIRRORTIMEOUT), 2.0)
        h.setopt(librepo.LRO_FASTESTMIRRORTIMEOUT,  32.256)
        self.assertEqual(h.getinfo(librepo.LRI_FASTESTMIRRORTIMEOUT), 32.256)
        h.setopt(librepo.LRO_FASTESTMIRRORTIMEOUT,  None)
        self.assertEqual(h.getinfo(librepo.LRI_FASTESTMIRRORTIMEOUT), 2.0)

        self.assertEqual(h.getinfo(librepo.LRI_HTTPHEADER), None)
        h.setopt(librepo.LRO_HTTPHEADER, ["Accept: text/xml", "charsets: utf-8"])
        self.assertEqual(h.getinfo(librepo.LRI_HTTPHEADER),
                         ["Accept: text/xml", "charsets: utf-8"])
        h.setopt(librepo.LRO_HTTPHEADER, None)
        self.assertEqual(h.getinfo(librepo.LRI_HTTPHEADER), None)

        self.assertEqual(h.getinfo(librepo.LRI_HTTPAUTHMETHODS), librepo.LR_AUTH_BASIC)
        h.setopt(librepo.LRO_HTTPAUTHMETHODS,  librepo.LR_AUTH_DIGEST)
        self.assertEqual(h.getinfo(librepo.LRI_HTTPAUTHMETHODS), librepo.LR_AUTH_DIGEST)
        h.setopt(librepo.LRO_HTTPAUTHMETHODS,  None)
        self.assertEqual(h.getinfo(librepo.LRI_HTTPAUTHMETHODS), librepo.LR_AUTH_BASIC)

        self.assertEqual(h.getinfo(librepo.LRI_PROXYAUTHMETHODS), librepo.LR_AUTH_BASIC)
        h.setopt(librepo.LRO_PROXYAUTHMETHODS,  librepo.LR_AUTH_DIGEST)
        self.assertEqual(h.getinfo(librepo.LRI_PROXYAUTHMETHODS), librepo.LR_AUTH_DIGEST)
        h.setopt(librepo.LRO_PROXYAUTHMETHODS,  None)
        self.assertEqual(h.getinfo(librepo.LRI_PROXYAUTHMETHODS), librepo.LR_AUTH_BASIC)

        self.assertEqual(h.getinfo(librepo.LRI_FTPUSEEPSV), 1)
        h.setopt(librepo.LRO_FTPUSEEPSV, 0)
        self.assertEqual(h.getinfo(librepo.LRI_FTPUSEEPSV), 0)
        h.setopt(librepo.LRO_FTPUSEEPSV, None)
        self.assertEqual(h.getinfo(librepo.LRI_FTPUSEEPSV), 1)


    def test_handle_setget_attr(self):
        """No exception should be raised."""
        h = librepo.Handle()

        self.assertFalse(h.update)
        h.update = True
        self.assertTrue(h.update)
        h.update = False
        self.assertFalse(h.update)
        h.update = 1
        self.assertTrue(h.update)
        h.update = 0
        self.assertFalse(h.update)

        self.assertEqual(h.urls, [])
        h.urls = "http://foo"
        self.assertEqual(h.urls, ["http://foo"])
        h.urls = ""
        self.assertEqual(h.urls, [""])
        h.urls = None
        self.assertEqual(h.urls, [])

        self.assertEqual(h.mirrorlist, None)
        h.mirrorlist = "http://ml"
        self.assertEqual(h.mirrorlist, "http://ml")
        h.mirrorlist = ""
        self.assertEqual(h.mirrorlist, "")
        h.mirrorlist = None
        self.assertEqual(h.mirrorlist, None)

        self.assertFalse(h.local)
        h.local =  1
        self.assertTrue(h.local)
        h.local =  0
        self.assertFalse(h.local)

        self.assertFalse(h.progresscb)
        h.progresscb = foo_cb
        self.assertEqual(h.progresscb, foo_cb)
        h.progresscb = None
        self.assertFalse(h.progresscb)

        data = {'a':'foo'}

        self.assertFalse(h.progressdata)
        h.progressdata = data
        self.assertEqual(h.progressdata, data)
        h.progressdata = None
        self.assertFalse(h.progressdata)

        self.assertEqual(h.destdir, None)
        h.destdir =  "foodir"
        self.assertEqual(h.destdir, "foodir")
        h.destdir =  None
        self.assertEqual(h.destdir, None)
        h.destdir =  ""
        self.assertEqual(h.destdir, "")

        self.assertEqual(h.useragent, None)
        h.useragent =  "librepo/0.0"
        self.assertEqual(h.useragent, "librepo/0.0")
        h.useragent =  None
        self.assertEqual(h.useragent, None)
        h.useragent =  ""
        self.assertEqual(h.useragent, "")

        self.assertEqual(h.yumdlist, None)
        h.yumdlist =  ["primary", "other"]
        self.assertEqual(h.yumdlist, ["primary", "other"])
        h.yumdlist =  []
        self.assertEqual(h.yumdlist, [])
        h.yumdlist =  [None]
        self.assertEqual(h.yumdlist, [])

        self.assertEqual(h.yumblist, None)
        h.yumblist =  ["primary", "other"]
        self.assertEqual(h.yumblist, ["primary", "other"])
        h.yumblist =  []
        self.assertEqual(h.yumblist, [])
        h.yumblist =  [None]
        self.assertEqual(h.yumblist, [])

        self.assertEqual(h.maxmirrortries, 0)
        h.maxmirrortries = 1
        self.assertEqual(h.maxmirrortries, 1)
        h.maxmirrortries = None
        self.assertEqual(h.maxmirrortries, 0)

        self.assertEqual(h.varsub, None)
        h.varsub = [("foo", "bar")]
        self.assertEqual(h.varsub, [("foo", "bar")])
        h.varsub = None
        self.assertEqual(h.varsub, None)

        self.assertEqual(h.fastestmirror, False)
        h.fastestmirror = True
        self.assertEqual(h.fastestmirror, True)
        h.fastestmirror = False
        self.assertEqual(h.fastestmirror, False)

        self.assertEqual(h.lowspeedtime, 120)
        h.lowspeedtime = 20
        self.assertEqual(h.lowspeedtime, 20)
        h.lowspeedtime = None
        self.assertEqual(h.lowspeedtime, 120)

        self.assertEqual(h.lowspeedlimit, 1000)
        h.lowspeedlimit = 55
        self.assertEqual(h.lowspeedlimit, 55)
        h.lowspeedlimit = None
        self.assertEqual(h.lowspeedlimit, 1000)

        self.assertFalse(h.hmfcb)
        h.hmfcb = foo_hmfcb
        self.assertEqual(h.hmfcb, foo_hmfcb)
        h.hmfcb = None
        self.assertFalse(h.hmfcb)

        self.assertTrue(h.sslverifypeer)
        h.sslverifypeer = False
        self.assertEqual(h.sslverifypeer, False)
        h.sslverifypeer = None
        self.assertTrue(h.sslverifypeer)

        self.assertTrue(h.sslverifyhost)
        h.sslverifyhost = False
        self.assertEqual(h.sslverifyhost, False)
        h.sslverifyhost = None
        self.assertTrue(h.sslverifyhost)

        self.assertEqual(h.ipresolve, librepo.IPRESOLVE_WHATEVER)
        h.ipresolve = librepo.IPRESOLVE_V4
        self.assertEqual(h.ipresolve, librepo.IPRESOLVE_V4)
        h.ipresolve = None
        self.assertEqual(h.ipresolve, librepo.IPRESOLVE_WHATEVER)

        self.assertEqual(h.allowedmirrorfailures, 4)
        h.allowedmirrorfailures = 1
        self.assertEqual(h.allowedmirrorfailures, 1)
        h.allowedmirrorfailures = None
        self.assertEqual(h.allowedmirrorfailures, 4)

        self.assertEqual(h.adaptivemirrorsorting, 1)
        h.adaptivemirrorsorting = 0
        self.assertEqual(h.adaptivemirrorsorting, 0)
        h.adaptivemirrorsorting = None
        self.assertEqual(h.adaptivemirrorsorting, 1)

        self.assertEqual(h.gnupghomedir, None)
        h.gnupghomedir =  "/tmp/keyring"
        self.assertEqual(h.gnupghomedir, "/tmp/keyring")
        h.gnupghomedir =  None
        self.assertEqual(h.gnupghomedir, None)
        h.gnupghomedir =  ""
        self.assertEqual(h.gnupghomedir, "")

        self.assertEqual(h.fastestmirrortimeout, 2.0)
        h.fastestmirrortimeout = 3.14
        self.assertEqual(h.fastestmirrortimeout, 3.14)
        h.fastestmirrortimeout = None
        self.assertEqual(h.fastestmirrortimeout, 2.0)

        self.assertEqual(h.httpheader, None)
        h.httpheader = ["Accept: text/xml", "charsets: utf-8"]
        self.assertEqual(h.httpheader, ["Accept: text/xml", "charsets: utf-8"])
        h.httpheader = None
        self.assertEqual(h.httpheader, None)

        self.assertEqual(h.httpauthmethods, librepo.LR_AUTH_BASIC)
        h.httpauthmethods = librepo.LR_AUTH_NTLM
        self.assertEqual(h.httpauthmethods, librepo.LR_AUTH_NTLM)
        h.httpauthmethods = None
        self.assertEqual(h.httpauthmethods, librepo.LR_AUTH_BASIC)

        self.assertEqual(h.proxyauthmethods, librepo.LR_AUTH_BASIC)
        h.proxyauthmethods = librepo.LR_AUTH_NTLM
        self.assertEqual(h.proxyauthmethods, librepo.LR_AUTH_NTLM)
        h.proxyauthmethods = None
        self.assertEqual(h.proxyauthmethods, librepo.LR_AUTH_BASIC)

        self.assertTrue(h.ftpuseepsv)
        h.ftpuseepsv = 0
        self.assertFalse(h.ftpuseepsv)
        h.ftpuseepsv = 1
        self.assertTrue(h.ftpuseepsv)
        h.ftpuseepsv = False
        self.assertFalse(h.ftpuseepsv)
        h.ftpuseepsv = True
        self.assertTrue(h.ftpuseepsv)
        h.ftpuseepsv = 0
        h.ftpuseepsv = None
        self.assertTrue(h.ftpuseepsv)


    def test_handle_setopt_none_value(self):
        """Using None in setopt."""
        h = librepo.Handle()

        h.setopt(librepo.LRO_LOCAL, None)
        h.local = None
        h.setopt(librepo.LRO_HTTPAUTH, None)
        h.httpauth = None
        h.setopt(librepo.LRO_USERPWD, None)
        h.userpwd = None
        h.setopt(librepo.LRO_PROXY, None)
        h.proxy = None
        h.setopt(librepo.LRO_PROXYPORT, None)       # None sets default value
        h.proxyport = None
        h.setopt(librepo.LRO_PROXYTYPE, None)
        h.proxytype = None
        h.setopt(librepo.LRO_PROXYAUTH, None)
        h.proxyauth = None
        h.setopt(librepo.LRO_PROXYUSERPWD, None)
        h.proxyuserpwd = None
        h.setopt(librepo.LRO_PROGRESSDATA, None)
        h.progressdata = None
        h.setopt(librepo.LRO_MAXSPEED, None)        # None sets default value
        h.maxspeed = None
        h.setopt(librepo.LRO_CONNECTTIMEOUT, None)  # None sets default value
        h.connecttimeout = None
        h.setopt(librepo.LRO_IGNOREMISSING, None)
        h.ignoremissing = None
        h.setopt(librepo.LRO_INTERRUPTIBLE, None)
        h.interruptible = None
        h.setopt(librepo.LRO_USERAGENT, None)
        h.useragent = None
        h.setopt(librepo.LRO_FETCHMIRRORS, None)
        h.fetchmirrors = None
        h.setopt(librepo.LRO_MAXMIRRORTRIES, None)
        h.maxmirrortries = None
        h.setopt(librepo.LRO_VARSUB, None)
        h.varsub = None
        h.setopt(librepo.LRO_FASTESTMIRROR, None)
        h.fastestmirror = None
        h.setopt(librepo.LRO_FASTESTMIRRORCACHE, None)
        h.fastestmirrorcache = None
        h.setopt(librepo.LRO_FASTESTMIRRORMAXAGE, None)
        h.fastestmirrormaxage = None
        h.setopt(librepo.LRO_FASTESTMIRRORDATA, None)
        h.fastestmirrordata = None
        h.setopt(librepo.LRO_LOWSPEEDTIME, None)
        h.lowspeedtime = None
        h.setopt(librepo.LRO_LOWSPEEDLIMIT, None)
        h.lowspeedlimit = None
        h.setopt(librepo.LRO_GPGCHECK, None)
        h.gpgcheck = None
        h.setopt(librepo.LRO_CHECKSUM, None)
        h.checksum = None
        h.setopt(librepo.LRO_LOWSPEEDTIME, None)
        h.lowspeedtime = None
        h.setopt(librepo.LRO_LOWSPEEDLIMIT, None)
        h.lowspeedlimit = None

        h.setopt(librepo.LRO_PROGRESSCB, None)
        h.progresscb = None
        h.setopt(librepo.LRO_PROGRESSCB, foo_cb)
        h.progresscb = foo_cb
        h.setopt(librepo.LRO_PROGRESSCB, None)
        h.progresscb = None

        def fmcallback(userdata, stage, data):
            pass
        h.setopt(librepo.LRO_FASTESTMIRRORCB, None)
        h.fastestmirrorcb = None
        h.setopt(librepo.LRO_FASTESTMIRRORCB, fmcallback)
        h.fastestmirrorcb = fmcallback
        h.setopt(librepo.LRO_FASTESTMIRRORCB, None)
        h.fastestmirror = None

        h.setopt(librepo.LRO_HMFCB, None)
        h.hmfcb = None
        h.setopt(librepo.LRO_HMFCB, foo_hmfcb)
        h.hmfcb = foo_hmfcb
        h.setopt(librepo.LRO_HMFCB, None)
        h.hmfcb = None

        h.setopt(librepo.LRO_SSLVERIFYPEER, None)
        h.sslverifypeer = None
        h.setopt(librepo.LRO_SSLVERIFYHOST, None)
        h.sslverifyhost = None
        h.setopt(librepo.LRO_SSLCLIENTCERT, None)
        h.sslclientcert = None
        h.setopt(librepo.LRO_SSLCLIENTKEY, None)
        h.sslclientkey = None
        h.setopt(librepo.LRO_SSLCACERT, None)
        h.sslcacert = None

        h.setopt(librepo.LRO_IPRESOLVE, None)
        h.ipresolve = None
        h.setopt(librepo.LRO_ALLOWEDMIRRORFAILURES, None)
        h.allowedmirrorfailures = None
        h.setopt(librepo.LRO_ADAPTIVEMIRRORSORTING, None)
        h.adaptivemirrorsorting = None

        h.setopt(librepo.LRO_GNUPGHOMEDIR, None)
        h.gnupghomedir = None
        h.setopt(librepo.LRO_FASTESTMIRRORTIMEOUT, None)
        h.fastestmirrortimeout = None
        h.setopt(librepo.LRO_HTTPHEADER, None)
        h.httpheader = None
        h.setopt(librepo.LRO_FTPUSEEPSV, None)
        h.ftpuseepsv = None

        h.httpauthmethods = None
        h.proxyauthmethods = None
