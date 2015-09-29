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


/**
  * Creates and returns a reference, in the REGISTRY table,
  * for the object at the top of the stack (and pops the object).
  */
#define luax_refreg(L) luaL_ref(L, LUA_REGISTRYINDEX)

/**
 * Releases reference ref from the REGISTRY.
 * The entry is removed from the REGISTRY,
 * so that the referred object can be collected.
 */
#define luax_unrefreg(L,i) luaL_unref(L, LUA_REGISTRYINDEX, i)

/** Pushes onto the stack referenced REGISTRY value. */
#define luax_getrefreg(L,i) lua_rawgeti(L, LUA_REGISTRYINDEX, i)

/** Pushes onto the stack the value with name from the REGISTRY. */
#define luax_getregfield(L,name) lua_getfield(L, LUA_REGISTRYINDEX, name)

/** Pops a value from the stack and sets it as the new value of REGISTRY. */
#define luax_setregfield(L,name) lua_setfield(L, LUA_REGISTRYINDEX, name)


#define LUAX_UDATA_TABLE "_lxu"
#define LUAX_REGISTRY_INDEX "__luax_idx"
#define LUAX_REGISTRY_NEWINDEX "__luax_newidx"

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
    luax_setregfield(L, LUAX_UDATA_TABLE);      // registry["udata"] = tbl
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


// TODO: add type properties.
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
    static bool usr_getter(lua_State *L);
    static bool usr_setter(lua_State *L);
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

    static void register_attrs(lua_State *L);
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
template <typename T> bool type<T>::usr_getter(lua_State*) { return false; }
template <typename T> bool type<T>::usr_setter(lua_State*) { return false; }
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

    lua_getmetatable(L, 1);         // obj key mt
    lua_pushstring(L, "__getters"); // obj key mt '__getters'
    lua_rawget(L, -2);              // obj key mt getters
    lua_pushvalue(L, 2);            // obj key mt getters key
    lua_rawget(L, -2);              // obj key mt getters getters[key]

    // If no getter is found.
    if (lua_isnil(L, -1))
    {
        lua_pop(L, 2);              // obj key mt

        // Call user defined hook, if it returns false then
        // do default action - search directly in the metatable.
        if (!usr_getter(L))
        {
            // Store userdata in the registry for later use.
            // We do it becuase next lua_gettable() may trigger
            // parent metatable's __index and userdata value will be lost
            // for the function's self parameter.
            // See below.
            if (lua_isuserdata(L, 1))
            {
                lua_pushvalue(L, 1);
                luax_setregfield(L, LUAX_REGISTRY_INDEX);
            }

            lua_pushvalue(L, 2);    // obj key mt key

            // We use gettable instead of rawget to support metatable
            // inheritance if metatable has parent metatable.
            // The call looks like:
            //      ud.mt.parent_mt.__index[func](mt)
            // Normal call looks like:
            //      ud.mt.__index[func](ud)
            // As you see parent mt uses preceding mt instad of userdata.
            // So we'll use previously stored LUAX_REGISTRY_INDEX for the call.
            // See below.
            lua_gettable(L, -2);    // obj key mt mt[key]
        }
    }

    // If getter is a function then call it.
    else if (lua_type(L, -1) == LUA_TFUNCTION)
    {
        // If self is not an userdata then this function is inherited from the
        // super class. We have to use LUAX_REGISTRY_INDEX instead of current
        // object (which is preceding metatable).
        if (!lua_isuserdata(L, 1))
        {
            luax_getregfield(L, LUAX_REGISTRY_INDEX);
            // If no ud value stored then put back old value.
            if (lua_isnil(L, -1))           // obj key mt getters func ud
            {
                lua_pop(L, 1);              // obj key mt getters func
                lua_pushvalue(L, 1);        // obj key mt getters func ud
            }
            // Erase ud from the registry since we don't need it anymore.
            else
            {
                lua_pushnil(L);
                luax_setregfield(L, LUAX_REGISTRY_INDEX);
            }
        }

        // If self is an userdata then use it.
        else
            lua_pushvalue(L, 1);        // obj key mt getters func obj

        lua_call(L, 1, 1);          // obj key mt getters result
    }

    // Erase ud from the registry.
    else if (!lua_isuserdata(L, 1))
    {
        lua_pushnil(L);
        luax_setregfield(L, LUAX_REGISTRY_INDEX);
    }

    return 1;
}
//------------------------------------------------------------------------------

