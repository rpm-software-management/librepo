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

.. class:: LibrepoException

Value of this exception is tuple with three elements:
``(return code, error message, general error message)``

* Return code is a value from: :ref:`error-codes-label`.
* String with a descriptive description of the error.
* General error message based on rc (feel free to ignore this message)


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

Some tricky/unsafe stuff which can be useful during developing and debugging,
but definitely shouldn't be used in production.

.. autofunction:: set_debug_log_handler
