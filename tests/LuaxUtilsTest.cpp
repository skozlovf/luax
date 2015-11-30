#include "common.h"
#include "luax.h"
#include "luax_utils.h"

class LuaxUtilsTest: public BaseLuaxTest {};

static int func(lua_State *) { return 0; }

// Test: push() function.
TEST_F(LuaxUtilsTest, push)
{
    void *addr = static_cast<void*>(L);
    luax::push(L, addr);
    EXPECT_EQ(addr, lua_touserdata(L, -1));
    lua_pop(L, 1);

    luax::push(L, (double)1.3);
    EXPECT_DOUBLE_EQ(1.3, (double)lua_tonumber(L,  -1));
    lua_pop(L, 1);

    luax::push(L, 1.3f);
    EXPECT_FLOAT_EQ(1.3f, (float)lua_tonumber(L,  -1));
    lua_pop(L, 1);

    luax::push(L, (signed char)-34);
    EXPECT_EQ(-34, (signed char)lua_tointeger(L,  -1));
    lua_pop(L, 1);

    luax::push(L, (unsigned char)34);
    EXPECT_EQ(34, (unsigned char)lua_tointeger(L,  -1));
    lua_pop(L, 1);

    luax::push(L, (int)-34);
    EXPECT_EQ(-34, (int)lua_tointeger(L,  -1));
    lua_pop(L, 1);

    luax::push(L, (long)34);
    EXPECT_EQ(34, (long)lua_tointeger(L,  -1));
    lua_pop(L, 1);

    luax::push(L, (unsigned long)34);
    EXPECT_EQ(34, (unsigned long)lua_tointeger(L,  -1));
    lua_pop(L, 1);

    const char *x = "hello";
    luax::push(L, x);
    EXPECT_STREQ(x, lua_tostring(L,  -1));
    lua_pop(L, 1);

    luax::push(L, (char*)x);
    EXPECT_STREQ(x, lua_tostring(L,  -1));
    lua_pop(L, 1);

    char c = 'c';
    luax::push(L, c);
    EXPECT_STREQ("c", lua_tostring(L,  -1));
    lua_pop(L, 1);

    luax::push(L, std::string("works"));
    EXPECT_STREQ("works", lua_tostring(L,  -1));
    lua_pop(L, 1);

    luax::push(L, func);
    EXPECT_EQ(&func, lua_tocfunction(L,  -1));
    lua_pop(L, 1);
}
//------------------------------------------------------------------------------

// Test: get() function.
TEST_F(LuaxUtilsTest, get)
{
    void *addr = static_cast<void*>(L);
    luax::push(L, addr);
    EXPECT_EQ(addr, luax::get<void*>(L, -1));
    lua_pop(L, 1);

    luax::push(L, 1.3);
    EXPECT_DOUBLE_EQ(1.3, luax::get<double>(L,  -1));
    lua_pop(L, 1);

    luax::push(L, 1.3f);
    EXPECT_FLOAT_EQ(1.3f, luax::get<float>(L,  -1));
    lua_pop(L, 1);

    luax::push(L, -34);
    EXPECT_EQ(-34, luax::get<signed char>(L,  -1));
    lua_pop(L, 1);

    luax::push(L, 34);
    EXPECT_EQ(34, luax::get<unsigned char>(L,  -1));
    lua_pop(L, 1);

    luax::push(L, -34);
    EXPECT_EQ(-34, luax::get<int>(L,  -1));
    lua_pop(L, 1);

    luax::push(L, 34);
    EXPECT_EQ(34, luax::get<long>(L,  -1));
    lua_pop(L, 1);

    luax::push(L, 34);
    EXPECT_EQ(34, luax::get<unsigned long>(L,  -1));
    lua_pop(L, 1);

    luax::push(L, "hello");
    EXPECT_STREQ("hello", luax::get<const char*>(L,  -1));
    lua_pop(L, 1);

    luax::push(L, "hello");
    EXPECT_EQ(std::string("hello"), luax::get<std::string>(L,  -1));
    lua_pop(L, 1);

    luax::push(L, func);
    EXPECT_EQ(&func, luax::get<lua_CFunction>(L,  -1));
    lua_pop(L, 1);
}
//------------------------------------------------------------------------------

