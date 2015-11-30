.. image:: https://travis-ci.org/skozlovf/luax.png?branch=master
   :target: https://travis-ci.org/skozlovf/luax


``luax`` is a simple template-based c++ to lua wrapper.

It's pretty low level and allows to do only one thing - expose c++ class to lua,
there is no function arguments mapping, return value policies
or something like that; you will work with lua stack directly.

Features:

* Methods and static functions binding.
* Properties.
* Enums.
* Single inheritance support.
* Type functions and enums.
* Simple GC.
* Requiers C++11 support.
* Works with lua 5.1 - 5.3, luajit 2.0/2.1.
