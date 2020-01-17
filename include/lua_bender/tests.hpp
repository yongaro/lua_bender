#ifndef LUA_BENDER_TESTS_HPP
#define LUA_BENDER_TESTS_HPP
#pragma once

#include "lua_bender.hpp"
#include <iostream>
#include <string>

namespace lua_bender{
    struct test_referenced_struct{
        void some_func(){
            std::cout << "This is some func from a struct not registered in lua." << std::endl;
        }
    };


    struct test_struct{
        std::string m_str_value;
        int         m_int_value;
        float       m_number_value;
        double      m_double_value;        


        test_struct(): m_str_value(), m_int_value(), m_number_value(){
            LUA_BENDER_LOG_INFO("Default constructor is called");
        }

        test_struct(const std::string& str, int i, float f): m_str_value(str), m_int_value(i), m_number_value(f){
            LUA_BENDER_LOG_INFO("Complete constructor is called : [\"%s\", %i, %f]", str.c_str(), i, f);
        }

        virtual ~test_struct(){
            LUA_BENDER_LOG_INFO("Destructor called");
        }

        static void s_function(){
            LUA_BENDER_LOG_INFO("This is test_struct s_function");
        }

        static int s_function2(float val1, double val2){
            std::cout << "This is test_struct s_function2 " << val1 << " "  << val2 << std::endl;
            return 42;
        }

        void set_str_value(const std::string& val){ m_str_value = val; }
        void set_int_value(int val){ m_int_value = val; }
        void set_number_value(float val){ m_number_value = val; }

        const std::string& get_str_value() const{ return m_str_value; }
        int get_int_value() const{ return m_int_value; }
        float get_number_value() const{ return m_number_value; }

        test_struct* test_create(){
            return new test_struct();
        }

        void test_delete(test_struct* val){
            if( val != nullptr ){
                delete val;
            }
        }

        test_struct& test_return_ref(){
            return *this;
        }

        void test_modify_ptr(test_struct* val){
            if( val != nullptr ){
                val->set_int_value( val->get_int_value() * 2 );
            }
        }

        void test_modify_ref(test_struct& val){
            val.set_int_value( val.get_int_value() * 2 );
        }

        void test_display_const_ptr(const test_struct* const val){
            if( val != nullptr ){
                std::cout << val->get_int_value() << std::endl;
            }
        }

        void test_get_referenced_data(test_referenced_struct* test){
            if( test != nullptr ){
                test->some_func();
            }
        }

        void test_get_referenced_data2(test_referenced_struct& test){
            test.some_func();
        }
    };

    template<typename T>
    T test_template(const T& val){
         std::cout << "C++ called from lua with : " << val << " ";
         return val;
    }




    const char* function_bindings_script = "print(\"Testing the template bindings.\")\n"
                                           "print(\"Lua test_template_int received : \" .. test_template_int(32)) \n"
                                           "print(\"Lua test_template_float received : \" .. test_template_float(64)) \n"
                                           "print(\"Lua test_template_str received : \" .. test_template_str(\"TEST STRING\")) \n"
                                           "print()\n";

    const char* class_binding_script = "do\n"

                                       "print(\"Testing the class test_struct bindings\") \n"

                                       "print(\"TESTING CONSTRUCTORS\")\n"
                                       "test_object = test_struct.new()\n"
                                       "test_object_2 = test_struct.new()\n"

                                       "print(\"TESTING MUTATORS\")\n"
                                       "test_object:set_int_value( test_struct.s_function2(45, 30) )\n"
                                       "test_object:set_number_value(53)\n"
                                       "test_object:set_str_value( \"This is another test string.\" )\n"
                                       "test_object_2:test_modify_ptr(test_object)\n"
                                       "test_object_2:test_modify_ref(test_object)\n"
                                       "test_struct.set_double_value(test_object, 66.6)\n"
                                       "test_struct.set(test_object_2, \"Object2\", 2, 4.0, 8.0)\n"

