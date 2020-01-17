#ifndef LUA_BENDER_FUNCTIONS_HPP
#define LUA_BENDER_FUNCTIONS_HPP
#pragma once

#include "basis.hpp"
#include "user_data.hpp"


// This file contains function adapters for both classic functions (i.e everything except non static members) and member functions.

// These macros conveniently wrap the two adapter templates and reduce the verbosity.
// Both the templates and macros return a lua_CFuntion and need to be used with a lua_Reg or push_function
// or the metatable struct to wrap a class member functions.

#define lua_bender_function(func) lua_bender::function<&func>::adapter
#define lua_bender_member_function(func) lua_bender::member_function<&func>::adapter

#define lua_bender_generate_accessor(type, member_name)\
    lua_bender::function<lua_bender::accessor<type, decltype(type::member_name), offsetof(type, member_name)>::get>::adapter

#define lua_bender_generate_mutator(type, member_name)\
    lua_bender::function<lua_bender::mutator<type, decltype(type::member_name), offsetof(type, member_name)>>::adapter


#ifndef LUA_BENDER_NO_BOOST
#include "../boost/preprocessor.hpp"
#include "../boost/preprocessor/punctuation/comma_if.hpp"
#include "../boost/preprocessor/seq/for_each_i.hpp"
#include "../boost/preprocessor/variadic/to_seq.hpp"

#define boost_decltype_member(r, data, index, elem) BOOST_PP_COMMA_IF(index) decltype(data::elem)
#define boost_decltype_const_member_ref(r, data, index, elem) BOOST_PP_COMMA_IF(index) const decltype(data::elem)&
#define boost_get_member_ptr(r, data, index, elem) BOOST_PP_COMMA_IF(index) &data::elem

#define lua_bender_initializer(type, ...)\
    lua_bender::initializer<type, BOOST_PP_SEQ_FOR_EACH_I(boost_get_member_ptr, type, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))>::init<BOOST_PP_SEQ_FOR_EACH_I(boost_decltype_member, type, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))>

#define lua_bender_instantiate_initializer(type, ...)\
    template void\
    lua_bender::initializer<type, BOOST_PP_SEQ_FOR_EACH_I(boost_get_member_ptr, type, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))>\
    ::init<BOOST_PP_SEQ_FOR_EACH_I(boost_decltype_member, type, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))>\
    (type&, BOOST_PP_SEQ_FOR_EACH_I(boost_decltype_const_member_ref, type, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)))

#define lua_bender_adapted_initializer(type, ...)\
    lua_bender::function<\
    lua_bender::initializer<type, BOOST_PP_SEQ_FOR_EACH_I(boost_get_member_ptr, type, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))>::init<BOOST_PP_SEQ_FOR_EACH_I(boost_decltype_member, type, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))>\
    >::adapter
#endif // LUA_BENDER_NO_BOOST

namespace lua_bender{
    // ******************************** HELPERS ********************************

    /** @brief Bind a global function under the given name to the lua context. */
    inline void bind_function(lua_State* L, const char* name, lua_CFunction func){
        lua_pushcfunction(L, func);
        lua_setglobal(L, name);
    }

    /** @brief Add both const and reference to the given type allowing T, T& and const T& to converge on the same template specialization. */
    template<typename T>
    struct add_const_ref{
        typedef typename std::add_const<T>::type& type;
    };


    // ******************************** CLASSIC FUNCTIONS ADAPTERS ********************************

    // This is the core of the function "conversion" to lua_CFunction system.
    // The main idea outlined here is to use the auto template from C++17 and specialization to deduce both arguments and return types
    // from any given function pointer.
    // The parameters deduced are then unpacked with a fold expression from C++17, and an integer sequence from C++14
    // in order to locate the argument in the Lua stack by index.
    // Finally each argument and return type get temporaries const ref qualifiers just to point them to the same value template implementation.

    // This template system is then specialized for 6 cases.
    // The first two are for classic function with and without returned value.
    // The last four after are the same than the previous two but for member functions with either a constant or not instance caller.


    template<auto Fn> struct function{};

    template<typename R, typename ...Args, R(*func)(Args...)>
    struct function<func>{
        template<int ...list>
        static int dispatch_function(lua_State* L, std::integer_sequence<int, list...>){
            return value< typename add_const_ref<R>::type >::push(L, func(value< typename add_const_ref<Args>::type >::check(L,  -sizeof...(Args) + list)...));
        }


        static int adapter(lua_State* L){
            return dispatch_function(L, std::make_integer_sequence<int, sizeof...(Args)>());
        }
    };

    template<typename ...Args, void(*func)(Args...)>
    struct function<func>{
        template<int ...list>
        static void dispatch_function(lua_State* L, std::integer_sequence<int, list...>){
            func(lua_bender::value< typename add_const_ref<Args>::type >::check(L, -sizeof...(Args) + list)...);
        }

