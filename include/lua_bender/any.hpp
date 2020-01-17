#ifndef LUA_BENDER_ANY_HPP
#define LUA_BENDER_ANY_HPP
#pragma once

#include "basis.hpp"
#include "user_data.hpp"
#include <vector>


namespace lua_bender{
    struct lua_any_t{
        lua_Number    m_number;
        std::string   m_str;
        void*         m_udata;
        lua_CFunction m_func;
        int           m_lua_type;

        lua_any_t(): m_number(), m_str(), m_udata(), m_lua_type(){}

        static void get_results(lua_State* L, std::vector<lua_any_t>& res){
            int returned_value_count = lua_gettop(L);
            user_data* udata;

            res.clear();
            res.resize(returned_value_count);
            // Reading the Lua stack from bottom to top.
            for(int i = 0; i < returned_value_count; ++i){
                int type = lua_type(L, i+1);
                res[i].m_lua_type = type;
                switch( type ){
                    case LUA_TNIL:
                        LUA_BENDER_LOG_ERROR("lua_bender::script::get_results is acessing an unexpected nil value");
                        break;
                    case LUA_TNONE:
                        LUA_BENDER_LOG_ERROR("lua_bender::script::get_results is acessing an unexpected no type value");
                        break;
                    case LUA_TNUMBER:
                        res[i].m_number = luaL_checknumber(L, i+1);
                        break;
                    case LUA_TTABLE:
                        LUA_BENDER_LOG_ERROR("lua_bender::script::get_results is accessing a table which is not supported for now.");
                        break;
                    case LUA_TSTRING:
                        res[i].m_str = luaL_checkstring(L, i+1);
                        break;
                    case LUA_TUSERDATA:
                        udata = *(user_data**)lua_touserdata(L, i+1);
                        if( udata != nullptr ){
                            // If a user data is returned AND recovered from a script, the ownership is transfered to the data consummer.
                            udata->m_garbage_collected = false;
                            res[i].m_udata = udata->m_data;
                        }
                        break;
                    case LUA_TLIGHTUSERDATA:
                        LUA_BENDER_LOG_ERROR("lua_bender::script::get_results is accessing a light user data which is not supported.");
                        break;
                    case LUA_TTHREAD:
                        LUA_BENDER_LOG_ERROR("lua_bender::script::get_results is accessing a thread which is not supported for now.");
                        break;
                    case LUA_TBOOLEAN:
                        res[i].m_number = lua_toboolean(L, i+1);
                        break;
                    case LUA_TFUNCTION:
                        LUA_BENDER_LOG_ERROR("lua_bender::script::get_results is accessing a function which is not supported for now.");
                        break;
                }
            }
        }


        void log_type_name() const{
            switch( m_lua_type ){
                case LUA_TNIL:
                    LUA_BENDER_LOG_INFO("any is type NIL");
                    break;
                case LUA_TNONE:
                    LUA_BENDER_LOG_INFO("any is type NONE");
                    break;
                case LUA_TNUMBER:
                    LUA_BENDER_LOG_INFO("any is type NUMBER: %f", m_number);
                    break;
                case LUA_TTABLE:
                    LUA_BENDER_LOG_INFO("any is type TABLE");
                    break;
                case LUA_TSTRING:
                    LUA_BENDER_LOG_INFO("any is type STRING: \"%s\"", m_str.c_str());
                    break;
                case LUA_TUSERDATA:
                    LUA_BENDER_LOG_INFO("any is type USER DATA");
                    break;
                case LUA_TLIGHTUSERDATA:
                    LUA_BENDER_LOG_INFO("any is type LIGHT USER DATA");
                    break;
                case LUA_TTHREAD:
                    LUA_BENDER_LOG_INFO("any is type THREAD");
                    break;
                case LUA_TBOOLEAN:
                    LUA_BENDER_LOG_INFO("any is type BOOLEAN: %f", m_number);
                    break;
                case LUA_TFUNCTION:
                    LUA_BENDER_LOG_INFO("any is type FUNCTION");
                    break;
            }
        }
    };


}

#endif
