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

#define LUA_BENDER_ERROR(msg) std::cerr << "[ERROR] " << std::endl\
    << "file : " << __FILE__ << std::endl\
    << "line " << __LINE__ << " in " << __func__ << "()"\
    << std::endl << msg << std::endl;\
    exit(EXIT_FAILURE);

#define LUA_BENDER_WARNING(msg) std::cerr << "[ERROR] " << std::endl\
    << "file : " << __FILE__ << std::endl\
    << "line " << __LINE__ << " in " << __func__ << "()"\
    << std::endl << msg << std::endl;


namespace lua_bender{
    // Next is a set of dispatchers for static, member functions and constructors.
    void erase_sub_strings(std::string& main, const std::string& sub){
        size_t pos = std::string::npos;

        while( (pos  = main.find(sub) )!= std::string::npos ){
            main.erase(pos, sub.length());
        }
    }

    template<class T>
    std::string get_type_name(){
        std::string name = typeid(T).name();
        // On MSVC at least the types are mangled with those prefixes.
        erase_sub_strings(name, "struct ");
        erase_sub_strings(name, "class ");
        // Finally remove all the namespaces.
        size_t index = name.find_last_of(':');
        if( index < name.size() ){
            return name.substr(index+1);
        }
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


    /**
     * The value template is used to both push and load values from the lua stack.
     * For now specializations are provided for the primitive c++ types.
     *
     * In the eventuallity
     * static T check(lua_State* L, int index){}
     * static int push(lua_State* L, const T& value){}
     */
    template<typename T>
    struct value{};

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

    template<class C>
    struct value<const std::vector<C>&>{

    };

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
    struct value<const char*>{
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



    struct lua_any_t{
        lua_Number            m_number;
        std::string           m_str;
        void*                 m_udata;
        lua_CFunction         m_func;
        int                   m_lua_type;

        lua_any_t(): m_number(), m_str(), m_udata(), m_lua_type(){}
    };

    std::ostream& operator<<(std::ostream& out, const lua_any_t& lua_any){
        switch(lua_any.m_lua_type){
            case LUA_TNUMBER:
                out << lua_any.m_number;
                break;
            case LUA_TSTRING:
                out << lua_any.m_str;
                break;
            case LUA_TUSERDATA:
                out << lua_any.m_udata;
                break;
        }
        return out;
    }

}

#pragma warning(pop)

#endif
