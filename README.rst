.. image:: https://travis-ci.org/skozlovf/luax.png?branch=master
   :target: https://travis-ci.org/skozlovf/luax

luax
====

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
* Requires C++11 support.
* Works with lua 5.1 - 5.3, luajit 2.0/2.1.

Fast example:

.. code-block:: c++

    class Point
    {
    public:
        Point(int x = 0, int y = 0);

        int getX(lua_State *L);
        int setX(lua_State *L);

        int getY(lua_State *L);
        int setY(lua_State *L);

        int move(lua_State *L);

    private:
        int m_x;
        int m_y;
    };

    // Wrapper:

    LUAX_TYPE_NAME(Point, "Point")

    LUAX_FUNCTIONS_M_BEGIN(Point)
        LUAX_FUNCTION("move", &Point::move)
    LUAX_FUNCTIONS_END

    LUAX_PROPERTIES_M_BEGIN(Point)
        LUAX_PROPERTY("x", &Point::getX, &Point::setX)
        LUAX_PROPERTY("y", &Point::getY, &Point::setY)
    LUAX_PROPERTIES_END

    // ...

    luax::init(L);
    luax::type<Point>::register_in(L);

    Point pt;
    luax::type<Point>::push(L, &pt, false);
    lua_setglobal(L, "point");

On lua side:

.. code-block:: lua

    point.x = 12
    point.y = 42
    print(point.x, point.y)  -- 12  42

    point:move(10, 11)
    print(point.x, point.y)  -- 10  11

    point.y = point.x + 93
    print(point.x, point.y)  -- 10  103

API Documentation
-----------------

``luax`` is represented by one file ``include/luax.h`` and provides only one
template class ``luax::type``. Another file is ``include/luax_utils.h``,
it provides few help functions to work with lua stack. You can use these files
independently from each other.

To bind a c++ class you will write it's specialization directly
or with help of macros.

Typical workflow to create new lua type:

* Set type name.
* Bind methods, enums, etc.
* Register the type in lua VM.


Bind class using luax::type directly
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

``luax::type`` has methods ``usr_*`` - user hooks, they will be called on
certain parts of type or instance lifetime.

API:

=======================  =======================================================
``usr_name()``           Defines type name. *Required*

``usr_super_name()``     Defines superclass name. *Optional*

``usr_instance_mt()``    User hook. Gets called on type registering after
                         *instance metatable* is initialized, but before
                         any attributes and methods registration. *Optional*

``usr_type_mt()``        User hook. Gets called on type registering after
                         *type metatable* is initialized, but before
                         any attributes and methods registration. *Optional*

``usr_getter()``         User hook. Fallback method for the ``__index`` event.
                         Gets called if no getter is found in the
                         instance. *Optional*

``usr_setter()``         User hook. Fallback method for the ``__newindex``
                         event. Gets called if no setter is found in the
                         instance. *Optional*

``usr_gc()``             User hook. Gets called instead of default
                         implementation on GC. *Optional*

``usr_constructor()``    Defines constructor. If not defined then it will be
                         impossible to create instance on the lua side.
                         *Optional*

``functions[]``          List of free functions to be used as instance methods.
                         *Optional*

``methods[]``            List of class methods to be used as instance methods.
                         *Optional*

``func_properties[]``    List of free functions to be used as instance
                         getters/setters. *Optional*

``method_properties[]``  List of class methods to be used as instance
                         getters/setters. *Optional*

``type_enums[]``         List of enum values. *Optional*

``type_functions[]``     List of free functions to be used as type functions.

``register_in()``        Register type in lua VM.

``push()``               Push instance on stack.

``get()``                Get instance from stack.

``check_get()``          Get instance from stack and check if it valid.
=======================  =======================================================


Methods and free functions signatures are the same as ``lua_CFunction``:

.. code-block:: c++

    class Point
    {
    public:
        ...
        
        int getX(lua_State *L);
        int setX(lua_State *L);
    };

    ...

    static int foo_x(lua_State *L);


Suppose you want to bind ``Point`` class.

At first you define it's lua name:

.. code-block:: c++

    template <> const char* type<Point>::usr_name() { return "Point"; }


If you want to expose methods then you do:

.. code-block:: c++

    template <> Method<Point> type<Point>::methods[] =
    {
        {"getX", &Point::getX},
        {"setX", &Point::setX},
        {0, 0}
    };

If you want to bind free functions as methods then you do:

.. code-block:: c++

    template <> luaL_Reg type<Point>::functions[] = {
        {"getx", foo_x},
        {"gety", foo_y},
        {0, 0}
    };

On lua side:

.. code-block:: lua

    point:getX()
    point:setX()
    point:getx()
    point:gety()


If you want to have properties then you do:

.. code-block:: c++

    template <> MethodProperty<Point> type<Point>::method_properties[] = {
        {"x", &Point::getX, &Point::setX},
        {0, 0, 0}
    };

    // For free functions:

    template <> FuncProperty type<Point>::func_properties[] = {
        {"y", get_y, set_y},
        {0, 0, 0}
    };

On lua side:

.. code-block:: lua

    local a = point.x
    local b = point.y


If you want to bind enum values then use:

.. code-block:: c++

    template <> Enum type<Point>::type_enums[] = {
        {"ENUM1", 10},
        {"ENUM2", 20},
        {0, 0}
    };

On lua side:

.. code-block:: lua

    local a = Point.ENUM1
    local b = Point.ENUM2


You also may have functions attached to type:

.. code-block:: c++

    template <> luaL_Reg type<Point>::type_functions[] = {
        {"func1", func1},
        {"func2", func2},
        {0, 0}
    };

On lua side:

.. code-block:: lua

    Point.func1()
    Point.func2()


Define a constructor to allow type instance creation on lua side:

