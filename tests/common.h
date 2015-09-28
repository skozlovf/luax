#ifndef COMMON_H
#define COMMON_H

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#ifdef __cplusplus
}
#endif

// Helper macro to run scripts.
// See LuaxTest::runScript().
#define EXPECT_SCRIPT(txt) EXPECT_TRUE(runScript(txt))
#define ASSERT_SCRIPT(txt) ASSERT_TRUE(runScript(txt))

// Tests fixture, manages lua VM.
class BaseLuaxTest: public ::testing::Test
{
protected:
    void SetUp() override { L = luaL_newstate(); luaL_openlibs(L); }
    void TearDown() override { lua_close(L); }

    ::testing::AssertionResult runScript(const char *txt)
    {
        if (luaL_loadbuffer(L, txt, strlen(txt), "test")
            || lua_pcall(L, 0, 0, 0))
        {
            auto res = ::testing::AssertionFailure() << lua_tostring(L, -1);
            lua_pop(L, -1);
            return res;
        }
        return ::testing::AssertionSuccess();
    }

    lua_State *L;
};

#endif // COMMON_H

