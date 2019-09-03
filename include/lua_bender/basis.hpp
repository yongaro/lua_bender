#ifndef LUA_BENDER_TOOLS_HPP
#define LUA_BENDER_TOOLS_HPP
#pragma once

#include <lua.hpp>
#include <memory>
#include <functional>
#include <tuple>
#include <iostream>
#include <type_traits>

#pragma warning(push)
// Disable possible loss of data warning
#pragma warning(disable: 4244)

#define LUA_BENDER_ERROR(msg) std::cerr << "[LUA_BENDER_ERROR] " << std::endl\
    << "file : " << __FILE__ << std::endl\
    << "line " << __LINE__ << " in " << __func__ << "()"\
    << std::endl << msg << std::endl;\
    exit(EXIT_FAILURE);

#define LUA_BENDER_WARNING(msg) std::cerr << "[LUA_BENDER_WARNING] " << std::endl\
    << "file : " << __FILE__ << std::endl\
    << "line " << __LINE__ << " in " << __func__ << "()"\
    << std::endl << msg << std::endl;

#define LUA_BENDER_LOG(msg) std::cout << "[LUA_BENDER] " << msg << std::endl;


namespace lua_bender{
    // The value template is used to both push and load values from the lua stack.
    // This first set provides support for the primitive types.
    //
    // As you may see here and in the function.hpp templates, const reference is ALWAYS added to the requested type.
    // For a given type T, the function adapter doesn't need to use different behaviors for T, T& and const T&.
    // Adding const ref to T is a quick way to make the compiler search always converge to a single implementation
    // and thus avoids duplicating the code for the 3 cases mentionned above.
    //
    // If you need a value template for a custom data structure, check the user_data.hpp header which contains
    // implementations that should fit all the remaining use cases not covered here.


    template<typename T>
    struct value{};

    template<>
    struct value<const float&>{
        static float check(lua_State* L, int index){ return luaL_checknumber(L, index); }

        static int push(lua_State* L, float value){
            lua_pushnumber(L, value);
            return 1;
        }
    };

    template<>
    struct value<const double&>{
        static double check(lua_State* L, int index){ return luaL_checknumber(L, index); }

        static int push(lua_State* L, double value){
            lua_pushnumber(L, value);
            return 1;
        }
    };

    template<>
    struct value<const int&>{
        static int check(lua_State* L, int index){ return luaL_checkinteger(L, index); }

        static int push(lua_State* L, int value){
            lua_pushinteger(L, value);
            return 1;
        }
    };

    template<>
    struct value<const bool&>{
        static bool check(lua_State* L, int index){ return lua_toboolean(L, index); }

        static int push(lua_State* L, bool value){
            lua_pushboolean(L, value);
            return 1;
        }
    };

    template<>
    struct value<const char* const&>{
        static const char* check(lua_State* L, int index){ return lua_tolstring(L, index, NULL); }

        static int push(lua_State* L, const char* value){
            lua_pushstring(L, value);
            return 1;
        }
    };


    template<>
    struct value<const std::string&>{
        static std::string check(lua_State* L, int index){
            return std::string(lua_tolstring(L, index, NULL));
        }

        static int push(lua_State* L, const std::string& value){
            lua_pushlstring(L, value.c_str(), value.size());
            return 1;
        }
    };
}

#pragma warning(pop)

#endif
