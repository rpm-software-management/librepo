import librepo
import os.path
import time
from multiprocessing import Process
try:
    import unittest2 as unittest
except ImportError:
    import unittest

MOCKURL="http://127.0.0.1:5000/"

class TestCase(unittest.TestCase):
    testdata_dir = os.path.normpath(os.path.join(__file__, "../../../test_data"))

class TestCaseWithFlask(TestCase):
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