        static int adapter(lua_State* L){
            dispatch_function(L, std::make_integer_sequence<int, sizeof...(Args)>());
            return 0;
        }
    };


    // ******************************** MEMBER FUNCTIONS ADAPTERS ********************************


   template<auto Fn> struct member_function{};

    template<class C, typename R, typename ...Args, R(C::*func)(Args...)>
    struct member_function<func>{
        template<int ...list>
        static int dispatch_function(lua_State* L, C* caller, std::integer_sequence<int, list...>){
            return value< typename add_const_ref<R>::type >::push(L, (caller->*func)(value< typename add_const_ref<Args>::type >::check(L, -sizeof...(Args) + list)...));
        }


        static int adapter(lua_State* L){
            user_data* udata = user_data::check(L, 1);
            if( udata == nullptr || udata->m_data == nullptr ){
                LUA_BENDER_LOG_ERROR("Could not get the caller from the lua stack");
            }

            C* caller = static_cast<C*>(udata->m_data);
            return dispatch_function(L, caller, std::make_integer_sequence<int, sizeof...(Args)>());
        }
    };


    template<class C, typename ...Args, void(C::*func)(Args...)>
    struct member_function<func>{
        template<int ...list>
        static void dispatch_function(lua_State* L, C* caller, std::integer_sequence<int, list...>){
            (caller->*func)(value< typename add_const_ref<Args>::type >::check(L, -sizeof...(Args) + list)...);
        }

        static int adapter(lua_State* L){
            user_data* udata = user_data::check(L, 1);
            if( udata == nullptr || udata->m_data == nullptr ){
                LUA_BENDER_LOG_ERROR("Could not get a caller from the lua stack");
                return 0;
            }

            C* caller = static_cast<C*>(udata->m_data);
            dispatch_function(L, caller, std::make_integer_sequence<int, sizeof...(Args)>());
            return 0;
        }
    };




    template<class C, typename R, typename ...Args, R(C::*func)(Args...) const>
    struct member_function<func>{
        template<int ...list>
        static int dispatch_function(lua_State* L, C* caller, std::integer_sequence<int, list...>){
            return value< typename add_const_ref<R>::type >::push(L, (caller->*func)(value< typename add_const_ref<Args>::type >::check(L, -sizeof...(Args) + list)...));
        }


        static int adapter(lua_State* L){
            user_data* udata = user_data::check(L, 1);
            if( udata == nullptr || udata->m_data == nullptr ){
                LUA_BENDER_LOG_ERROR("Could not get the caller from the lua stack");
            }

            C* caller = static_cast<C*>(udata->m_data);
            return dispatch_function(L, caller, std::make_integer_sequence<int, sizeof...(Args)>());
        }
    };


    template<class C, typename ...Args, void(C::*func)(Args...) const>
    struct member_function<func>{
        template<int ...list>
        static void dispatch_function(lua_State* L, C* caller, std::integer_sequence<int, list...>){
            (caller->*func)(value< typename add_const_ref<Args>::type >::check(L, -sizeof...(Args) + list)...);
        }

        static int adapter(lua_State* L){
            user_data* udata = user_data::check(L, 1);
            if( udata == nullptr || udata->m_data == nullptr ){
                LUA_BENDER_LOG_ERROR("Could not get a caller from the lua stack");
                return 0;
            }

            C* caller = static_cast<C*>(udata->m_data);
            dispatch_function(L, caller, std::make_integer_sequence<int, sizeof...(Args)>());
            return 0;
        }
    };

    /** @brief Generate a copy accessor to a data member of a given structure using pointer logic. */
    template<class C, typename mtype, int offset>
    struct accessor{
        static mtype get(C& obj){
            uint8_t* base_ptr = reinterpret_cast<uint8_t*>(&obj);
            mtype* member = reinterpret_cast<mtype*>(base_ptr + offset);
            return *member;
        }
    };

    /** @brief Generate a mutator to a data member of a given structure using pointer logic. */
    template<class C, typename mtype, int offset>
    void mutator(C& obj, const mtype& value){
        uint8_t* base_ptr = reinterpret_cast<uint8_t*>(&obj);
        // The is_polymorphic is a C++11 standard way to check for a vtable at the beginning of the object (size of a pointer).
        mtype* member = reinterpret_cast<mtype*>(base_ptr + offset);
        *member = value;
    }

    template <class C, typename T>
    auto get_member_ptr_type(T C::*v) -> const T&;


    template<class C, auto ...members> // The auto C::* pointer does not work on GCC use auto alone.
    struct initializer{
        template<typename... Args>
        static void init(C& obj, const Args&... values){
            auto set_func = [](C& obj_ref, auto C::*ptr, auto& val) -> void{
                obj_ref.*ptr = val;
            };

            (set_func(obj, members, values), ...);
        }
    };
}


#endif
