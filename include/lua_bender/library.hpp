#ifndef LUA_BENDER_LIBRARY_HPP
#define LUA_BENDER_LIBRARY_HPP
#pragma once

#include "basis.hpp"
#include "functions.hpp"
#include "metatable.hpp"

namespace lua_bender{
    struct lua_library{
        std::unordered_map<std::string, std::shared_ptr<LuaMetatable>> m_metatables_reg;
        std::unordered_map<std::string, lua_CFunction>                 m_functions_reg;

        lua_library(): m_metatables_reg(), m_functions_reg(){}

        void bind(lua_State* L){
            // Register all the metatables.
            for(const auto& entry : m_metatables_reg){
                entry.second->create_metatable(L);
            }

            // Register all global functions.
            for(const auto& entry : m_functions_reg){
                lua_bender::bind_function(L, entry.first.c_str(), entry.second);
            }
        }

        void set_metatable(const char* name, const std::shared_ptr<LuaMetatable> metatable){
            auto entry = m_metatables_reg.find(name);
            if( entry != m_metatables_reg.end() ){
                entry->second = metatable;
            }
            else{
                m_metatables_reg.insert({name, metatable});
            }
        }

        void remove_metatable(const char* name){
             m_metatables_reg.erase(name);
        }

        void set_function(const char* name, lua_CFunction function){
            auto entry = m_functions_reg.find(name);
            if( entry != m_functions_reg.end() ){
                entry->second = function;
            }
            else{
                m_functions_reg.insert({name, function});
            }
        }

        void remove_function(const char* name){
             m_functions_reg.erase(name);
        }
    };

}


#endif
