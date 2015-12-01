//  This example show how to bind a class using luax::type directly.

#include "common.h"
#include "luax.h"

class PointWrapper
{
public:
    PointWrapper(int x = 0, int y = 0): m_x(x), m_y(y) {}

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
    PointWrapper *p = luax::type<PointWrapper>::check_get(L, 1);
    p->m_x = luaL_checkinteger(L, 2);
    p->m_y = luaL_checkinteger(L, 3);
    return 0;
}
//------------------------------------------------------------------------------

static int get_y(lua_State *L)
{
    PointWrapper *p = luax::type<PointWrapper>::check_get(L, 1);
    lua_pushinteger(L, p->m_y);
    return 1;
}
//------------------------------------------------------------------------------

static int set_y(lua_State *L)
{
    PointWrapper *p = luax::type<PointWrapper>::check_get(L, 1);
    p->m_y = luaL_checkinteger(L, 2);
    return 0;
}
//------------------------------------------------------------------------------

static int make_point(lua_State *L)
{
    PointWrapper *p = new PointWrapper;
    p->m_x = luaL_checkinteger(L, 1);
    p->m_y = luaL_checkinteger(L, 2);

    luax::type<PointWrapper>::push(L, p);
    return 1;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------


// Wrapper
namespace luax {

/** Define type name. */
template <> const char* type<PointWrapper>::usr_name() { return "Point"; }

/** Define simple constructor Point(x, y). */
template <> PointWrapper* type<PointWrapper>::usr_constructor(lua_State *L)
{
    int x = luaL_checkinteger(L, 2);
    int y = luaL_checkinteger(L, 3);
    return new PointWrapper(x, y);
}

/** Define Point methods. */
template <> Method<PointWrapper> type<PointWrapper>::methods[] =
{
    {"getX", &PointWrapper::getX},
    {"setX", &PointWrapper::setX},
    {0, 0}
};

/** Define Point methods implemented by free functions. */
template <> luaL_Reg type<PointWrapper>::functions[] = {
    {"move", move},
    {0, 0}
};

/** Define Point properies. */
template <> MethodProperty<PointWrapper> type<PointWrapper>::method_properties[] = {
    {"x", &PointWrapper::getX, &PointWrapper::setX},
    {0, 0, 0}
};

/** Define Point properies implemented by free functions. */
template <> FuncProperty type<PointWrapper>::func_properties[] = {
    {"y", get_y, set_y},
    {0, 0, 0}
};

/** Define Point enums. */
template <> Enum type<PointWrapper>::type_enums[] = {
    {"FOO", 10},
    {"BAR", 20},
    {0, 0}
};

/** Define Point functions. */
template <> luaL_Reg type<PointWrapper>::type_functions[] = {
    {"make", make_point},
    {0, 0}
};

} // namespace luax
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------


class LuaxPointExample: public BaseLuaxTest {};
TEST_F(LuaxPointExample, example)
{
    luax::init(L);
    luax::type<PointWrapper>::register_in(L);

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
