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
template <> const char* type<Point>::usr_name() { return "Point"; }
template <> void type<Point>::usr_instance_mt(lua_State*) { usr_instance_mt_called = true; }
template <> void type<Point>::usr_type_mt(lua_State*) { usr_type_mt_called = true;}
template <> bool type<Point>::usr_index(lua_State*) { usr_index_called = true; return false; }
template <> bool type<Point>::usr_newindex(lua_State*) { usr_newindex_called = true; return false; }
template <> bool type<Point>::usr_gc(lua_State*, Point*) { usr_gc_called = true; return false; }
}

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
namespace luax {
template <> luaL_Reg type<Point>::functions[] = {
    {"getx", pt_x},
    {"gety", pt_y},
    {"sety", pt_y},
    {0, 0}
};

template <> Method<Point> type<Point>::methods[] =
{
    {"getX", &Point::getX},
    {"setX", &Point::setX},
    {0, 0}
};
}

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

namespace luax {
template<> const char* type<PointExt>::usr_name() { return "PointExt"; }
template<> const char* type<PointExt>::usr_super_name() { return "Point"; }
template <> Method<PointExt> type<PointExt>::methods[] =
{
    {"getZ", &PointExt::getsetZ},
    {"setZ", &PointExt::getsetZ},
    {0, 0}
};
}

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

// Test: property
namespace luax {
template <> FuncProperty type<Point>::func_properties[] = {
    {"x", pt_x, pt_x},
    {"y", pt_y, pt_y},
    {0, 0, 0}
};
template <> MethodProperty<Point> type<Point>::method_properties[] = {
    {"X", &Point::getX, &Point::setX},
    {0, 0, 0}
};
}

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

// Test: properties inheritance.
namespace luax {
template <> MethodProperty<PointExt> type<PointExt>::method_properties[] = {
    {"Z", &PointExt::getsetZ, &PointExt::getsetZ},
    {0, 0, 0}
};
}

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

// Test: type enums.
namespace luax {
template <> Enum type<Point>::type_enums[] = {
    {"ENUM1", 10},
    {"ENUM2", 20},
    {0, 0}
};
}

TEST_F(LuaxTest, typeEnums)
{
    luax::init(L);
    luax::type<Point>::register_in(L);
    EXPECT_SCRIPT("assert(Point.ENUM1 == 10)");
    EXPECT_SCRIPT("assert(Point.ENUM2 == 20)");
}
//------------------------------------------------------------------------------

// Test: type functions.

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


namespace luax {
template <> luaL_Reg type<Point>::type_functions[] = {
    {"func1", func1},
    {"func2", func2},
    {0, 0}
};
}

TEST_F(LuaxTest, typeFunc)
{
    luax::init(L);
    luax::type<Point>::register_in(L);
    EXPECT_SCRIPT("assert(Point.func1() == 10)");
    EXPECT_SCRIPT("assert(Point.func2() == 20)");
}
//------------------------------------------------------------------------------
