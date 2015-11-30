// The MIT License
//
// Copyright (c) 2015 Sergey Kozlov
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#ifndef LUAX_UTILS_H
#define LUAX_UTILS_H

#include <string>
#include <sstream>

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#include "luax.h"

namespace luax {

namespace dump
{
static std::string value(lua_State *L, int index)
{
    int type = lua_type(L, index);
    std::ostringstream buf;

    switch (type)
    {
    case LUA_TSTRING:
        buf << lua_tostring(L, index);
        break;

    case LUA_TBOOLEAN:
        buf << (lua_toboolean(L, index) ? "true" : "false");
        break;

    case LUA_TNUMBER:
        buf << lua_tonumber(L, index);
        break;

    case LUA_TUSERDATA:
    case LUA_TLIGHTUSERDATA:
        {
            void *d = lua_touserdata(L, index);
            buf << (type == LUA_TUSERDATA ? "userdata": "lightuserdata")
                << " " << d;
        }
        break;
    case LUA_TFUNCTION:
        {
            buf << lua_typename(L, type);

            lua_pushvalue(L, -1);
            lua_Debug d;

            if (lua_getinfo(L, ">Snl", &d) != 0)
            {
                if(d.what)
                    buf << " " << d.what;
                if(d.namewhat)
                    buf << " " << d.namewhat;
                if(d.name)
                    buf << " " << d.name;
                buf << " " << d.short_src;
            }
        }

    default:
        buf << lua_typename(L, type);
    } // switch

    return buf.str();
}
//------------------------------------------------------------------------------

static std::string stack(lua_State *L)
{
    int top = lua_gettop(L);
    std::ostringstream out;

    for (int i = top; i >= 1; i--)
        out << "[" << -(top - i + 1) << "]" << "[" << (i) << "]" << ": "
            << value(L, i) << std::endl;

    return out.str();
}
//------------------------------------------------------------------------------

}// namespace dump


//------------------------------------------------------------------------------
// StackCleaner
//------------------------------------------------------------------------------

class StackCleaner
{
public:
    StackCleaner(lua_State *L): L(L), m_top(lua_gettop(L)) { }
    ~StackCleaner() { lua_settop(L, m_top); }

private:
    lua_State *L;
    int m_top;
};


//------------------------------------------------------------------------------
// luax::push()
//------------------------------------------------------------------------------

/**
 * Push value on stack.
 *
 * Usage examples:
 *
 *      luax::push(L, 12);
 *      luax::push(L, "str");
 */
template<typename T> inline void push(lua_State *L, T v);

template<> inline void push(lua_State *L, void *v)
{
    lua_pushlightuserdata(L, v);
}
//------------------------------------------------------------------------------

template<> inline void push(lua_State *L, double v)
{
    lua_pushnumber(L, static_cast<lua_Number>(v));
}
//------------------------------------------------------------------------------

template<> inline void push(lua_State *L, float v)
{
    lua_pushnumber(L, static_cast<lua_Number>(v));
}
//------------------------------------------------------------------------------

template<> inline void push(lua_State *L, signed char v)
{
    lua_pushinteger(L, static_cast<lua_Integer>(v));
}
//------------------------------------------------------------------------------

template<> inline void push(lua_State *L, unsigned char v)
{
    lua_pushinteger(L, static_cast<lua_Integer>(v));
}
//------------------------------------------------------------------------------

template<> inline void push(lua_State *L, int v)
{
    lua_pushinteger(L, static_cast<lua_Integer>(v));
}
//------------------------------------------------------------------------------

template<> inline void push(lua_State *L, unsigned int v)
{
    lua_pushinteger(L, static_cast<lua_Integer>(v));
}
//------------------------------------------------------------------------------

template<> inline void push(lua_State *L, long v)
{
    lua_pushinteger(L, static_cast<lua_Integer>(v));
}
//------------------------------------------------------------------------------

template<> inline void push(lua_State *L, unsigned long v)
{
    lua_pushinteger(L, static_cast<lua_Integer>(v));
}
//------------------------------------------------------------------------------

template<> inline void push(lua_State *L, const char *v)
{
    lua_pushstring(L, v);
}
//------------------------------------------------------------------------------

template<> inline void push(lua_State *L, char *v)
{
    lua_pushstring(L, v);
}
//------------------------------------------------------------------------------

template<> inline void push(lua_State * L, char v)
{
    lua_pushlstring(L, static_cast<const char *>(&v), 1);
}
//------------------------------------------------------------------------------

static inline void push(lua_State * L, const std::string &v)
{
    lua_pushstring(L, v.c_str());
}
//------------------------------------------------------------------------------

template<> inline void push(lua_State * L, bool v)
{
    lua_pushboolean(L, static_cast<lua_Integer>(v));
}
//------------------------------------------------------------------------------

template<> inline void push(lua_State * L, lua_CFunction v)
{
    lua_pushcfunction(L, v);
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// luax::get()
//------------------------------------------------------------------------------

template<typename T> inline T get(lua_State *L, int idx);

template<> inline void* get<void*>(lua_State *L, int idx)
{
    return lua_touserdata(L, idx);
}
//------------------------------------------------------------------------------

template<> inline double get(lua_State *L, int idx)
{
    return lua_tonumber(L, idx);
}
//------------------------------------------------------------------------------

template<> inline float get(lua_State *L, int idx)
{
    return static_cast<float>(lua_tonumber(L, idx));
}
//------------------------------------------------------------------------------

template<> inline int get(lua_State *L, int idx)
{
    return static_cast<int>(lua_tonumber(L, idx));
}
//------------------------------------------------------------------------------

template<> inline unsigned int get(lua_State *L, int idx)
{
    return static_cast<unsigned int>(lua_tonumber(L, idx));
}
//------------------------------------------------------------------------------

template<> inline long get(lua_State *L, int idx)
{
    return static_cast<long>(lua_tonumber(L, idx));
}
//------------------------------------------------------------------------------

template<> inline unsigned long get(lua_State *L, int idx)
{
    return static_cast<unsigned long>(lua_tonumber(L, idx));
}
//------------------------------------------------------------------------------

template<> inline const char* get<const char*>(lua_State *L, int idx)
{
    return lua_tostring(L, idx);
}
//------------------------------------------------------------------------------

template<> inline std::string get(lua_State *L, int idx)
{
    return lua_tostring(L, idx);
}
//------------------------------------------------------------------------------

template<> inline lua_CFunction get<lua_CFunction>(lua_State *L, int idx)
{
    return lua_tocfunction(L, idx);
}
//------------------------------------------------------------------------------

template<> inline bool get(lua_State *L, int idx)
{
    return lua_toboolean(L, idx) ? true : false;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// luax::checkget()
//------------------------------------------------------------------------------

template<typename T> inline T checkget(lua_State *L, int narg);

template<> inline void* checkget<void*>(lua_State *L, int narg)
{
    luaL_argcheck(L, lua_type(L, narg) == LUA_TLIGHTUSERDATA, narg,
                  "lightuserdata expected");
    return lua_touserdata(L, narg);
}
//------------------------------------------------------------------------------

template<> inline double checkget(lua_State *L, int narg)
{
    return luaL_checknumber(L, narg);
}
//------------------------------------------------------------------------------

template<> inline float checkget(lua_State *L, int narg)
{
    return static_cast<float>(luaL_checknumber(L, narg));
}
//------------------------------------------------------------------------------

template<> inline int checkget(lua_State *L, int narg)
{
    return static_cast<int>(luaL_checknumber(L, narg));
}
//------------------------------------------------------------------------------

template<> inline unsigned int checkget(lua_State *L, int narg)
{
    return static_cast<unsigned int>(luaL_checknumber(L, narg));
}
//------------------------------------------------------------------------------

template<> inline long checkget(lua_State *L, int narg)
{
    return static_cast<long>(luaL_checknumber(L, narg));
}
//------------------------------------------------------------------------------

template<> inline unsigned long checkget(lua_State *L, int narg)
{
    return static_cast<unsigned long>(luaL_checknumber(L, narg));
}
//------------------------------------------------------------------------------

template<> inline const char* checkget<const char*>(lua_State *L, int narg)
{
    return luaL_checkstring(L, narg);
}
//------------------------------------------------------------------------------

template<> inline std::string checkget(lua_State *L, int narg)
{
    return luaL_checkstring(L, narg);
}
//------------------------------------------------------------------------------

template<> inline lua_CFunction checkget<lua_CFunction>(lua_State *L, int narg)
{
    lua_CFunction func = lua_tocfunction(L, narg);
    luaL_argcheck(L, func != 0, narg, "function expected");
    return func;
}
//------------------------------------------------------------------------------

template<> inline bool checkget(lua_State *L, int narg)
{
    luaL_argcheck(L, lua_isboolean(L, narg), narg, "boolean expected");
    return lua_toboolean(L, narg) ? true : false;
}
//------------------------------------------------------------------------------

// TODO: add take() and checktake()

//------------------------------------------------------------------------------
// Other stuff.
//------------------------------------------------------------------------------

/**
 * Return value of the global name.
 * Value itself will be poped from the stack.
 */
template <typename T>
inline T get_global(lua_State *L, const char *name)
{
    StackCleaner cleaner(L);
    lua_getglobal(L, name);
    return get<T>(L, -1);
}
//------------------------------------------------------------------------------

/** Set v as the new value of global name. */
template <typename T>
inline void set_global(lua_State *L, const char *name, T v)
{
    push<T>(L, v);
    lua_setglobal(L, name);
}
//------------------------------------------------------------------------------


/**
 * Return value t[name], where t is the value at the given valid index.
 * This function may trigger a metamethod for the "index" event.
 */
template <typename T>
inline T get_field(lua_State *L, int index, const char *name)
{
    StackCleaner cleaner(L);
    lua_getfield(L, index, name);
    return get<T>(L, -1);
}
//------------------------------------------------------------------------------

/**
 * Equivalent to t[name] = v, where t is the value at the given valid index.
 * This function may trigger a metamethod for the "newindex" event.
 */
template <typename T>
inline void set_field(lua_State *L, int index, const char *name, T v)
{
    push<T>(L, v);
    lua_setfield(L, index < 0 ? index - 1: index, name);
}
//------------------------------------------------------------------------------


/**
 * Same as luax::get_field() but does raw access
 * (without triggering "index" event).
 */
template <typename T>
inline T rawget_field(lua_State *L, int index, const char *name)
{
    StackCleaner cleaner(L);
    lua_pushstring(L, name);
    lua_rawget(L, index < 0 ? index - 1: index);
    return get<T>(L, -1);
}
//------------------------------------------------------------------------------

/**
 * Same as luax::set_field() but does a raw assignment
 * (without triggering "newindex" event).
 */
template <typename T>
inline void rawset_field(lua_State *L, int index, const char *name, T v)
{
    lua_pushstring(L, name);
    push<T>(L, v);
    lua_rawset(L, index < 0 ? index - 2: index);
}
//------------------------------------------------------------------------------
} // namespace luax

#endif // LUAX_UTILS_H

