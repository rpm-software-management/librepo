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


Python tests with Flask
=======================

Some python tests use **Flask** python framework (http://flask.pocoo.org/)
to simulate web server. The server is started automatically during that tests.

*TestCases with Flask inherit from TestCaseWithApp class.*

The Flask is then set as the app to the test case by 'application' class attribute.

If you want to start server manually::

$ python python/tests/servermock/server.py

Url examples
------------
http://127.0.0.1:5000/yum/auth_basic/static/01/repodata/repomd.xml
 * This url provides basic authentification

http://127.0.0.1:5000/yum/badgpg/static/01/repodata/repomd.xml.asc
 * This url in fact returns .../repodata/repomd.xml.asc.bad file

etc..

Modularity of tests with Flask
------------------------------

Flask tests are intended to be modular.

Modularity is provided by use of http://flask.pocoo.org/docs/blueprints/
Currently there is only one module (blueprint) for yum repo mocking
in servermock/yum_mock.

Repos for test with Flask
-------------------------

Currently each module (blueprint) uses its own data (repositories,
packages, ..).

E.g. for yum mocking module: servermock/yum_mock/static/

Configuration and globals for tests with Flask
----------------------------------------------

Each module (blueprint) has its own configuration and globals which uses
and provides to tests which use the module.

E.g. for yum mocking module: servermock/yum_mock/config.py