.. code-block:: c++

    template <> Point* type<Point>::usr_constructor(lua_State *L)
    {
        int x = lua_tointeger(L, 2);
        int y = lua_tointeger(L, 3);
        return new Point(x, y);
    }

On lua side:

.. code-block:: lua

    local p = Point(1, 2)
    print(p.x, p.y)



Before registering any types you have to run ``luax::init``:

.. code-block:: c++

    luax::init(L);

Now you can register your type:

.. code-block:: c++

    luax::type<Point>::register_in(L);


Use ``type::push()`` to put an instance on stack:

.. code-block:: c++

    Point *p = new Point(1, 2);
    luax::type<Point>::push(L, p);
    lua_setglobal(L, "p");

On lua side:

.. code-block:: lua

    local a = p.x
    local b = p.getY()

By default ``p`` will be deleted on GC, if you want to prevent this then pass
``false`` as last parameter:

.. code-block:: c++

    Point p;
    ...
    luax::type<Point>::push(L, &p, false);


Use ``type::get()`` or ``type::check_get()`` to get ``Point`` instance
from the stack:

.. code-block:: c++

    Point *p = luax::type<Point>::get(L, -1);
    Point *p = luax::type<Point>::check_get(L, -1);

``type::check_get()`` will raise an error if value on stack is not a Point
instance.

See complete example in ``tests\LuaxPointExample.cpp``.

Bind class using macro
^^^^^^^^^^^^^^^^^^^^^^

There are few macro to help with type definition:

+-------------------------------------+---------------------------------------+
| Macro                               | Description                           |
+=====================================+=======================================+
| ``LUAX_TYPE_NAME(cls,name)``        | Define type name.                     |
+-------------------------------------+---------------------------------------+
| ``LUAX_TYPE_SUPER_NAME(cls,name)``  | Define superclass name.               |
+-------------------------------------+---------------------------------------+
| ::                                  | Define instance methods.              |
|                                     |                                       |
|     LUAX_FUNCTIONS_BEGIN(cls)       |                                       |
|     LUAX_FUNCTIONS_M_BEGIN(cls)     |                                       |
|     LUAX_FUNCTION(name, f)          |                                       |
|     LUAX_FUNCTIONS_END              |                                       |
+-------------------------------------+---------------------------------------+
| ::                                  | Define instance properties.           |
|                                     |                                       |
|     LUAX_PROPERTIES_BEGIN(cls)      |                                       |
|     LUAX_PROPERTIES_M_BEGIN(cls)    |                                       |
|     LUAX_PROPERTY(name,get,set)     |                                       |
|     LUAX_PROPERTIES_END             |                                       |
+-------------------------------------+---------------------------------------+
| ::                                  | Define enums.                         |
|                                     |                                       |
|     LUAX_TYPE_ENUMS_BEGIN(cls)      |                                       |
|     LUAX_ENUM(name,val)             |                                       |
|     LUAX_TYPE_ENUMS_END             |                                       |
+-------------------------------------+---------------------------------------+
| ::                                  | Define type functions.                |
|                                     |                                       |
|     LUAX_TYPE_FUNCTIONS_BEGIN(cls)  |                                       |
|     LUAX_TYPE_FUNCTIONS_END         |                                       |
+-------------------------------------+---------------------------------------+

See example in ``tests\LuaxPointWtihMacroExample.cpp``.


Inheritance
^^^^^^^^^^^

``luax`` supports single inheritance.

If you want to inherit base class attributes then define superclass name
and register types starting from base class.

.. code-block:: c++

    class Point {};
    class PointEx: public Point {};

    LUAX_TYPE_NAME(Point, "Point")
    ...

    LUAX_TYPE_NAME(PointEx, "PointEx")
    LUAX_TYPE_SUPER_NAME(PointEx, "Point")
    ...

    luax::init(L);
    luax::type<Point>::register_in(L);
    luax::type<PointEx>::register_in(L);

After that if no attribute (property or method) is found in PointEx then it
will be searched in Point.

User hooks
^^^^^^^^^^

``luax`` provides user hooks which gets called on type registration and
at runtime.

If you want to customize lua type somehow then you may use registration hooks:

* ``usr_instance_mt()``
* ``usr_type_mt()``

Here are simplified registration steps when you call
``luax::type<>::register_in()``:

* Create instance metatable.
* Hook metamethods on the metatable (``__index``, ``__newindex`` etc).
* **Call** ``usr_instance_mt()``. The metatable will be on top of the stack.
* Register methods and properties.
* Create type metatable.
* Hook metamethods on the metatable.
* **Call** ``usr_type_mt()``. Type's metatable will be on top of the stack.
* Register type functions and enums.

--------------------------------------------------------------------------------

Runtime hooks:

* ``usr_gc()``
* ``usr_getter()``
* ``usr_setter()``

If you want to perform custom finalization on GC then implement ``usr_gc()``.

Return ``false`` to allow instance deleting by ``luax``, if you
perform custom delete operations then return ``true``.

Default implementation returns ``false`` to auto delete the object:

.. code-block:: c++

    bool type<T>::usr_gc(lua_State*, T*) { return false; }


``usr_getter()`` and ``usr_setter()`` are used as fallback actions if no
attribute is found.

``__index`` lookup algorithm:

1. Search getter.
2. Search method in the instance.
3. Search getter in the superclass.
4. Search method in the superclass.
5. Repeat 3, 4 for all superclasses.
6. **Call** ``usr_getter()``.

``__newindex`` lookup algorithm:

1. Search setter.
2. Search attribute in the instance.
3. Search setter in the superclass.
4. Search attribute in the superclass.
5. Repeat 3, 4 for all superclasses.
6. **Call** ``usr_setter()``.

By default ``usr_getter()`` and ``usr_setter()`` returns ``nil``.
