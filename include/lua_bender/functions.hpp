#ifndef LUA_BENDER_FUNCTIONS_HPP
#define LUA_BENDER_FUNCTIONS_HPP
#pragma once

#include "basis.hpp"
#include "user_data.hpp"



/**
  * This file contains function adapters for both classic functions (i.e everything except non static members)
  * with the <function> class, and member functions with the <member_function> class.
  */

// These macros conveniently wrap the two adapter templates and reduce the verbosity.
// Both the templates and macros return a lua_CFuntion and need to be used with a lua_Reg or push_function
// or the metatable struct to wrap a class member functions.
#ifdef __linux__
// On linux the __VA_ARGS__ doesn't remove the previous ',' if no argument were given thus explaining the
// "##" prefix on some of the __VA_ARGS__ use.

#define lua_bender_function(func, ret_type, ...)\
    lua_bender::function<ret_type(*)(__VA_ARGS__), func, ret_type, ##__VA_ARGS__>::adapter

#define lua_bender_member_function(C, Fn, RT, ...)\
    lua_bender::member_function<C, RT(C::*)(__VA_ARGS__), &C::Fn, RT, ##__VA_ARGS__>::adapter

#define lua_bender_const_member_function(C, Fn, RT, ...)\
    lua_bender::member_function<C, RT(C::*)(__VA_ARGS__) const, &C::Fn, RT, ##__VA_ARGS__>::adapter

#define lua_bender_generate_initializer(type, ...)\
    lua_bender::function<void(*)(type&, ##__VA_ARGS__), lua_bender::generic_set_object<type, ##__VA_ARGS__>, void, type&, ##__VA_ARGS__>::adapter

#else

#define lua_bender_function(func, ret_type, ...)\
    lua_bender::function<ret_type(*)(__VA_ARGS__), func, ret_type, __VA_ARGS__>::adapter

#define lua_bender_member_function(C, Fn, RT, ...)\
    lua_bender::member_function<C, RT(C::*)(__VA_ARGS__), &C::Fn, RT, __VA_ARGS__>::adapter

#define lua_bender_const_member_function(C, Fn, RT, ...)\
    lua_bender::member_function<C, RT(C::*)(__VA_ARGS__) const, &C::Fn, RT, __VA_ARGS__>::adapter

#define lua_bender_generate_initializer(type, ...)\
    lua_bender::function<void(*)(type&, __VA_ARGS__), lua_bender::generic_set_object<type, __VA_ARGS__>, void, type&, __VA_ARGS__>::adapter

#endif

// These other macros shorten the definition accessors and mutators for a given class member
#define lua_bender_generate_accessor(type, member_type, member_name)\
    lua_bender::function<member_type(*)(type&), lua_bender::accessor<type, member_type, offsetof(type, member_name)>::get, member_type, type&>::adapter

#define lua_bender_generate_mutator(type, member_type, member_name)\
    lua_bender::function<void(*)(type&, const member_type&), lua_bender::mutator<type, member_type, offsetof(type, member_name)>, void, type&, const member_type&>::adapter

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

    template<typename T>
    void generic_set(uint8_t*& ptr, const T& value){
        T* member = reinterpret_cast<T*>(ptr);
        *member = value;
        ptr += sizeof(T);
    }

    /** @brief This function is a simple generator for a complete set of the object. */
    template<class C, typename ...Args>
    void generic_set_object(C& obj, Args... args){
        uint8_t* base_ptr = reinterpret_cast<uint8_t*>(&obj);
        // The is_polymorphic is a C++11 standard way to check for a vtable at the beginning of the object (size of a pointer).
        base_ptr += std::is_polymorphic<C>::value ? sizeof(uint8_t*) : 0;
        (generic_set<Args>(base_ptr, args), ...);
    }

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
        mtype* member = reinterpret_cast<mtype*>(base_ptr + offset);
        *member = value;
    }
}


#endif
