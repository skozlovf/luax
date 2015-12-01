//  This example show how to bind a class using luax helpe macros.

#include "common.h"
#include "luax.h"

class PointMacroWrapper
{
public:
    PointMacroWrapper(int x = 0, int y = 0): m_x(x), m_y(y) {}

    int getX(lua_State *L)
    {
        lua_pushinteger(L, m_x);
        return 1;
    }

    int setX(lua_State *L)
    {
        m_x = luaL_checkinteger(L, 1);
        return 0;
    }

    int m_x;
    int m_y;
};
//------------------------------------------------------------------------------

static int move(lua_State *L)
{
    PointMacroWrapper *p = luax::type<PointMacroWrapper>::check_get(L, 1);
    p->m_x = luaL_checkinteger(L, 2);
    p->m_y = luaL_checkinteger(L, 3);
    return 0;
}
//------------------------------------------------------------------------------

static int get_y(lua_State *L)
{
    PointMacroWrapper *p = luax::type<PointMacroWrapper>::check_get(L, 1);
    lua_pushinteger(L, p->m_y);
    return 1;
}
//------------------------------------------------------------------------------

static int set_y(lua_State *L)
{
    PointMacroWrapper *p = luax::type<PointMacroWrapper>::check_get(L, 1);
    p->m_y = luaL_checkinteger(L, 2);
    return 0;
}
//------------------------------------------------------------------------------

static int make_point(lua_State *L)
{
    PointMacroWrapper *p = new PointMacroWrapper;
    p->m_x = luaL_checkinteger(L, 1);
    p->m_y = luaL_checkinteger(L, 2);

    luax::type<PointMacroWrapper>::push(L, p);
    return 1;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------


// Wrapper

/** Define type name. */
LUAX_TYPE_NAME(PointMacroWrapper, "Point")


/** Define Point methods. */
LUAX_FUNCTIONS_M_BEGIN(PointMacroWrapper)
    LUAX_FUNCTION("getX", &PointMacroWrapper::getX)
    LUAX_FUNCTION("setX", &PointMacroWrapper::setX)
LUAX_FUNCTIONS_END


/** Define Point methods implemented by free functions. */
LUAX_FUNCTIONS_BEGIN(PointMacroWrapper)
    LUAX_FUNCTION("move", move)
LUAX_FUNCTIONS_END


/** Define Point properies. */
LUAX_PROPERTIES_M_BEGIN(PointMacroWrapper)
    LUAX_PROPERTY("x", &PointMacroWrapper::getX, &PointMacroWrapper::setX)
LUAX_PROPERTIES_END


/** Define Point properies implemented by free functions. */
LUAX_PROPERTIES_BEGIN(PointMacroWrapper)
    LUAX_PROPERTY("y", get_y, set_y)
LUAX_PROPERTIES_END


/** Define Point enums. */
LUAX_TYPE_ENUMS_BEGIN(PointMacroWrapper)
    LUAX_ENUM("FOO", 10)
    LUAX_ENUM("BAR", 20)
LUAX_TYPE_ENUMS_END


/** Define Point functions. */
LUAX_TYPE_FUNCTIONS_BEGIN(PointMacroWrapper)
    LUAX_FUNCTION("make", make_point)
LUAX_TYPE_FUNCTIONS_END


/** Define simple constructor Point(x, y). */
namespace luax {
template <> PointMacroWrapper* type<PointMacroWrapper>::usr_constructor(lua_State *L)
{
    int x = luaL_checkinteger(L, 2);
    int y = luaL_checkinteger(L, 3);
    return new PointMacroWrapper(x, y);
}
} // namespace luax
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------


class LuaxPointMacroExample: public BaseLuaxTest {};
TEST_F(LuaxPointMacroExample, example)
{
    luax::init(L);
    luax::type<PointMacroWrapper>::register_in(L);

    EXPECT_SCRIPT("p = Point(1, 100)");
    EXPECT_SCRIPT("assert(1, p.x)");
    EXPECT_SCRIPT("assert(100, p.y)");

    EXPECT_SCRIPT("p:move(30, 40)");
    EXPECT_SCRIPT("assert(30, p.x)");
    EXPECT_SCRIPT("assert(40, p.y)");

    EXPECT_SCRIPT("p.x, p.y = p.y, p.x");
    EXPECT_SCRIPT("assert(40, p.x)");
    EXPECT_SCRIPT("assert(30, p.y)");

    EXPECT_SCRIPT("p.x = Point.FOO + 10");
    EXPECT_SCRIPT("assert(Point.BAR, p.x)");

    EXPECT_SCRIPT("p2 = Point.make(20, -40)");
    EXPECT_SCRIPT("assert(p.x, p2.x)");
    EXPECT_SCRIPT("assert(-40, p2.y)");
}
//------------------------------------------------------------------------------

