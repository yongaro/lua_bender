#ifndef LUA_BENDER_LIBRARY_HPP
#define LUA_BENDER_LIBRARY_HPP
#pragma once

#include "basis.hpp"
#include "functions.hpp"
#include "metatable.hpp"

namespace lua_bender{


    struct lua_library{
        std::unordered_map<std::string, const lua_metatable*> m_metatables_reg;
        std::unordered_map<std::string, luaL_Reg>       m_functions_reg;

        lua_library(): m_metatables_reg(), m_functions_reg(){}


        lua_library(const std::vector<lua_metatable*>& metatables, const std::vector<luaL_Reg>& functions): m_metatables_reg(), m_functions_reg(){
            for(const lua_metatable* table : metatables){
                if( table != nullptr){
                    m_metatables_reg.insert({table->get_name(), table});
                }
            }

            for(const luaL_Reg& func : functions){
                m_functions_reg.insert({func.name, func});
            }
        }

        void bind(lua_State* L) const{
            // Register all the metatables.
            for(const auto& entry : m_metatables_reg){
                entry.second->create_metatable(L);
            }

            // Register all global functions.
            for(const auto& entry : m_functions_reg){
                lua_bender::bind_function(L, entry.first.c_str(), entry.second.func);
            }
        }

        void set_metatable(const lua_metatable* table, const char* name){
            auto entry = m_metatables_reg.find(name);
            if( entry != m_metatables_reg.end() ){
                entry->second = table;
            }
            else{
                m_metatables_reg.insert({name, table});
            }
        }

        template<class C>
        void set_metatable(const lua_metatable* table){
            const char* name = lua_bender::user_data_type_name<C>::s_name.c_str();
            set_metatable(table, name);
        }


        void remove_metatable(const char* name){
             m_metatables_reg.erase(name);
        }

        template<class C>
        void remove_metatable(){
            const char* name = user_data_type_name<C>::s_name.c_str();
            remove_metatable(name);
        }

        void set_function(const luaL_Reg& reg){
            auto entry = m_functions_reg.find(reg.name);
            if( entry != m_functions_reg.end() ){
                entry->second = reg;
            }
            else{
                m_functions_reg.insert({reg.name, reg});
            }
        }

        void remove_function(const char* name){
             m_functions_reg.erase(name);
        }
    };

}


#endif
