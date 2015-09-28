#include "common.h"
#include "luax.h"
#include "luax_utils.h"

class LuaxExampleTest: public BaseLuaxTest {};

class Item
{
public:
    Item(int x = 0, int y = 0): m_x(x), m_y(y) { }
    Item(const Item &other): m_x(other.m_x), m_y(other.m_y) {}
    virtual ~Item() { }

    int getX(lua_State *L)
    {
        luax::push(L, m_x);
        std::string d = luax::dump::stack(L);
        return 1;
    }

    int setX(lua_State *L)
    {
        m_x = luax::checkget<int>(L, 1);
        return 0;
    }

    int m_x;
    int m_y;
};
//------------------------------------------------------------------------------

class ItemHolder
{
public:
    ItemHolder() {}
    ItemHolder(const Item &item): m_item(item) {}

    int getItem(lua_State *L)
    {
        luax::type<Item>::push(L, &m_item, false);
        return 1;
    }

    int setItem(lua_State *L)
    {
        Item *i = luax::type<Item>::check_get(L, 1);
        m_item.m_x = i->m_x;
        m_item.m_y = i->m_y;
        return 0;
    }

protected:
    Item m_item;
};
//------------------------------------------------------------------------------

class ItemHolderExt: public ItemHolder
{
public:
    ItemHolderExt() {}
    ItemHolderExt(const Item &item): ItemHolder(item) {}

    int getY(lua_State *L)
    {
        luax::push(L, m_item.m_y);
        return 1;
    }
};
//------------------------------------------------------------------------------

// Bindings.

namespace luax {

// Item

template <> const char* type<Item>::usr_name() { return "Item"; }
template <> MethodProperty<Item> type<Item>::method_properties[] = {
    {"x", &Item::getX, &Item::setX},
    {0, 0, 0}
};

template <> Item* type<Item>::usr_constructor(lua_State *L)
{
    int top = lua_gettop(L);

    if (top == 1)
        return new Item();
    else
    {
        int x = luax::checkget<int>(L, 2);
        int y = luax::checkget<int>(L, 3);
        return new Item(x, y);
    }
    return 0;
}

// Forbid to set unknown values.
// stack: obj key val mt
template <> bool type<Item>::usr_newindex(lua_State *L)
{
    const char *key = lua_tostring(L, 2);
    luaL_error(L, "Error setting value to [%s]", key);
    return true;
}

// ItemHolder

template <> const char* type<ItemHolder>::usr_name() { return "ItemHolder"; }
template <> MethodProperty<ItemHolder> type<ItemHolder>::method_properties[] = {
    {"item", &ItemHolder::getItem, &ItemHolder::setItem},
    {0, 0, 0}
};

template <> ItemHolder* type<ItemHolder>::usr_constructor(lua_State *L)
{
    int top = lua_gettop(L);

    if (top == 1)
        return new ItemHolder();
    else
    {
        Item *i = luax::type<Item>::check_get(L, 2);
        return new ItemHolder(*i);
    }
    return 0;
}

// ItemHolderExt

template <> const char* type<ItemHolderExt>::usr_name() { return "ItemHolderExt"; }
template <> const char* type<ItemHolderExt>::usr_super_name() { return "ItemHolder"; }
template <> MethodProperty<ItemHolderExt> type<ItemHolderExt>::method_properties[] = {
    {"y", &ItemHolderExt::getY, 0},
    {0, 0, 0}
};

template <> ItemHolderExt* type<ItemHolderExt>::usr_constructor(lua_State *L)
{
    int top = lua_gettop(L);

    if (top == 1)
        return new ItemHolderExt();
    else
    {
        Item *i = luax::type<Item>::check_get(L, 2);
        return new ItemHolderExt(*i);
    }
    return 0;
}
}

TEST_F(LuaxExampleTest, push)
{
    luax::init(L);
    luax::type<Item>::register_in(L);
    luax::type<ItemHolder>::register_in(L);
    luax::type<ItemHolderExt>::register_in(L);

    EXPECT_SCRIPT("p = Item(1, 100)");
    EXPECT_SCRIPT("p.x = 67");
    EXPECT_SCRIPT("assert(p.x == 67)");
    EXPECT_SCRIPT("holder = ItemHolderExt(p)");
    EXPECT_SCRIPT("assert(holder.item.x == 67)");
    EXPECT_SCRIPT("holder.item.x = 5");
    EXPECT_SCRIPT("assert(holder.item.x == 5)");
    EXPECT_SCRIPT("assert(holder.y == 100)");
    EXPECT_SCRIPT("assert(p.x == 67)");

    // Test usr_newindex().
    EXPECT_SCRIPT("status, err = pcall(function() p.z = 2 end)");
    EXPECT_SCRIPT("check = 'Error setting value to [z]'");
    EXPECT_SCRIPT("assert(err:sub(err:len() - check:len() + 1) == check)");
}
//------------------------------------------------------------------------------
