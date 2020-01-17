#ifndef LUA_BENDER_USER_DATA_HPP
#define LUA_BENDER_USER_DATA_HPP
#pragma once

#include "basis.hpp"

#define lua_bender_register_user_data_name(type, name)\
    template<> std::string lua_bender::user_data_type_name<type>::s_name = name


namespace lua_bender{
    struct user_data{
        void*       m_data;
        bool        m_garbage_collected;

        static inline void push(lua_State* L, void* data, const char* type_name, bool garbage_collected = false){
            user_data** udata = (user_data**)lua_newuserdata(L, sizeof(user_data*));
            *udata = new user_data();
            (*udata)->m_data = data;
            (*udata)->m_garbage_collected = garbage_collected;

            luaL_getmetatable(L, type_name);
            lua_setmetatable(L, -2);
        }

        static inline void push(lua_State* L, void* data){
            user_data** udata = (user_data**)lua_newuserdata(L, sizeof(user_data*));
            *udata = new user_data();
            (*udata)->m_data = data;
            (*udata)->m_garbage_collected = false;
        }

        static inline user_data* check(lua_State* L, int index){
            return *(user_data**)lua_touserdata(L, index);
        }
    };

    template<class C>
    struct user_data_type_name{ static std::string s_name; };


    template<class C>
    struct value<C&>{
        static C& check(lua_State* L, int index){
            user_data* udata = user_data::check(L, index);
            return *((C*)udata->m_data);
        }

        static int push(lua_State* L, C& value){
            user_data::push(L, &value, user_data_type_name<C>::s_name.c_str(), false);
            return 1;
        }
    };

    template<class C>
    struct value<const C&>{
        static C& check(lua_State* L, int index){
            user_data* udata = user_data::check(L, index);
            return *((C*)udata->m_data);
        }

        static int push(lua_State* L, C& value){
            user_data::push(L, &value, user_data_type_name<C>::s_name.c_str(), false);
            return 1;
        }
    };


    template<class C>
    struct value<C* const&>{
        static C* check(lua_State* L, int index){
            user_data* udata = user_data::check(L, index);
            if( udata != nullptr ){
                return (C*)udata->m_data;
            }
            return nullptr;
        }

        static int push(lua_State* L, C* value){
            user_data::push(L, value, user_data_type_name<C>::s_name.c_str(), false);
            return 1;
        }
    };
}

#endif
