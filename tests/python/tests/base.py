import sys
import time
import socket
import ctypes
import os.path
from multiprocessing import Process, Value
from tests.servermock.server import app
try:
    import unittest2 as unittest
except ImportError:
    import unittest

import librepo

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
        time.sleep(0.5)
        cls.MOCKURL = MOCKURL_TEMPLATE % cls._TS_PORT.value
        cls.PORT = cls._TS_PORT.value

    @classmethod
    def tearDownClass(cls):
        cls.server.terminate()
        cls.server.join()
