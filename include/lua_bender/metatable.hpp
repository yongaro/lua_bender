#ifndef LUA_BENDER_METATABLE_HPP
#define LUA_BENDER_METATABLE_HPP
#pragma once


#include "basis.hpp"
#include "functions.hpp"
#include "user_data.hpp"
#include <memory>
#include <unordered_map>
#include <string>
#include <vector>

/** Macro shortcut to create a filled metatable and also register the type string name to the user data system. */
#define CREATE_LUA_BENDER_CLASS_METATABLE(type, type_name, table_var_name, ...)\
    REGISTER_LUA_BENDER_UDATA_TYPE_NAME(type, type_name);\
    const std::shared_ptr<const lua_metatable> table_var_name(new lua_class_metatable<type>({__VA_ARGS__}))

namespace lua_bender{
    struct lua_metatable{
        virtual ~lua_metatable(){}
        virtual void set_function(const char* name, lua_CFunction function) = 0;
        virtual void remove_function(const char* name) = 0;
        virtual void create_metatable(lua_State* L) const = 0;
        virtual const std::string& get_name() const = 0;
    };


    template<class C>
    struct lua_class_metatable : public lua_metatable{
        std::unordered_map<std::string, luaL_Reg> m_registry;

        lua_class_metatable():  m_registry(){}
        lua_class_metatable(int value_count, const luaL_Reg* reg): m_registry(){
            for(int i = 0; i < value_count; ++i){
                set_function(reg[i].name, reg[i].func);
            }
        }

        lua_class_metatable(const std::vector<luaL_Reg>& functions): m_registry(){
            for(const auto& reg : functions){
                set_function(reg.name, reg.func);
            }
        }

        template<typename ...Args>
        static int create_instance(lua_State* L){
            int first_index = 1;
            C* data = new C(value< typename add_const_ref<Args>::type >::check(L, first_index++)...);
            user_data::push(L, data, user_data_type_name<C>::s_name.c_str(), true);
            return 1;
        }


        static int destroy_instance(lua_State* L){
            user_data* udata = user_data::check(L, 1);
            if( udata != nullptr ){
                if( udata->m_garbage_collected && udata->m_data != nullptr ){
                    delete static_cast<C*>(udata->m_data);
                }
                delete udata;
            }
            return 0;
        }

        virtual void set(int value_count, const luaL_Reg* reg){
            m_registry.clear();

            for(int i = 0; i < value_count; ++i){
                set_function(reg[i].name, reg[i].func);
            }
        }

        virtual void set_function(const char* name, const lua_CFunction function){
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


        virtual void create_metatable(lua_State* L) const{
            // Build the registry given to the metatable.
            std::vector<luaL_Reg> regs;
            for(auto it : m_registry){
                regs.push_back(it.second);
            }
            regs.push_back({nullptr, nullptr});

            // Create a metatable.
            luaL_newmetatable(L, user_data_type_name<C>::s_name.c_str());

            luaL_setfuncs (L, regs.data(), 0);
            lua_pushvalue(L, -1);
            lua_setfield(L, -1, "__index");
            lua_setglobal(L, user_data_type_name<C>::s_name.c_str());
        }

        virtual const std::string& get_name() const{
            return lua_bender::user_data_type_name<C>::s_name;
        }
    };
}

#endif