// Test: get_global().
TEST_F(LuaxUtilsTest, get_global)
{
    luax::push(L, "hello");
    lua_setglobal(L, "field");

    luax::push(L, 21.3);
    lua_setglobal(L, "num");

    EXPECT_STREQ("hello", luax::get_global<const char*>(L, "field"));
    EXPECT_EQ(0, lua_gettop(L));

    EXPECT_EQ(std::string("hello"), luax::get_global<std::string>(L, "field"));
    EXPECT_EQ(0, lua_gettop(L));

    EXPECT_EQ(0, luax::get_global<int>(L, "field"));
    EXPECT_EQ(21, luax::get_global<int>(L, "num"));
    EXPECT_EQ(21, luax::get_global<unsigned int>(L, "num"));
    EXPECT_EQ(21, luax::get_global<long>(L, "num"));
    EXPECT_EQ(21, luax::get_global<unsigned long>(L, "num"));
    EXPECT_DOUBLE_EQ(21.3, luax::get_global<double>(L, "num"));
}
//------------------------------------------------------------------------------

// Test: set_global().
TEST_F(LuaxUtilsTest, set_global)
{
    luax::set_global(L, "val", 12);
    EXPECT_EQ(0, lua_gettop(L));
    EXPECT_EQ(12, luax::get_global<int>(L, "val"));

    luax::set_global(L, "val", 12.3);
    EXPECT_EQ(0, lua_gettop(L));
    EXPECT_DOUBLE_EQ(12.3, luax::get_global<double>(L, "val"));

    luax::set_global(L, "val", "hello");
    EXPECT_EQ(0, lua_gettop(L));
    EXPECT_STREQ("hello", luax::get_global<const char*>(L, "val"));
}
//------------------------------------------------------------------------------

// Test: get_field().
TEST_F(LuaxUtilsTest, get_field)
{
    lua_newtable(L);

    lua_pushnumber(L, 11);
    lua_setfield(L, -2, "num");
    EXPECT_EQ(11, luax::get_field<int>(L, -1, "num"));
    EXPECT_EQ(1, lua_gettop(L));

    lua_pushstring(L, "hello");
    lua_setfield(L, -2, "num");
    EXPECT_EQ(std::string("hello"), luax::get_field<std::string>(L, -1, "num"));
    EXPECT_EQ(1, lua_gettop(L));
}
//------------------------------------------------------------------------------

// Test: set_field().
TEST_F(LuaxUtilsTest, set_field)
{
    lua_newtable(L);

    luax::set_field(L, -1, "num", 232);
    EXPECT_EQ(1, lua_gettop(L));
    EXPECT_EQ(232, luax::get_field<int>(L, -1, "num"));

    // test if index used correctly.
    luax::set_field(L, 1, "num", 232);
    EXPECT_EQ(1, lua_gettop(L));
    EXPECT_EQ(232, luax::get_field<int>(L, -1, "num"));

    luax::set_field(L, -1, "num", "hello");
    EXPECT_EQ(1, lua_gettop(L));
    EXPECT_STREQ("hello", luax::get_field<const char*>(L, -1, "num"));
}
//------------------------------------------------------------------------------

// Test: rawget_field().
TEST_F(LuaxUtilsTest, rawget_field)
{
    EXPECT_SCRIPT(
                "tbl = {}\n"
                "mt = {\n"
                "__index = function(o,k) return 42 end,\n"
                "__newindex = function(o,k,v) fake = v end}\n"
                "setmetatable(tbl, mt)"
            );

    lua_getglobal(L, "tbl");

    EXPECT_EQ(42, luax::get_field<int>(L, -1, "num"));
    EXPECT_EQ(1, lua_gettop(L));
    EXPECT_EQ(0, luax::rawget_field<int>(L, -1, "num"));
    EXPECT_EQ(1, lua_gettop(L));
    EXPECT_EQ(0, luax::rawget_field<int>(L, 1, "num"));
    EXPECT_EQ(1, lua_gettop(L));
}
//------------------------------------------------------------------------------

// Test: rawset_field().
TEST_F(LuaxUtilsTest, rawset_field)
{
    EXPECT_SCRIPT(
                "tbl = {}\n"
                "mt = {\n"
                "__index = function(o,k) return 42 end,\n"
                "__newindex = function(o,k,v) fake = v end}\n"
                "setmetatable(tbl, mt)"
            );

    lua_getglobal(L, "tbl");

    luax::rawset_field(L, -1, "num", 23);
    EXPECT_EQ(1, lua_gettop(L));

    EXPECT_EQ(23, luax::get_field<int>(L, -1, "num"));
    EXPECT_EQ(23, luax::rawget_field<int>(L, -1, "num"));
}
//------------------------------------------------------------------------------


