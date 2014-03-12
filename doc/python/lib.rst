.. _lib:

The Librepo Library
===================

Classes
-------

Librepo includes several classes:

.. toctree::
   
   handle
   result
   packagetarget

Exceptions
----------

Librepo module has only one own exception.

.. autoclass:: librepo.LibrepoException

Constants
----------

.. automodule:: librepo

Functions
---------

.. autofunction:: checksum_str_to_type
.. autofunction:: download_packages
.. autofunction:: download_url
.. autofunction:: yum_repomd_get_age

Debugging
---------

Some tricky/unsafe stuff which can be usefull during developing and debugging,
but definitely shouldn't be used in production.

.. autofunction:: set_debug_log_handler
