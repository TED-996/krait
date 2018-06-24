Krait Documentation Home
========================

.. toctree::
   :hidden:

   self

.. toctree::
   :maxdepth: 2
   :caption: Reference:

   krait

What is Krait?
==============

Krait is an HTTP server with Python server-side scripting. Its main purpose is to be as simple and elegant
to use as possible, while having no major drawbacks (no important missing features, no performance,
stability or security issues)

It is written in C++, but you'll be coding the backend only in Python. Its Python API is built with simplicity
and elegance in mind. Right now Krait only runs on Linux, but it also runs perfectly under Windows 10,
in the Linux Subsystem.

What can Krait offer?
=====================

Krait has most of the features that web apps need:

* Serving static files
* Routing requests based on URL and HTTP method
* Running Python scripts in response to HTTP requests
* Full control of responses & cookies from Python
* MVC framework with fully-featured templating language (also simple, and elegant!)
* Near-complete compatibility with HTTP/1.1
* HTTPS support
* Websockets support
* Open-source, under MIT

What is still on the way?
=========================

The server is still in development, so a few features are still on the way. Before v1.0, the following
features are planned to be complete:

* Easy to understand and pinpoint error messages
* Comprehensive documentation, tutorials, and examples
* UTF-8 compatibility
* More flexible routing
* Hopefully, a Windows port
* Becoming even more elegant, and simple.


Python Documentation
====================

Krait has its own Python package, appropriately named ``krait``.
Access its documentation :ref:`here <krait_package>`.


Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