                                       "print(\"TESTING ACCESSORS\")\n"
                                       "print(test_object_2:get_int_value())\n"
                                       "print(test_object_2:get_number_value())\n"
                                       "print(test_object_2:get_str_value())\n"
                                       "print(test_object_2:test_return_ref())\n"
                                       "print(test_struct.get_double_value(test_object_2))\n"


                                       "print(\"DISPLAYING SOME GENERAL LUA INFOS\")"
                                       "print(getmetatable(test_object))\n"
                                       "print(test_struct.__name)\n"

                                       "print(\"TESTING THE RETURNED VALUES.\")"
                                       "return test_object, 13, 53, \"LOL\"\n"

                                       "end\n";


    lua_bender_register_user_data_name(test_struct, "test_struct");
    lua_bender_instantiate_initializer(test_struct, m_str_value, m_int_value, m_number_value, m_double_value);
    const std::shared_ptr<lua_metatable> test_struct_metatable(new lua_class_metatable<test_struct>({
                        {"new",              lua_class_metatable<test_struct>::create_instance<>},
                        {"__gc",             lua_class_metatable<test_struct>::destroy_instance},
                        {"set",              lua_bender_adapted_initializer(test_struct, m_str_value, m_int_value, m_number_value, m_double_value)},
                        {"set_str_value",    lua_bender_member_function(test_struct::set_str_value)},
                        {"set_int_value",    lua_bender_member_function(test_struct::set_int_value)},
                        {"set_number_value", lua_bender_member_function(test_struct::set_number_value)},

                        {"get_str_value",    lua_bender_member_function(test_struct::get_str_value)},
                        {"get_number_value", lua_bender_member_function(test_struct::get_number_value)},
                        {"get_int_value",    lua_bender_member_function(test_struct::get_int_value)},

                        {"test_modify_ptr",  lua_bender_member_function(test_struct::test_modify_ptr)},
                        {"test_modify_ref",  lua_bender_member_function(test_struct::test_modify_ref)},
                        {"test_return_ref",  lua_bender_member_function(test_struct::test_return_ref)},

                        {"s_function",       lua_bender_function(test_struct::s_function)},
                        {"s_function2",      lua_bender_function(test_struct::s_function2)},
                        {"get_double_value", lua_bender_generate_accessor(test_struct, m_double_value)},
                        {"set_double_value", lua_bender_generate_mutator(test_struct, m_double_value)}
                    }));


    const std::shared_ptr<lua_library> test_lib(new lua_library(
        {test_struct_metatable.get()},
        {
            {"test_template_int",       lua_bender::function<test_template<int>>::adapter},
            {"test_template_float",     lua_bender::function<test_template<float>>::adapter},
            {"test_template_str",       lua_bender::function<test_template<std::string>>::adapter},
            {"test_struct_s_function",  lua_bender::function<test_struct::s_function>::adapter},
            {"test_struct_s_function2", lua_bender::function<test_struct::s_function2>::adapter}
        }
    ));

    inline void launch_test(){
        // Creating the execution context.
        lua_State* L  = luaL_newstate();

        // Adding the libraries.
        luaL_openlibs(L);
        test_lib->bind(L);

        // Assembling and running code.
        std::string code = function_bindings_script;
        code += class_binding_script;
        lua_bender::script::do_string(L, code.c_str());

        // Accessing the results of the test script.
        std::vector<lua_any_t> res;
        lua_bender::lua_any_t::get_results(L, res);

        // Closing the context.
        lua_close(L);

        // Making sure the data has not been garbage collected and deleting it.
        std::cout << "Printing the results of the scripts" << std::endl;
        for(const lua_any_t& val : res){
            val.log_type_name();
            if( val.m_lua_type == LUA_TUSERDATA ){
                test_struct* data = static_cast<test_struct*>(val.m_udata);
                LUA_BENDER_LOG_INFO("test_struct : (\"%s\", %d, %f, %f)", data->get_str_value().c_str(), data->get_int_value(), data->get_number_value(), data->m_double_value);
                delete data;
            }
        }
    }
}

#endif
