#ifndef LUA_BENDER_TOOLS_HPP
#define LUA_BENDER_TOOLS_HPP
#pragma once

#include <lua.hpp>
#include <memory>
#include <string>
#include <type_traits>

#pragma warning(push)
// Disable possible loss of data warning
#pragma warning(disable: 4244)




#ifdef __ANDROID__
    #include <android/log.h>
    #define LUA_BENDER_ANDROID_TAG "android.lua_bender"
    #define LUA_BENDER_LOG_ERROR(...)   __android_log_print(ANDROID_LOG_ERROR, LUA_BENDER_ANDROID_TAG, __VA_ARGS__)
    #define LUA_BENDER_LOG_WARNING(...) __android_log_print(ANDROID_LOG_WARN,  LUA_BENDER_ANDROID_TAG, __VA_ARGS__)
#else
    #include <stdio.h>
    #define LUA_BENDER_LOG_ERROR(...)   fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n")
    #define LUA_BENDER_LOG_WARNING(...) fprintf(stdout, __VA_ARGS__); fprintf(stdout, "\n")
#endif


#ifdef NDEBUG
    #define LUA_BENDER_LOG_DEBUG(...)
    #define LUA_BENDER_LOG_INFO(...)
#else
    #ifdef __ANDROID__
        #define LUA_BENDER_LOG_DEBUG(...) __android_log_print(ANDROID_LOG_DEBUG, LUA_BENDER_ANDROID_TAG, __VA_ARGS__)
        #define LUA_BENDER_LOG_INFO(...)  __android_log_print(ANDROID_LOG_INFO,  LUA_BENDER_ANDROID_TAG, __VA_ARGS__)
    #else
        #define LUA_BENDER_LOG_DEBUG(...) fprintf(stdout, __VA_ARGS__); fprintf(stdout, "\n")
        #define LUA_BENDER_LOG_INFO(...)  fprintf(stdout, __VA_ARGS__); fprintf(stdout, "\n")
    #endif
#endif


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
        static float check(lua_State* L, int index){ return float(luaL_checknumber(L, index)); }

        static int push(lua_State* L, float value){
            lua_pushnumber(L, double(value));
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
        static int check(lua_State* L, int index){ return int(luaL_checkinteger(L, index)); }

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
        static const char* check(lua_State* L, int index){ return lua_tolstring(L, index, nullptr); }

        static int push(lua_State* L, const char* value){
            lua_pushstring(L, value);
            return 1;
        }
    };


    template<>
    struct value<const std::string&>{
        static std::string check(lua_State* L, int index){
            return std::string(lua_tolstring(L, index, nullptr));
        }

        static int push(lua_State* L, const std::string& value){
            lua_pushlstring(L, value.c_str(), value.size());
            return 1;
        }
    };
}

#pragma warning(pop)

#endif
