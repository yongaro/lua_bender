#ifndef lUA_BENDER_SCRIPT_HPP
#define lUA_BENDER_SCRIPT_HPP
#pragma once

#include "basis.hpp"

namespace lua_bender{
    struct script{
        static void do_string(lua_State* L, const char* code){
            int res = luaL_dostring(L, code);
            if( res != LUA_OK ){
                if( lua_pcall(L, 0, LUA_MULTRET, 0) ){
                    printf("Error: %s \n", lua_tostring(L, -1));
                    lua_pop(L, 1);
                }
            }
        }

        static void do_file(lua_State* L, const char* file){
            int res = luaL_dofile(L, file);
            if( res != LUA_OK ){
                if( lua_pcall(L, 0, LUA_MULTRET, 0) ){
                    printf("Error: %s \n", lua_tostring(L, -1));
                    lua_pop(L, 1);
                }
            }
        }
    };
}


#endif
