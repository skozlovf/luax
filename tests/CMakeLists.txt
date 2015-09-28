project(luax-tests)
cmake_minimum_required(VERSION 2.8)
enable_testing()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED 11)

include_directories(../include)

# Set LUA_DIR.
find_package(Lua REQUIRED)
include_directories(${LUA_INCLUDE_DIR})

set(GMOCK_SRC gmock/gmock-gtest-all.cc gmock/gmock_main.cc)
include_directories(gmock)

aux_source_directory(. SRC)
file(GLOB SRC_H ../include/*.h)  # to see *.h in project tree.

add_executable(luax-tests ${GMOCK_SRC} ${SRC} ${SRC_H})
target_link_libraries(luax-tests ${LUA_LIBRARIES})

add_test(NAME luax
         COMMAND luax-tests
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

set_tests_properties(luax
    PROPERTIES ENVIRONMENT "PATH=${LUA_INCLUDE_DIR}/../bin")
