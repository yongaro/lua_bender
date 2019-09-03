#ifndef LUA_BENDER_USER_DATA_HPP
#define LUA_BENDER_USER_DATA_HPP
#pragma once

#include "basis.hpp"

namespace lua_bender{
    void erase_sub_strings(std::string& main, const std::string& sub){
        size_t pos = std::string::npos;

        while( (pos  = main.find(sub) )!= std::string::npos ){
            main.erase(pos, sub.length());
        }
    }

    void replace_all(std::string& data, std::string toSearch, std::string replaceStr){
        // Get the first occurrence
        size_t pos = data.find(toSearch);

        // Repeat till end is reached
        while( pos != std::string::npos){
            // Replace this occurrence of Sub String
            data.replace(pos, toSearch.size(), replaceStr);
            // Get the next occurrence from the current position
            pos =data.find(toSearch, pos + replaceStr.size());
        }
    }


    template<class T>
    std::string get_type_name(){
        std::string name = typeid(T).name();
        // On MSVC at least the types are mangled with those prefixes.
        erase_sub_strings(name, "struct ");
        erase_sub_strings(name, "class ");
        // replace the namespace sperator with underscore to avoid conflicts with lua syntax.
        replace_all(name, "::", "_");
        return name;
    }


    template<class T>
    std::string get_luaL_type_name(){
        // If by any chance you want to add lua_L to the unmangled type name, it should be done here.
        return get_type_name<T>();
    }

    struct user_data{
        void*       m_data;
        bool        m_garbage_collected;
        std::string m_type_name;

        static inline void push(lua_State* L, void* data, const char* type_name, bool garbage_collected = false){
            user_data** udata = (user_data**)lua_newuserdata(L, sizeof(user_data*));
            *udata = new user_data();
            (*udata)->m_data = data;
            (*udata)->m_type_name = type_name;
            (*udata)->m_garbage_collected = garbage_collected;

            luaL_getmetatable(L, type_name);
            lua_setmetatable(L, -2);
        }

        static inline user_data* check(lua_State* L, int index){
            return *(user_data**)lua_touserdata(L, index);
        }
    };




    template<class C>
    struct value<C&>{
        static C& check(lua_State* L, int index){
            user_data* udata = user_data::check(L, index);
            return *((C*)udata->m_data);
        }

        static int push(lua_State* L, C& value){
            user_data::push(L, &value, get_luaL_type_name<C>().c_str(), false);
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
            user_data::push(L, &value, get_luaL_type_name<C>().c_str(), false);
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
            user_data::push(L, value, get_luaL_type_name<C>().c_str(), false);
            return 1;
        }
    };
}

#endif