template <typename T> int type<T>::newindex(lua_State *L)
{
    // Initial stack: obj key val

    lua_getmetatable(L, 1);         // obj key val mt
    lua_pushstring(L, "__setters"); // obj key val mt '__setters'
    lua_rawget(L, -2);              // obj key val mt setters
    lua_pushvalue(L, 2);            // obj key val mt setters key
    lua_rawget(L, -2);              // obj key val mt setters setters[key]

    // If no setter is found.
    if (lua_isnil(L, -1))
    {
        lua_pop(L, 2);              // obj key val mt

        // Call user defined hook, if it returns false then
        // do default action - search directly in the metatable.
        if (!usr_setter(L))
        {
            // See index() comments about same action.
            if (lua_isuserdata(L, 1))
            {
                lua_pushvalue(L, 1);
                luax_setregfield(L, LUAX_REGISTRY_NEWINDEX);
            }

            lua_pushvalue(L, 2);    // obj key val mt key
            lua_pushvalue(L, 3);    // obj key val mt key val
            // we use settable instead of rawset to support metatable
            // inheritance if metatable has parent metatable.
            lua_settable(L, -3);    // obj key val mt
        }
    }

    // If setter is a function then call it.
    // stack: obj key val mt setters func
    else if (lua_type(L, -1) == LUA_TFUNCTION)
    {
        // See index() comments about same action.
        if (!lua_isuserdata(L, 1))
        {
            luax_getregfield(L, LUAX_REGISTRY_NEWINDEX);
            if (lua_isnil(L, -1))           // obj key mt getters func ud
            {
                lua_pop(L, 1);              // obj key mt getters func
                lua_pushvalue(L, 1);        // obj key mt getters func ud
            }
            else
            {
                lua_pushnil(L);
                luax_setregfield(L, LUAX_REGISTRY_NEWINDEX);
            }
        }

        // If self is an userdata then use it.
        else
            lua_pushvalue(L, 1);        // obj key val mt setters func obj

        lua_pushvalue(L, 3);        // obj key val mt setters func obj val
        lua_call(L, 2, 0);          // obj key mt getters result
    }

    // Erase ud from the registry.
    else if (!lua_isuserdata(L, 1))
    {
        lua_pushnil(L);
        luax_setregfield(L, LUAX_REGISTRY_NEWINDEX);
    }

    return 0;
}
//------------------------------------------------------------------------------

template <typename T> void type<T>::register_attrs(lua_State *L)
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

    if (func_properties[0].name || method_properties[0].name)
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

    if (func_properties[0].name || method_properties[0].name)
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

    register_attrs(L);

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

template <typename T> int type<T>::push(lua_State *L, T *obj, bool useGc)
{
    if (!obj)
        return luaL_error(L, "Can't push null [%s] instance.", usr_name());

    luax_getregfield(L, LUAX_UDATA_TABLE);              // udata
    lua_pushfstring(L, "%s_%p", usr_name(), obj);       // udata 'name_ptr'
    lua_gettable(L, -2);                                // udata udata[ptr]

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

        // Associate val with the userdata.
        lua_pushlightuserdata(L, wrapper->ptr);         // udata ud ptr
        lua_pushvalue(L, -2);                           // udata ud ptr ud
        lua_settable(L, -4); // udata[val] = ud, stack:    udata ud
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
