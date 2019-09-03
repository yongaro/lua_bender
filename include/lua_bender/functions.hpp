#ifndef LUA_BENDER_FUNCTIONS_HPP
#define LUA_BENDER_FUNCTIONS_HPP
#pragma once

#include "basis.hpp"
#include "user_data.hpp"



/**
  * This file contains function adapters for both classic functions (i.e everything except non static members)
  * with the <function> class, and member functions with the <member_function> class.
  *
  *
  *
  *
  **/


// These macros conveniently wrap the two adapter templates and reduce the verbosity.
// Both the templates and macros return a lua_CFuntion and need to be used with a lua_Reg or push_function
// or the metatable struct to wrap a class member functions.
#define lua_bender_function(func, ret_type, ...)\
    lua_bender::function<ret_type(*)(__VA_ARGS__), func, ret_type, __VA_ARGS__>::adapter

#define lua_bender_member_function(C, Fn, RT, ...)\
    lua_bender::member_function<C, RT(C::*)(__VA_ARGS__), &C::Fn, RT, __VA_ARGS__>::adapter

#define lua_bender_const_member_function(C, Fn, RT, ...)\
    lua_bender::member_function<C, RT(C::*)(__VA_ARGS__) const, &C::Fn, RT, __VA_ARGS__>::adapter

namespace lua_bender{




    inline void bind_function(lua_State* L, const char* name, lua_CFunction func){
        lua_pushcfunction(L, func);
        lua_setglobal(L, name);
    }

    template<typename T>
    struct add_const_ref{
        typedef typename std::add_const<T>::type& type;
    };

    template<typename T>
    struct remove_const_ref{
        typedef typename std::remove_reference< typename std::remove_const<T>::type >::type type;
    };

    template<typename Fn, Fn func, typename R, typename ...Args>
    struct function{
        static int adapter(lua_State* L){
            int first_index = -1;
            return value< typename add_const_ref<R>::type >::push(L, func(value< typename add_const_ref<Args>::type >::check(L, first_index--)...));
        }
    };

    template<typename Fn, Fn func, typename ...Args>
    struct function<Fn, func, void, Args...>{
        static int adapter(lua_State* L){
            int first_index = -1;
            func(lua_bender::value< typename add_const_ref<Args>::type >::check(L, first_index--)...);
            return 0;
        }
    };





    template<class C, typename Fn, Fn func, typename R, typename ...Args>
    struct member_function{
        static int adapter(lua_State* L){
            user_data* udata = user_data::check(L, 1);
            if( udata == nullptr || udata->m_data == nullptr ){
                LUA_BENDER_ERROR("Could not get the caller from the lua stack");
            }

            C* caller = static_cast<C*>(udata->m_data);
            int first_index = -1;
            return value< typename add_const_ref<R>::type >::push(L, (caller->*func)(value< typename add_const_ref<Args>::type >::check(L, first_index--)...));
        }
    };


    template<class C, typename Fn, Fn func, typename ...Args>
    struct member_function<C, Fn, func, void, Args...>{
        static int adapter(lua_State* L){
            user_data* udata = user_data::check(L, 1);
            if( udata == nullptr || udata->m_data == nullptr ){
                LUA_BENDER_ERROR("Could not get a caller from the lua stack");
                return 0;
            }

            C* caller = static_cast<C*>(udata->m_data);
            int first_index = -1;
            (caller->*func)(value< typename add_const_ref<Args>::type >::check(L, first_index--)...);
            return 0;
        }
    };



    // Old snippet kept for the records because it's a little bit technical.
    // This versions used to differ the argument loading by building a tuple.
    // Eventually this was replaced by a direct due to some probleme loading strings and objects
    // which were flushed before being used.
    //    template<int ...>
    //    struct seq { };

    //    template<int N, int ...S>
    //    struct gens : gens<N-1, N-1, S...> { };

    //    template<int ...S>
    //    struct gens<0, S...> {
    //        typedef seq<S...> type;
    //    };

    //    template<int ...S>
    //    static R call_function(std::tuple<Args...>& args, lua_bender::seq<S...>){
    //         return func(std::get<S>(args) ...);
    //    }
}


#endif
