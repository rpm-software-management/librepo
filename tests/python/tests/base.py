import sys
import time
import socket
import ctypes
import os.path
import requests
from multiprocessing import Process, Value
from tests.servermock.server import app
try:
    import unittest2 as unittest
except ImportError:
    import unittest

import librepo

try:
    from gpg import Context
except ImportError:
    import gpgme

    class Context(object):
        def __init__(self):
            self.__dict__["ctx"] = gpgme.Context()

        def __enter__(self):
            return self

        def __exit__(self, type, value, tb):
            pass

        @property
        def armor(self):
            return self.ctx.armor

        @armor.setter
        def armor(self, value):
            self.ctx.armor = value

        def op_import(self, key_fo):
            self.ctx.import_(key_fo)

        def op_export(self, pattern, mode, keydata):
            self.ctx.export(pattern, keydata)

        def __getattr__(self, name):
            return getattr(self.ctx, name)


MOCKURL_TEMPLATE = "http://127.0.0.1:%d/"
TEST_DATA = os.path.normpath(os.path.join(__file__, "../../../test_data"))

class TestCase(unittest.TestCase):
    pass


class TestCaseWithApp(TestCase):
    application = NotImplemented

    @classmethod
    def setUpClass(cls):
        cls.server = Process(target=cls.application.run)
        cls.server.start()
        time.sleep(0.5)

    @classmethod
    def tearDownClass(cls):
        cls.server.terminate()
        cls.server.join()


def application(port):
    """Sometimes, the port is used, in that case, use different port"""

    while True:
        try:
            port_val = port.value
            app._librepo_port = port_val    # Store used port into Flask app
            app.run(port=port_val)
        except socket.error as e:
            if e.errno == 98:
                # Address already in use
                port.value += 1
                continue
            raise
        break


class TestCaseWithFlask(TestCase):
    _TS_PORT = Value(ctypes.c_int, 5000)
    MOCKURL = None
    PORT = -1

    @classmethod
    def setUpClass(cls):
        cls.server = Process(target=application, args=(cls._TS_PORT,))
        cls.server.start()
        cls.MOCKURL = MOCKURL_TEMPLATE % cls._TS_PORT.value
        cls.PORT = cls._TS_PORT.value
        # Wait for the server to start (max 5 seconds)
        for i in range(50):
            try:
                requests.get(cls.MOCKURL, timeout=0.1)
                break
            except (requests.exceptions.ConnectionError):
                time.sleep(0.1)
            except (requests.exceptions.Timeout):
                pass
        else:
            cls.tearDownClass()
            raise Exception("Server didn't start even after 5 seconds.")

    @classmethod
    def tearDownClass(cls):
        cls.server.terminate()
        cls.server.join()
