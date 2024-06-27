C tests
=======
C tests uses the Check unit testing framework (http://check.sourceforge.net/).

Run (from your checkout dir)::

build/tests/test_main tests/test_data/

*Tip:* Use Valgrind (http://valgrind.org/) with C tests::

valgrind build/tests/test_main tests/test_data/

Files of C tests
================

**test_data/**
 * Directory with testing repositories, metalink files, etc.

**CMakeLists.txt**
 * CMake config file

**fixtures.c  fixtures.h**
 * Global test data (global test variable, setup and tear down function(s), ...)

**testsys.c  testsys.h**
 * Global test configuration, expected results (constants in testsys.h)
   and useful functions (functions in testsys.h)

**test_main.c**
 * Main file

**test_*.c  test_*.h**
 * Test suites


Python tests http.server
========================

Some python tests use **http.server** python standard library
to simulate web server. The server is started automatically during that tests.

*TestCases with http.server inherit from TestCaseWithServer class.*

If you want to start server manually::

$ python python/tests/servermock/server.py

Url examples
------------
http://127.0.0.1:5000/yum/auth_basic/static/01/repodata/repomd.xml
 * This url provides basic authentication

http://127.0.0.1:5000/yum/badgpg/static/01/repodata/repomd.xml.asc
 * This url in fact returns .../repodata/repomd.xml.asc.bad file

etc..

Repos for test with http.server
-------------------------------

All data (repositories, packages, ..) are in servermock/yum_mock/static/

Configuration and globals for tests with http.server
----------------------------------------------------

Configuration and globals used by these tests are in servermock/yum_mock/config.py

