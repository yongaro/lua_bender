#ifndef LUA_BENDER_METATABLE_HPP
#define LUA_BENDER_METATABLE_HPP
#pragma once


#include "basis.hpp"
#include "functions.hpp"
#include "user_data.hpp"
#include <memory>
#include <unordered_map>
#include <string>
#include <iostream>
#include <sstream>


namespace lua_bender{
    struct LuaMetatable{
        virtual void set_function(const char* name, lua_CFunction function) = 0;
        virtual void remove_function(const char* name) = 0;
        virtual void create_metatable(lua_State* L) = 0;
    };


    template<class T>
    struct LuaClassMetatable : public LuaMetatable{
        std::unordered_map<std::string, luaL_Reg> m_registry;

        LuaClassMetatable(): m_registry(){}

        template<typename ...Args>
        static int create_instance(lua_State* L){
            std::string type_name = get_luaL_type_name<T>();
            int first_index = 1;
            T* data = new T(value< typename remove_const_ref<Args>::type >::check(L, first_index++)...);
            user_data::push(L, data, type_name.c_str(), true);
            return 1;
        }


        static int destroy_instance(lua_State* L){
            user_data* udata = user_data::check(L, 1);
            if( udata != nullptr ){
                if( udata->m_garbage_collected && udata->m_data != nullptr ){
                    delete (T*)udata->m_data;
                }
                delete udata;
            }
            return 0;
        }

        virtual void set_function(const char* name, lua_CFunction function){
            auto entry = m_registry.find(name);
            if( entry != m_registry.end() ){
                entry->second = {name, function};
            }
            else{
                m_registry.insert({name, {name, function}});
            }
        }

        virtual void remove_function(const char* name){
             m_registry.erase(name);
        }


        virtual void create_metatable(lua_State* L){
            // Build the registry given to the metatable.
            std::vector<luaL_Reg> regs;
            for(auto it : m_registry){
                regs.push_back(it.second);
            }
            regs.push_back({NULL, NULL});

            // Create a metatable named "luaL"
            std::string table_name = get_luaL_type_name<T>();
            luaL_newmetatable(L, table_name.c_str());

            luaL_setfuncs (L, regs.data(), 0);
            lua_pushvalue(L, -1);
            lua_setfield(L, -1, "__index");
            lua_setglobal(L, get_type_name<T>().c_str());
        }
    };
}

#endif
