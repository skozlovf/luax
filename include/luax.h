// Inspired by
// https://github.com/igoumeninja/Gea/blob/master/bin/data/scripts/my-lua-scripts/LuaAV/include/lua_glue.h
// and similar wrappers.

#ifndef LUAX_H
#define LUAX_H

#include <cassert>

#ifdef __cplusplus
extern "C" {
#endif

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#ifdef __cplusplus
}
#endif


// Helper macros to define a type.
// TODO: add docs for the macros.

#define LUAX_TYPE_NAME(cls,name)                                        \
    namespace luax {                                                    \
        template <> const char* type<cls>::usr_name() { return name; }  \
    }

#define LUAX_TYPE_SUPER_NAME(cls,name)                                      \
    namespace luax {                                                        \
        template <> const char* type<cls>::usr_super_name() { return name; }\
    }


#define LUAX_FUNCTIONS_BEGIN(cls)           \
    namespace luax {                        \
        template <> luaL_Reg type<cls>::functions[] = {

#define LUAX_FUNCTIONS_M_BEGIN(cls)         \
    namespace luax {                        \
        template <> Method<cls> type<cls>::methods[] = {

#define LUAX_FUNCTIONS_END                  \
            {0, 0}                          \
        };                                  \
    } // namespace luax

#define LUAX_FUNCTION(name, f) {name, f},


#define LUAX_PROPERTIES_BEGIN(cls)          \
    namespace luax {                        \
        template <>                         \
        FuncProperty type<cls>::func_properties[] = {

#define LUAX_PROPERTIES_M_BEGIN(cls)        \
    namespace luax {                        \
        template <>                         \
        MethodProperty<cls> type<cls>::method_properties[] = {

#define LUAX_PROPERTIES_END                 \
            {0, 0, 0}                       \
        };                                  \
    } // namespace luax

#define LUAX_PROPERTY(name,getter,setter) {name, getter, setter},


#define LUAX_TYPE_ENUMS_BEGIN(cls)          \
    namespace luax {                        \
        template <> Enum type<cls>::type_enums[] = {

#define LUAX_TYPE_ENUMS_END                 \
            {0, 0}                          \
        };                                  \
    } // namespace luax

#define LUAX_ENUM(name,val) {name, val},


#define LUAX_TYPE_FUNCTIONS_BEGIN(cls)      \
    namespace luax {                        \
        template <> luaL_Reg type<cls>::type_functions[] = {

#define LUAX_TYPE_FUNCTIONS_END             \
            {0, 0}                          \
        };                                  \
    } // namespace luax


// Registry table name where luax stores userdata, see init() and push().
#define LUAX_UDATA "__luax_ud"

namespace luax
{

// Initialization:
// Create lightuserdata table with weak values.
// It used to store instance pointers to reuse already pushed values.
static void init(lua_State * L)
{
    lua_newtable(L);                            // tbl
    lua_newtable(L);                            // tbl mt
    lua_pushliteral(L, "__mode");               // tbl mt key
    lua_pushliteral(L, "v");                    // tbl mt key value
    lua_rawset(L, -3);                          // mt.__mode = 'v', tbl mt
    lua_setmetatable(L, -2);                    // tbl.__mt = mt, tbl
    lua_setfield(L, LUA_REGISTRYINDEX, LUAX_UDATA); // registry[key] = tbl
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------


/** Instance wrapper. */
struct Wrapper
{
    void *ptr;
    bool use_gc;
};
//------------------------------------------------------------------------------

/** Method wrapper. */
template <typename T>
struct Method
{
    typedef int (T::*Func)(lua_State*);
    const char *name;
    Func method;
};
//------------------------------------------------------------------------------

/** Enum wrapper. */
struct Enum
{
    const char *name;
    int val;
};
//------------------------------------------------------------------------------

/** Function base property. */
struct FuncProperty
{
    const char *name;
    lua_CFunction getter;
    lua_CFunction setter;
};
//------------------------------------------------------------------------------

/** Method based property. */
template <typename T>
struct MethodProperty
{
    typedef typename Method<T>::Func Func;
    const char *name;
    Func getter;
    Func setter;
};
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------


/**
 * Simple template class to bind c++ type to lua.
 *
 * It supports:
 *
 * - Constructor, methods and free functions binding.
 * - Properties.
 * - Simple inheritance (single metatable inheritance).
 * - GC.
 * - User hooks.
 */
template <typename T>
class type
{
public:
    static const char* usr_name();
    static const char* usr_super_name();
    static void usr_instance_mt(lua_State *L);
    static void usr_type_mt(lua_State *L);
    static int usr_getter(lua_State *L);
    static int usr_setter(lua_State *L);
    static bool usr_gc(lua_State *L, T *obj);
    static T* usr_constructor(lua_State *L);

    static luaL_Reg functions[];
    static Method<T> methods[];
    static FuncProperty func_properties[];
    static MethodProperty<T> method_properties[];

    static Enum type_enums[];
    static luaL_Reg type_functions[];

    static void register_in(lua_State *L);
    static inline int push(lua_State *L, T *obj, bool useGc = true);
    static inline T* get(lua_State *L, int index);
    static inline T* check_get(lua_State *L, int index);

private:
    static inline int create(lua_State *L);
    static inline int gc(lua_State *L);
    static inline int index(lua_State *L);
    static inline int newindex(lua_State *L);

    static void register_attrs(lua_State *L, bool customIndex);
    static inline int on_method(lua_State *L);
    static inline int on_getter(lua_State *L);
    static inline int on_setter(lua_State *L);
    static void register_type_attrs(lua_State *L);
};
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------


// Implementation.

template <typename T> const char* type<T>::usr_super_name() { return 0; }
template <typename T> void type<T>::usr_instance_mt(lua_State*) { }
template <typename T> void type<T>::usr_type_mt(lua_State*) { }
template <typename T> int type<T>::usr_getter(lua_State*) { return 0; }
template <typename T> int type<T>::usr_setter(lua_State*) { return 0; }
template <typename T> bool type<T>::usr_gc(lua_State*, T*) { return false; }
template <typename T> T* type<T>::usr_constructor(lua_State*) { return 0; }

template <typename T> luaL_Reg type<T>::functions[] = {0, 0};
template <typename T> Method<T> type<T>::methods[] = {0, 0};
template <typename T> FuncProperty type<T>::func_properties[] = {0, 0, 0};
template <typename T> MethodProperty<T> type<T>::method_properties[] = {0, 0, 0};
template <typename T> Enum type<T>::type_enums[] = {0, 0};
template <typename T> luaL_Reg type<T>::type_functions[] = {0, 0};
//------------------------------------------------------------------------------

template <typename T> int type<T>::create(lua_State *L)
{
    T *obj = usr_constructor(L);
    if (!obj)
        luaL_error(L, "Error creating %s", usr_name());
    push(L, obj);
    return 1;
}
//------------------------------------------------------------------------------

template <typename T> int type<T>::gc(lua_State *L)
{
    Wrapper *wrapper = static_cast<Wrapper*>(lua_touserdata(L, 1));
    assert(wrapper);

    T *obj = static_cast<T*>(wrapper->ptr);
    if (wrapper->use_gc)
    {
        if (!usr_gc(L, obj))
            delete obj;
    }

    return 0;
}
//------------------------------------------------------------------------------

template <typename T> int type<T>::index(lua_State *L)
{
    // Initial stack: obj key

    if (!lua_getmetatable(L, 1))         // obj key mt
        return usr_getter(L);

    while (1)
    {
        // No metatable, return nil
        if (lua_isnil(L, -1))
        {
            lua_pop(L, 1);              // obj key
            return usr_getter(L);
        }

        lua_pushstring(L, "__getters"); // obj key mt '__getters'
        lua_rawget(L, -2);              // obj key mt getters
        if (lua_isnil(L, -1))
            continue;
        lua_pushvalue(L, 2);            // obj key mt getters key
        lua_rawget(L, -2);              // obj key mt getters getters[key]

        // If no getter is found in the current mt.
        if (lua_isnil(L, -1))
        {
            lua_pop(L, 2);              // obj key mt
            lua_pushvalue(L, 2);        // obj key mt key
            lua_rawget(L, -2);          // obj key mt mt[key]

            // No attr found in the current metatable.
            if (lua_isnil(L, -1))
            {
                lua_pop(L, 1);              // obj key mt
                if (!lua_getmetatable(L, -1))    // obj key mt mt_mt
                    lua_pushnil(L);
                lua_remove(L, -2);          // obj key mt_mt
                continue;
            }
            break;
        }
        // If getter is found then call it.
        else if (lua_type(L, -1) == LUA_TFUNCTION)
        {
            lua_pushvalue(L, 1);        // obj key mt getters func obj
            lua_call(L, 1, 1);          // obj key mt getters result
            break;
        }
    }

    return 1;
}
//------------------------------------------------------------------------------

template <typename T> int type<T>::newindex(lua_State *L)
{
    // Initial stack: obj key val

    if (!lua_getmetatable(L, 1))         // obj key val mt
        return usr_setter(L);

    while (1)
    {
        if (lua_isnil(L, -1))
        {
            lua_pop(L, 1);              // obj key val
            return usr_setter(L);
        }

        lua_pushstring(L, "__setters"); // obj key val mt '__setters'
        lua_rawget(L, -2);              // obj key val mt setters
        if (lua_isnil(L, -1))
            continue;
        lua_pushvalue(L, 2);            // obj key val mt setters key
        lua_rawget(L, -2);              // obj key val mt setters setters[key]

        // If no setter is found in the current mt;
        // Try super metatable.
        if (lua_isnil(L, -1))
        {
            lua_pop(L, 2);              // obj key val mt
            if (!lua_getmetatable(L, -1))    // obj key val mt mt_mt
                lua_pushnil(L);
            lua_remove(L, -2);          // obj key val mt_mt
            continue;
        }
        // If setter is found then call it.
        else if (lua_type(L, -1) == LUA_TFUNCTION)
        {
            lua_pushvalue(L, 1);        // obj key val mt setters func obj
            lua_pushvalue(L, 3);        // obj key val mt setters func obj val
            lua_call(L, 2, 0);          // obj key mt getters result
            break;
        }
    }

    return 0;
}
//------------------------------------------------------------------------------

template <typename T> void type<T>::register_attrs(lua_State *L,
                                                   bool customIndex)
{
    for (luaL_reg *m = functions; m->name; ++m)
    {
        lua_pushcfunction(L, m->func);
        lua_setfield(L, -2, m->name);
    }

    for (Method<T> *m = methods; m->name; ++m)
    {
        lua_pushlightuserdata(L, static_cast<void*>(m));
        lua_pushcclosure(L, type<T>::on_method, 1);
        lua_setfield(L, -2, m->name);
    }

    if (customIndex)
    {
        lua_newtable(L);        // mt getters
        lua_newtable(L);        // mt getters setters

        for (FuncProperty *m = func_properties; m->name; ++m)
        {
            if (m->getter > 0)
            {
                lua_pushcfunction(L, m->getter);
                lua_setfield(L, -3, m->name);
            }
            if (m->setter)
            {
                lua_pushcfunction(L, m->setter);
                lua_setfield(L, -2, m->name);
            }
        } // for FuncProperty

        for (MethodProperty<T> *m = method_properties; m->name; ++m)
        {
            if (m->getter)
            {
                lua_pushlightuserdata(L, static_cast<void*>(m));
                lua_pushcclosure(L, on_getter, 1);
                lua_setfield(L, -3, m->name);
            }
            if (m->setter)
            {
                lua_pushlightuserdata(L, static_cast<void*>(m));
                lua_pushcclosure(L, on_setter, 1);
                lua_setfield(L, -2, m->name);
            }
        } // for ClassProperty

        lua_pushliteral(L, "__setters");    // mt getters setters key
        lua_insert(L, -2);                  // mt getters key setters
        lua_rawset(L, -4);                  // mt getters

        lua_pushliteral(L, "__getters");    // mt getters key
        lua_insert(L, -2);                  // mt key getters
        lua_rawset(L, -3);                  // mt
    } // if (func_properties || method_properties)
}
//------------------------------------------------------------------------------

template <typename T> int type<T>::on_method(lua_State * L)
{
    typedef Method<T> Meth;
    Meth *m = static_cast<Meth*>(lua_touserdata(L, lua_upvalueindex(1)));
    T *obj = type<T>::get(L, 1);
    assert(obj);
    lua_remove(L, 1);
    return (obj->*(m->method))(L);
}
//------------------------------------------------------------------------------

template <typename T> int type<T>::on_getter(lua_State * L)
{
    typedef MethodProperty<T> Meth;
    Meth *m = static_cast<Meth*>(lua_touserdata(L, lua_upvalueindex(1)));
    T *obj = type<T>::get(L, 1);
    assert(obj);
    lua_remove(L, 1);
    return (obj->*(m->getter))(L);
}
//------------------------------------------------------------------------------

template <typename T> int type<T>::on_setter(lua_State * L)
{
    typedef MethodProperty<T> Meth;
    Meth *m = static_cast<Meth*>(lua_touserdata(L, lua_upvalueindex(1)));
    T *obj = type<T>::get(L, 1);
    assert(obj);
    lua_remove(L, 1);
    return (obj->*(m->setter))(L);
}
//------------------------------------------------------------------------------

// stack: tbl mt
template <typename T> void type<T>::register_type_attrs(lua_State *L)
{
    for (luaL_reg *m = type_functions; m->name; ++m)
    {
        lua_pushcfunction(L, m->func);
        lua_setfield(L, -3, m->name);   // set to type table, not it's mt.
    }

    for (Enum *m = type_enums; m->name; ++m)
    {
        lua_pushinteger(L, m->val);
        lua_setfield(L, -3, m->name);   // set to type table, not it's mt.
    }
}
//------------------------------------------------------------------------------

template <typename T> void type<T>::register_in(lua_State *L)
{
    // Instance specific.

    // Already registered.
    if (!luaL_newmetatable(L, usr_name()))  // stack: mt
    {
        lua_pop(L, 1);
        return;
    }

    bool custom_index = false;

    // If super class is set then check it's metatable for properties.
    // If it has props then we have to control __index and __newindex.
    // This allows to correctly handle parent properties and function calls.
    if (usr_super_name())
    {
        luaL_getmetatable(L, usr_super_name());
        lua_pushstring(L, "__getters");
        lua_rawget(L, -2);
        custom_index = !lua_isnil(L, -1);
        lua_pop(L, 2);
    }
    else
        custom_index = func_properties[0].name || method_properties[0].name;


    if (custom_index)
    {
        lua_pushcfunction(L, index);
        lua_setfield(L, -2, "__index");
        lua_pushcfunction(L, newindex);
        lua_setfield(L, -2, "__newindex");
    }
    else
    {
        // Lookup missing object attrs in the metatable by default.
        lua_pushvalue(L, -1);                   // stack: mt mt
        lua_setfield(L, -2, "__index");         // mt._index = mt, stack: mt
    }

    lua_pushcfunction(L, gc);
    lua_setfield(L, -2, "__gc");

    int top = lua_gettop(L);

    // stack: mt.
    usr_instance_mt(L);

    // Cleanup stack from usr_instance_mt() garbage if present.
    lua_settop(L, top);

    register_attrs(L, custom_index);

    if (usr_super_name())
    {
        luaL_getmetatable(L, usr_super_name());
        lua_setmetatable(L, -2);
    }

    // Cleanup stack.
    lua_settop(L, top - 1);

    // Type specific.

    top = lua_gettop(L);

    lua_newtable(L);                // tbl
    lua_newtable(L);                // tbl mt
    lua_pushvalue(L, -1);           // tbl mt mt(copy)
    lua_setmetatable(L, -3);        // tbl.__mt = mt, stack: tbl mt

    lua_pushstring(L, "__call");    // tbl mt '__call'
    lua_pushcfunction(L, create);   // tbl mt '__call' func
    lua_rawset(L, -3);              // tbl mt.__call = func, stack: tbl mt

    lua_pushvalue(L, -2);           // tbl mt tbl(copy)
    lua_setglobal(L, usr_name());   // _G[name] = name, stack: tbl mt

    int type_top = lua_gettop(L);
    usr_type_mt(L);

    // Cleanup stack from usr_type_mt() garbage if present.
    lua_settop(L, type_top);
    register_type_attrs(L);

    lua_settop(L, top);
}
//------------------------------------------------------------------------------

// NOTE: why we use lua_pushfstring() as a key to associate instance with
// the userdata. We might use something like:
//
//  lua_getfield(L, LUA_REGISTRYINDEX, LUAX_UDATA);     // udata
//  lua_pushnumber(L, reinterpret_cast<LUA_INTEGER>(obj)); // udata ptr
//  lua_gettable(L, -2);                                // udata udata[ptr]
//
// But if obj is struct or class:
//
//  class Obj { Some attr; }
//
// then address of the obj and obj->attr may be the same; as a result if we do
// push(obj); push(obj->attr); second call will extract userdata for obj.
template <typename T> int type<T>::push(lua_State *L, T *obj, bool useGc)
{
    if (!obj)
    {
        lua_pushnil(L);
        return 1;
    }

    lua_getfield(L, LUA_REGISTRYINDEX, LUAX_UDATA);     // udata
    lua_pushfstring(L, "%s_%p", usr_name(), obj);       // udata name
    lua_gettable(L, -2);                                // udata udata[name]

    // If no object is associated then we create full userdata
    // and attach type metatable. Also we associate val with full userdata.
    if (lua_isnil(L, -1))
    {
        // Remove nil from the stack.
        lua_pop(L, 1);

        // Create userdata, stack: udata ud
        Wrapper *wrapper =
            static_cast<Wrapper*>(lua_newuserdata(L, sizeof(Wrapper)));
        wrapper->ptr = static_cast<void*>(obj);
        wrapper->use_gc = useGc;

        // Set type metatable to the userdata.
        luaL_getmetatable(L,  usr_name());              // udata ud mt
        lua_setmetatable(L, -2);                        // udata ud

        // Link userdata to name for later use. This allows to reuse the same
        // userdata if obj pushed multiple times.
        // NOTE: use the same useGc for the multiple pushes.
        lua_pushfstring(L, "%s_%p", usr_name(), obj);   // udata ud name
        lua_pushvalue(L, -2);                           // udata ud name ud
        lua_settable(L, -4); // udata[name] = ud,          udata ud
    }
    lua_remove(L, -2);                                  // ud

    return 1;
}
//------------------------------------------------------------------------------

template <typename T> T* type<T>::get(lua_State *L, int index)
{
    Wrapper *wrapper = static_cast<Wrapper*>(lua_touserdata(L, index));
    return wrapper ? static_cast<T*>(wrapper->ptr) : 0;
}
//------------------------------------------------------------------------------

template <typename T> T* type<T>::check_get(lua_State *L, int index)
{
    Wrapper *wrapper =
        static_cast<Wrapper*>(luaL_checkudata(L, index, usr_name()));
    T *ptr = wrapper ? static_cast<T*>(wrapper->ptr) : 0;
    if (!ptr)
        luaL_error(L, "Invalid [%s] object at index %d.", usr_name(), index);
    return ptr;
}
//------------------------------------------------------------------------------

} // namespace luax

#endif // LUAX_H