int f_void(lua_State *L)
{
    luax::push(L, luax::checkget<void*>(L, 1));
    return 1;
}
//------------------------------------------------------------------------------

int f_double(lua_State *L)
{
    luax::push(L, luax::checkget<double>(L, 1));
    return 1;
}
//------------------------------------------------------------------------------

int f_float(lua_State *L)
{
    luax::push(L, luax::checkget<float>(L, 1));
    return 1;
}
//------------------------------------------------------------------------------

int f_signed_char(lua_State *L)
{
    luax::push(L, luax::checkget<signed char>(L, 1));
    return 1;
}
//------------------------------------------------------------------------------

int f_unsigned_char(lua_State *L)
{
    luax::push(L, luax::checkget<unsigned char>(L, 1));
    return 1;
}
//------------------------------------------------------------------------------

int f_int(lua_State *L)
{
    luax::push(L, luax::checkget<int>(L, 1));
    return 1;
}
//------------------------------------------------------------------------------

int f_unsigned_int(lua_State *L)
{
    luax::push(L, luax::checkget<unsigned int>(L, 1));
    return 1;
}
//------------------------------------------------------------------------------

int f_long(lua_State *L)
{
    luax::push(L, luax::checkget<long>(L, 1));
    return 1;
}
//------------------------------------------------------------------------------

int f_unsigned_long(lua_State *L)
{
    luax::push(L, luax::checkget<unsigned long>(L, 1));
    return 1;
}
//------------------------------------------------------------------------------

int f_const_char(lua_State *L)
{
    luax::push(L, luax::checkget<const char*>(L, 1));
    return 1;
}
//------------------------------------------------------------------------------

int f_string(lua_State *L)
{
    luax::push(L, luax::checkget<std::string>(L, 1));
    return 1;
}
//------------------------------------------------------------------------------

int f_bool(lua_State *L)
{
    luax::push(L, luax::checkget<bool>(L, 1));
    return 1;
}
//------------------------------------------------------------------------------

#define CALL_FUNC(name, good, bad)       \
    lua_getglobal(L, name);              \
    luax::push(L, good);                 \
    EXPECT_EQ(0, lua_pcall(L, 1, 1, 0)) << lua_tostring(L, -1); \
    lua_pop(L, 1);                       \
    \
    lua_getglobal(L, name);              \
    luax::push(L, bad);                 \
    EXPECT_NE(0, lua_pcall(L, 1, 1, 0)); \
    lua_pop(L, 1);

// Test: checkget().
TEST_F(LuaxUtilsTest, checkget)
{
    luax::set_global(L, "f_void", f_void);
    luax::set_global(L, "f_double", f_double);
    luax::set_global(L, "f_float", f_float);
    luax::set_global(L, "f_signed_char", f_signed_char);
    luax::set_global(L, "f_unsigned_char", f_unsigned_char);
    luax::set_global(L, "f_int", f_int);
    luax::set_global(L, "f_unsigned_int", f_unsigned_int);
    luax::set_global(L, "f_long", f_long);
    luax::set_global(L, "f_unsigned_long", f_unsigned_long);
    luax::set_global(L, "f_const_char", f_const_char);
    luax::set_global(L, "f_string", f_string);
    luax::set_global(L, "f_bool", f_bool);

    CALL_FUNC("f_void", (void*)L, 12);
    CALL_FUNC("f_double", 12, (void*)L);
    CALL_FUNC("f_float", 12, (void*)L);
    CALL_FUNC("f_signed_char", 12, (void*)L);
    CALL_FUNC("f_unsigned_char", 12, (void*)L);
    CALL_FUNC("f_int", 12, (void*)L);
    CALL_FUNC("f_unsigned_int", 12, (void*)L);
    CALL_FUNC("f_long", 12, (void*)L);
    CALL_FUNC("f_unsigned_long", 12, (void*)L);
    CALL_FUNC("f_const_char", "hello", (void*)L);
    CALL_FUNC("f_string", "hello", (void*)L);
    CALL_FUNC("f_bool", true, (void*)L);
}
//------------------------------------------------------------------------------
