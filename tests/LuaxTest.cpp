#include "common.h"
#include "luax.h"

class LuaxTest: public BaseLuaxTest {};

static int counter = 0;

// Class to register.
struct Point
{
    int x;
    int y;

    Point(int x = 0, int y = 0): x(x), y(y) { ++counter; }
    virtual ~Point() { --counter; }

    int getX(lua_State *L)
    {
        assert(this);
        lua_pushinteger(L, x);
        return 1;
    }

    int setX(lua_State *L)
    {
        assert(this);
        x = static_cast<int>(luaL_checkinteger(L, 1));
        return 0;
    }
};
//------------------------------------------------------------------------------

// Free function getter/setter for the Point.
static int pt_x(lua_State *L)
{
    Point *pt = luax::type<Point>::get(L, 1);
    assert(pt);
    int top = lua_gettop(L);

    if (top == 1)
    {
        lua_pushinteger(L, pt->x);
        return 1;
    }
    else
    {
        pt->x = static_cast<int>(luaL_checkinteger(L, 2));
        return 0;
    }
}
//------------------------------------------------------------------------------

// Free function getter/setter for the Point.
static int pt_y(lua_State *L)
{
    Point *pt = luax::type<Point>::get(L, 1);
    assert(pt);
    int top = lua_gettop(L);

    if (top == 1)
    {
        lua_pushinteger(L, pt->y);
        return 1;
    }
    else
    {
        pt->y = static_cast<int>(luaL_checkinteger(L, 2));
        return 0;
    }
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

// Flags to test user hooks.
static bool usr_instance_mt_called = false;
static bool usr_type_mt_called = false;
static bool usr_index_called = false;
static bool usr_newindex_called = false;
static bool usr_gc_called = false;


namespace luax {
template <> void type<Point>::usr_instance_mt(lua_State*) { usr_instance_mt_called = true; }
template <> void type<Point>::usr_type_mt(lua_State*) { usr_type_mt_called = true;}
template <> int type<Point>::usr_getter(lua_State*) { usr_index_called = true; return 0; }
template <> int type<Point>::usr_setter(lua_State*) { usr_newindex_called = true; return 0; }
template <> bool type<Point>::usr_gc(lua_State*, Point*) { usr_gc_called = true; return false; }
}

LUAX_TYPE_NAME(Point, "Point")
// Instad of macro you can use:
//
//  namespace luax {
//      template <> const char* type<Point>::usr_name() { return "Point"; }
//  }

TEST_F(LuaxTest, simple)
{
    usr_instance_mt_called = false;
    usr_type_mt_called = false;

    luax::init(L);
    luax::type<Point>::register_in(L);

    // usr_instance_mt() and usr_type_mt() gets called while registration.
    EXPECT_TRUE(usr_instance_mt_called);
    EXPECT_TRUE(usr_type_mt_called);

    EXPECT_SCRIPT("assert(Point)");

    luax::type<Point>::push(L, 0);
    ASSERT_EQ(1, lua_gettop(L));
    ASSERT_TRUE(lua_isnil(L, -1));
}
//------------------------------------------------------------------------------

// Test: push with & without GC option.
TEST_F(LuaxTest, gc)
{
    usr_gc_called = false;

    luax::init(L);
    luax::type<Point>::register_in(L);

    // Without GC.

    Point pt;
    luax::type<Point>::push(L, &pt, false);
    lua_setglobal(L, "p");

    counter = 12;

    EXPECT_SCRIPT("p = nil");
    lua_gc(L, LUA_GCCOLLECT, 0);
    EXPECT_EQ(12, counter); // Point::~Point() decrements counter.

    // If GC is off then usr_gc() will not be called.
    EXPECT_FALSE(usr_gc_called);

    // With GC.

    luax::type<Point>::push(L, new Point);
    lua_setglobal(L, "p");

    counter = 100;

    EXPECT_SCRIPT("p = nil");
    lua_gc(L, LUA_GCCOLLECT, 0);
    EXPECT_EQ(99, counter); // Point::~Point() decrements counter.

    // usr_gc() hook is called on GC.
    EXPECT_TRUE(usr_gc_called);
}
//------------------------------------------------------------------------------

// Test: methods.
LUAX_FUNCTIONS_BEGIN(Point)
    LUAX_FUNCTION("getx", pt_x)
    LUAX_FUNCTION("gety", pt_y)
    LUAX_FUNCTION("sety", pt_y)
LUAX_FUNCTIONS_END

LUAX_FUNCTIONS_M_BEGIN(Point)
    LUAX_FUNCTION("getX", &Point::getX)
    LUAX_FUNCTION("setX", &Point::setX)
LUAX_FUNCTIONS_END

// Or direct specs:
//
//namespace luax {
//template <> luaL_Reg type<Point>::functions[] = {
//    {"getx", pt_x},
//    {"gety", pt_y},
//    {"sety", pt_y},
//    {0, 0}
//};
//
//template <> Method<Point> type<Point>::methods[] =
//{
//    {"getX", &Point::getX},
//    {"setX", &Point::setX},
//    {0, 0}
//};
//}

TEST_F(LuaxTest, methods)
{
    luax::init(L);
    luax::type<Point>::register_in(L);

    Point pt(10, 20);
    luax::type<Point>::push(L, &pt, false);
    lua_setglobal(L, "p");

    EXPECT_SCRIPT("assert(p:getx() == 10)");
    EXPECT_SCRIPT("assert(p:gety() == 20)");
    EXPECT_SCRIPT("assert(p:getX() == 10)");
    EXPECT_SCRIPT("p:sety(33)");
    EXPECT_SCRIPT("p:setX(43)");
    EXPECT_SCRIPT("assert(p:getX() == 43)");
    EXPECT_SCRIPT("assert(p:gety() == 33)");
}
//------------------------------------------------------------------------------

// Test: constructor.
namespace luax {
template <> Point* type<Point>::usr_constructor(lua_State *L)
{
    int top = lua_gettop(L);

    if (top == 1)
        return new Point();
    else
    {
        int x = static_cast<int>(luaL_checkinteger(L, 2));
        int y = static_cast<int>(luaL_checkinteger(L, 3));
        return new Point(x, y);
    }
    return 0;
}
}

TEST_F(LuaxTest, construct)
{
    luax::init(L);
    luax::type<Point>::register_in(L);

    ASSERT_SCRIPT("p = Point()");
    EXPECT_SCRIPT("assert(p:getx() == 0)");
    EXPECT_SCRIPT("assert(p:gety() == 0)");

    ASSERT_SCRIPT("p1 = Point(1, 2)");
    EXPECT_SCRIPT("assert(p1:getx() == 1)");
    EXPECT_SCRIPT("assert(p1:gety() == 2)");
}
//------------------------------------------------------------------------------

// Test: inheritance.
struct PointExt: public Point
{
    int z;

    PointExt(int x = 0, int y = 0): Point(x, y) {}

    int getsetZ(lua_State *L)
    {
        int top = lua_gettop(L);

        if (!top)
        {
            lua_pushinteger(L, z);
            return 1;
        }
        else
        {
            z = static_cast<int>(luaL_checkinteger(L, 1));
            return 0;
        }
    }
};

LUAX_TYPE_NAME(PointExt, "PointExt")
LUAX_TYPE_SUPER_NAME(PointExt, "Point")
LUAX_FUNCTIONS_M_BEGIN(PointExt)
    LUAX_FUNCTION("getZ", &PointExt::getsetZ)
    LUAX_FUNCTION("setZ", &PointExt::getsetZ)
LUAX_FUNCTIONS_END

// Or:
//
//namespace luax {
//template<> const char* type<PointExt>::usr_name() { return "PointExt"; }
//template<> const char* type<PointExt>::usr_super_name() { return "Point"; }
//template <> Method<PointExt> type<PointExt>::methods[] =
//{
//    {"getZ", &PointExt::getsetZ},
//    {"setZ", &PointExt::getsetZ},
//    {0, 0}
//};
//}

TEST_F(LuaxTest, inherit)
{
    luax::init(L);
    luax::type<Point>::register_in(L);
    luax::type<PointExt>::register_in(L);

    PointExt pt(10, 20);
    pt.z = 30;
    luax::type<PointExt>::push(L, &pt, false);
    lua_setglobal(L, "p");

    EXPECT_SCRIPT("assert(p:getx() == 10)");
    EXPECT_SCRIPT("assert(p:gety() == 20)");
    EXPECT_SCRIPT("assert(p:getZ() == 30)");
    EXPECT_SCRIPT("p:setZ(11)");
    EXPECT_SCRIPT("assert(p:getZ() == 11)");
}
//------------------------------------------------------------------------------

LUAX_PROPERTIES_BEGIN(Point)
    LUAX_PROPERTY("x", pt_x, pt_x)
    LUAX_PROPERTY("y", pt_y, pt_y)
LUAX_PROPERTIES_END

LUAX_PROPERTIES_M_BEGIN(Point)
    LUAX_PROPERTY("X", &Point::getX, &Point::setX)
LUAX_PROPERTIES_END

// Or:
//namespace luax {
//template <> FuncProperty type<Point>::func_properties[] = {
//    {"x", pt_x, pt_x},
//    {"y", pt_y, pt_y},
//    {0, 0, 0}
//};
//template <> MethodProperty<Point> type<Point>::method_properties[] = {
//    {"X", &Point::getX, &Point::setX},
//    {0, 0, 0}
//};
//}

// Test: property
TEST_F(LuaxTest, prop)
{
    usr_index_called = false;
    usr_newindex_called = false;

    luax::init(L);
    luax::type<Point>::register_in(L);

    Point pt(10, 20);
    luax::type<Point>::push(L, &pt, false);
    lua_setglobal(L, "p");

    EXPECT_SCRIPT("assert(p.x == 10)");
    // usr_index() hook gets called only if property is not found for the
    // instance. NOTE: the hook may be called in situations when current
    // metatable has no key and parent metatable has the key.
    EXPECT_FALSE(usr_index_called);
    EXPECT_SCRIPT("assert(p.fake == nil)");
    EXPECT_TRUE(usr_index_called);

    EXPECT_SCRIPT("assert(p:getx() == 10)");
    EXPECT_SCRIPT("assert(p.y == 20)");
    EXPECT_SCRIPT("assert(p.X == 10)");

    EXPECT_SCRIPT("p.x = 11");
    // See comments about usr_index().
    EXPECT_FALSE(usr_newindex_called);
    EXPECT_SCRIPT("p.fake = 1");
    EXPECT_TRUE(usr_newindex_called);

    EXPECT_SCRIPT("p.y = 12");
    EXPECT_SCRIPT("assert(p.x == 11)");
    EXPECT_SCRIPT("assert(p.y == 12)");
    EXPECT_SCRIPT("p.X = 33");
    EXPECT_SCRIPT("assert(p.X == 33)");
}
//------------------------------------------------------------------------------

LUAX_PROPERTIES_M_BEGIN(PointExt)
    LUAX_PROPERTY("Z", &PointExt::getsetZ, &PointExt::getsetZ)
LUAX_PROPERTIES_END

// Or:
//namespace luax {
//template <> MethodProperty<PointExt> type<PointExt>::method_properties[] = {
//    {"Z", &PointExt::getsetZ, &PointExt::getsetZ},
//    {0, 0, 0}
//};
//}

// Test: properties inheritance.
TEST_F(LuaxTest, propInherit)
{
    luax::init(L);
    luax::type<Point>::register_in(L);
    luax::type<PointExt>::register_in(L);

    PointExt pt(10, 20);
    luax::type<PointExt>::push(L, &pt, false);
    lua_setglobal(L, "p");

    EXPECT_SCRIPT("assert(p.x == 10)");
    EXPECT_SCRIPT("assert(p.y == 20)");
    EXPECT_SCRIPT("assert(p.X == 10)");
    EXPECT_SCRIPT("p.x = 11");
    EXPECT_SCRIPT("p.y = 12");
    EXPECT_SCRIPT("assert(p.x == 11)");
    EXPECT_SCRIPT("assert(p.y == 12)");
    EXPECT_SCRIPT("p.X = 33");
    EXPECT_SCRIPT("assert(p.X == 33)");
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------


struct PointExt2: public PointExt
{
    PointExt2(int x = 0, int y = 0): PointExt(x, y) {}

    int set_foo_x(lua_State *L)
    {
        x = static_cast<int>(luaL_checkinteger(L, 1));
        return 0;
    }
};

LUAX_TYPE_NAME(PointExt2, "PointExt2")
LUAX_TYPE_SUPER_NAME(PointExt2, "PointExt")
LUAX_FUNCTIONS_M_BEGIN(PointExt2)
    LUAX_FUNCTION("set_foo_x", &PointExt2::set_foo_x)
LUAX_FUNCTIONS_END

static bool getter_called = false;
static bool setter_called = false;

namespace luax {
// For non existent propes we return fixed value.
template <> int type<PointExt2>::usr_getter(lua_State *L)
{
    lua_pushinteger(L, 42);
    getter_called = true;
    return 1;
}
template <> int type<PointExt2>::usr_setter(lua_State *L)
{
    setter_called = true;
    return 0;
}
} // namespace luax

// Test: properties and function access if usr_getter() is defined.
TEST_F(LuaxTest, propWithUserIndex)
{
    getter_called = false;
    setter_called = false;

    luax::init(L);
    luax::type<Point>::register_in(L);
    luax::type<PointExt>::register_in(L);
    luax::type<PointExt2>::register_in(L);

    PointExt2 pt(10, 20);
    pt.z = 30;

    luax::type<PointExt2>::push(L, &pt, false);
    lua_setglobal(L, "p");

    // Since parent has X then usr_getter() will not be called.
    EXPECT_SCRIPT("assert(p.x == 10)");
    EXPECT_FALSE(getter_called);
    EXPECT_SCRIPT("assert(p.y == 20)");
    EXPECT_FALSE(getter_called);
    EXPECT_SCRIPT("assert(p.Z == 30)");
    EXPECT_FALSE(getter_called);

    // And here we call user hook, since fakeprop is not registered
    // in any parent.
    EXPECT_SCRIPT("assert(p.fakeprop == 42)");
    EXPECT_TRUE(getter_called);

    // Now test setter.
    EXPECT_SCRIPT("p.x = 11");
    EXPECT_FALSE(setter_called);
    EXPECT_SCRIPT("p.Z = 12");
    EXPECT_FALSE(setter_called);

    EXPECT_SCRIPT("p.fakeprop = 12");
    EXPECT_TRUE(setter_called);

    // Now test how function call works.
    getter_called = false;
    setter_called = false;

    EXPECT_SCRIPT("assert(p:getx() == 11)");
    EXPECT_FALSE(getter_called);

    EXPECT_SCRIPT("p:set_foo_x(32)");
    EXPECT_FALSE(getter_called);
    EXPECT_SCRIPT("assert(p:getx() == 32)");

}
//------------------------------------------------------------------------------

TEST_F(LuaxTest, propWithUserIndexBenchGet)
{
    getter_called = false;
    setter_called = false;

    luax::init(L);
    luax::type<Point>::register_in(L);
    luax::type<PointExt>::register_in(L);
    luax::type<PointExt2>::register_in(L);

    PointExt2 pt(10, 20);
    pt.z = 30;

    luax::type<PointExt2>::push(L, &pt, false);
    lua_setglobal(L, "p");

    EXPECT_SCRIPT("assert(p.x == 10)");
    EXPECT_SCRIPT("for i=1,1000000 do s=p.fakeprop end");
}
//------------------------------------------------------------------------------

TEST_F(LuaxTest, propWithUserIndexBenchSet)
{
    getter_called = false;
    setter_called = false;

    luax::init(L);
    luax::type<Point>::register_in(L);
    luax::type<PointExt>::register_in(L);
    luax::type<PointExt2>::register_in(L);

    PointExt2 pt(10, 20);
    pt.z = 30;

    luax::type<PointExt2>::push(L, &pt, false);
    lua_setglobal(L, "p");

    EXPECT_SCRIPT("assert(p.x == 10)");
    EXPECT_SCRIPT("for i=1,1000000 do p.x=i end");
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

LUAX_TYPE_ENUMS_BEGIN(Point)
    LUAX_ENUM("ENUM1", 10)
    LUAX_ENUM("ENUM2", 20)
LUAX_TYPE_ENUMS_END

// Or:
//namespace luax {
//template <> Enum type<Point>::type_enums[] = {
//    {"ENUM1", 10},
//    {"ENUM2", 20},
//    {0, 0}
//};
//}

// Test: type enums.
TEST_F(LuaxTest, typeEnums)
{
    luax::init(L);
    luax::type<Point>::register_in(L);
    EXPECT_SCRIPT("assert(Point.ENUM1 == 10)");
    EXPECT_SCRIPT("assert(Point.ENUM2 == 20)");
}
//------------------------------------------------------------------------------


static int func1(lua_State *L)
{
    lua_pushinteger(L, 10);
    return 1;
}

static int func2(lua_State *L)
{
    lua_pushinteger(L, 20);
    return 1;
}

LUAX_TYPE_FUNCTIONS_BEGIN(Point)
    LUAX_FUNCTION("func1", func1)
    LUAX_FUNCTION("func2", func2)
LUAX_TYPE_FUNCTIONS_END
// Or:
//namespace luax {
//template <> luaL_Reg type<Point>::type_functions[] = {
//    {"func1", func1},
//    {"func2", func2},
//    {0, 0}
//};
//}

// Test: type functions.
TEST_F(LuaxTest, typeFunc)
{
    luax::init(L);
    luax::type<Point>::register_in(L);
    EXPECT_SCRIPT("assert(Point.func1() == 10)");
    EXPECT_SCRIPT("assert(Point.func2() == 20)");
}
//------------------------------------------------------------------------------
