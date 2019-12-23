# Lua Bender

## **Description**

Lua bender is a header only library that helps generate bindings of C++ functions and classes to Lua.

This project provides templates and macros to handle the automatic generation of lua_CFunction and optionnal object oriented structures to wrap up the Lua C API concepts.


## **Introduction**

Lua is a very lightweight and flexible language that can be easily embedded in a C/C++ application.


Indeed the Lua API is an ANSI C small library, in size, with no third party dependency.  
This compact and standard design allows to integrate it without modification into any project architecture and on any platform with an ANSI C compiler.

However this API has some drawbacks that can become a real problem quite early in a project developpement.

The main downside is the generic function binding systems.  
To boldly sum up the process, the Lua API only register C functions with the following signature.
> ```cpp
> typedef int (*lua_CFunction) (lua_State *L);
> ```

This means that any given C/C++ function must be **manually** wrapped by a lua_CFunction
that will handle, argument loading from the Lua state, function dispatching, and result transmission back to Lua if necessary.

> ```cpp
> // Some random no brain operation
> double some_func(double arg1, int arg2){
>     return arg1 + arg2;
> }
>
> int some_func_binding(lua_State* L){
>     // Lua stores arguments and caller in a stack accessible with indices.
>     // These indices starts either with -1 (from the top) or 1 (from the bottom).
>     double a1  = luaL_checknumber(L, -2);
>     int    a2  = luaL_checkinteger(L, -1); // Last argument is on top.
>     double res = some_func(a1, a2);
>     lua_pushnumber(L, res);
>     return 1; // The number of returned values.
> }
>
> 
> luaL_Reg binding = {"some_func", some_func_binding};
> ```

The **luaL_Reg** is then given to the Lua API and **some_func** can be called normally from Lua.
> ```lua
> local res = some_func(24, 10)
> ```

As is, this approach involves a constantly growing redundancy in the code base that obviously make the project hard to modify or maintain.

This problem has already a lot of different solutions that can be found [here](http://lua-users.org/wiki/BindingCodeToLua).  
However when writing this project and description, many of the above mentionned projects have been left aside for several years.  
Some of them are actively maintained and highly efficient but totally wrap and overhaul the original C API.

This projects aims to round the edges of the Lua API and stick to it closely by keeping most of its components optionals, while trying to keep some decent performance and a complete set of features.  
That way it may be used entirely or partially in more demanding projects or at least for educationnal purposes.

## **Dependencies**

Lua 5.3.5 and C++ 17 are required.

## **Documentation and examples**

To begin using the library, first include lua_bender.hpp

 > ```cpp
 > // This header also takes care of including lua.hpp
 > #include <lua_bender/lua_bender.hpp>
 > ```

A complete and easy to use set of examples can be found and executed from the **test.hpp** header (Lua code is also there for convenience).

> ```cpp
> #include <lua_bender/tests.hpp>
>
> int main(){
>     lua_bender::launch_test();
>     return 0;
> }
> ```

### **1. Function bindings**

#### **a) Classic functions**

There is two ways to bind both classic functions and member functions, and the first is to directly use the template api as follow.

> ```cpp
> // Example of C++ function
> template<typename T>
> T test_template(const T& val){
>     std::cout << "C++ called from lua with : " << val << " ";
>     return val;
> }
>
> // The function adapter template will provide a lua_CFunction but requires
> // all these parameters in the right order.
> // 1. The function pointer type.
> // 2. The function address.
> // 3. The return type (primitive type, custom types or void).
> // 4. A variadic list of arguments types in order.
> // Of course the test_template here is just an example.
> lua_CFunction f =  lua_bender::function<int(*)(const int&), test_template<int>, int, const int&>::adapter
> ```

This approach exposes already two drawbacks, it remains quite technical since it forces the user to know the underlying function pointer systems, and it is quite verbose.

This is why a set of macros are provided which only use function addresses and its signature as input.  
Applying the macro system to the previous example we obtain the following.

> ```cpp
> lua_CFunction f = lua_bender_function(test_template<int>, int, const int&);
> ```

#### **b) Member functions**

Member functions use the same logic with another template system that will also load the caller instance from Lua user data.

> ```cpp
> struct test_struct{
>     std::string m_str_value;
>     int         m_int_value;
>     float       m_number_value;
>     double      m_double_value;
> 
>     void  some_func_1(int val){ /* Some code */ }
>     float some_func_2() const { return 42.0f; }
>     static void some_static_func(float val1, double val2){ /* Some code */ }
> };
>
> // Using the template API
> // 1. The structure/class type.
> // 2. The function pointer type.
> // 3. The function address.
> // 4. The return type (primitive type, custom types or void).
> // 5. A variadic list of arguments types in order.
> // Of course the test_template here is just an example.
> lua_CFunction tp_f1 = lua_bender::member_function<test_struct, void(test_struct::*)(int), &test_struct::some_func1, void, int>
>
> // Using the macros
> lua_CFunction f1 = lua_bender_member_function(test_struct, some_func_1, void, int);
> lua_CFunction f2 = lua_bender_const_member_function(test_struct, some_func_2, float);
>
> // Static functions fit the classic function category
> lua_CFunction sf1 = lua_bender_function(test_struct::some_static_func, void, float, double);
> ```

This part of the API can be used alone as is to build your own class bindings, but Lua Bender also provides helpers to wrap some more concepts of the C API that we will describe in the following part.



### **2. Metatables and user data**

Lua offers **metatables** to customize the behavior of its data structures.  
As the name may have hinted, these tables are function collections bound to Lua variables.  

From the Lua side, this allows for instance to customize classic **tables** behaviors, and get some object oriented programming working.  

On the C/C++ side of things, **metatables** can be combined with another Lua data type called **user data**.  
These last are a way to pass pointers to some data to the Lua language and, using a metatable containing member functions bindings,
conveniently create, use and garbage collect C/C++ data structures.

Lua Bender thus provides several temaplates for both of them and some helpers.  

  - The **user_data** class is the wrapper itself.  
    This template expends the previous function binding system and allows the use of other types than the primitive ones for both parameters and return values.  
    It can be used manually like this.  
    > ```cpp
    > lua_State* L = luaL_newstate();
    > // Pushing some new user data.
    > test_struct* struct_a = new test_struct();
    >
    > // Pushing the struct as is to Lua (no metatable, no garbage collection).
    > lua_bender::user_data::push(L, struct_a);
    > // or with a metatable named "test_struct" with garbage collection turned off
    > lua_bender::user_data::push(L, struct_a, "test_struct", false);
    >
    > /* ... Running a Lua script that creates and returns a test_struct ... */
    >
    > // Accessing the user data just returned and thus located at the top (index -1).
    > lua_bender::user_data* udata = lua_bender::user_data::check(L, -1);
    > test_struct* struct_b = (test_struct*)udata->m_data;
    >
    > lua_close(L);
    > // Past this point the created test_struct will still be valid if the garbage collection stays disabled.
    > // However the user_data will ALWAYS be destroyed and should never be stored for later use.
    > ```

  - This second template is a helper that provides a static string name for every C/C++ type defined by the user.  
    Indeed, Lua identifies types with string names and Lua bender handles this need with the **user_data_type_name** class.  
    The **REGISTER_LUA_BENDER_UDATA_TYPE_NAME** macro is a shorthand for both defining the template for a given type, and setting the name.  
    Pairing a class to its Lua name is done out of any function body like this.  
      > ```cpp
      > template<> std::string lua_bender::user_data_type_name<test_struct>::s_name = "test_struct";
      > // or for short
      > REGISTER_LUA_BENDER_UDATA_TYPE_NAME(test_struct "test_struct");
      > ```

    And in C/C++ pushing manually a **user_data** with the according metatable name is done as follow.  

      > ```cpp
      > // There is a false default value to the last garbage collection argument.
      > lua_bender::user_data::push(L, struct_a, user_data_type_name<test_struct>::s_name.c_str());
      > ```


- Finally for **metatables**, the **lua_class_metatable** is provided to cover the redundancy of binding the table to a given Lua state.  
  
  This template is an unordered_map of **luaL_Reg**, which main goal is to keep efficiently a collection of member and static functions bindings, for a given C++ class.  
  This structure also provides a wrapping system for custom constructors and destructors that can be used directly for the **"new"** and **"__gc"** slots of the table.  
  The C/C++ data structures created from Lua using this sytem will then be automatically garbage collected unless specified otherwise.  
  
  Here is an example of binding for the previously defined **test_strust**.
    > ```cpp
    > // Set the Lua name of the test_struct type to "test_struct".
    > REGISTER_LUA_BENDER_UDATA_TYPE_NAME(test_struct, "test_struct");
    > // Fill the metatable with some functions bindings.
    > const std::shared_ptr<lua_metatable> test_struct_metatable(
    >     new lua_class_metatable<test_struct>({
    >         // Examples of constructor and destructor
    >         {"new",  lua_class_metatable<test_struct>::create_instance<>},
    >         {"__gc", lua_class_metatable<test_struct>::destroy_instance},
    >         // Example of function bindings (member and static).
    >         {"some_func_1", lua_bender_member_function(test_struct, some_func_1, void, int)},
    >         {"some_func_2", lua_bender_const_member_function(test_struct, get_str_value, float)},
    >         {"static_function", lua_bender_function(test_struct::static_function, int, float, double)}
    >     })
    > );
    >
    > lua_State* L = luaL_newstate();
    >
    > // Finally create the metatable for the given state.
    > mtable->create_metatable(L);
    > ```

  Which then allows to do in Lua.

    > ```lua
    > local var = test_struct.new()
    > var:some_func_1(42)
    > -- Of course no need to call the __gc destructor who will be used automatically once the state is close if needed.
    > ```

  If by any mean this structure doesn't meet your needs, the **lua_metatable** interface defines the mendatory services that any implementation must provide in order to work with other components from this API.  

### **3. Accessors and mutators generators**

Direct access to any data member of a C/C++ structure in Lua is impossible.  
In the same way there doesn't seem to be as much flexibility in Lua for setting whole or parts of a given object as there is in C++.  

Solving this problem can be done simply by creating the corresponding "get" and "set" functions and binding them to lua using the previous mechaninics.  
However some may want to keep their code clean of those potentially useless functions for their C/C++ API.  

As an experiment, a set of 3 macros using templates is provided to generate either accessor, mutator or initializers.  
Using pointer arithmetic, these new functions, available only to Lua, aim to improve greatly the flexibility of the Lua side while keeping the C/C++ API concise.  

These macros can be use as following to complete the previous test_struct binding example.  
> ```cpp
> REGISTER_LUA_BENDER_UDATA_TYPE_NAME(test_struct, "test_struct");
> const std::shared_ptr<lua_metatable> test_struct_metatable(
>     new lua_class_metatable<test_struct>({
>         // ... All the previous bindings ...
>         // An example of complete data set.
>         {"set", lua_bender_generate_initializer(test_struct, std::string, int, float, double)},
>         // And here is a partial one.
>         {"set_part", lua_bender_generate_initializer(test_struct, std::string, int)},
>         // Now the accessor and mutator for the m_double_value.
>         {"get_double_value", lua_bender_generate_accessor(test_struct, double, m_double_value)},
>         {"set_double_value", lua_bender_generate_mutator(test_struct, double, m_double_value)}
>     })
> );
> ```

Which then give later in Lua.

> ```lua
> local var = test_struct.new()
> test_struct.set(var, "FINDING_NAMES_IS_BORING", 2, 4.0, 8.0)
> test_struct.set_double_value(var, 42.0)
> print(test_struct.get_double_value(var))
> ```

### **4. Lua library**

The **lua_library** structure have two **unordered_map** of **lua_metatable** and **LuaL_Reg**.  
A very basic way to store a set of bindings and share it with differents scripts using the same logic as before.  

Some quick function wrapper are also available in the **script** class to run code from a file or a string.

Here is an example using the previously defined metatable and the template function used in the **1.a** section.

> ```cpp
> const std::shared_ptr<lua_library> test_lib(new lua_library(
>     // First using a list of all the previously defined pointers to the existing metatables.
>     {test_struct_metatable.get()},
>     // Finally additional independent functions can be added here
>     {
>         {"test_template_int", lua_bender_function(test_template<int>, int, const int&)},
>         {"test_template_float", lua_bender_function(test_template<float>, float, const float&)},
>         {"test_template_str",   lua_bender_function(test_template<std::string>, std::string, const std::string&)}
>     }
> ));
>
> void doing_some_lua(){
>     lua_State* L  = luaL_newstate();
>
>     // Binding the libraries, should be done only once per state.
>     luaL_openlibs(L);
>     test_lib.bind(L);
>
>     // Run some code.
>     const char* lua_code = "print(\"THIS IS SOME LUA CODE\")";
>     const char* lua_file = "some/path/file.lua";
>     lua_bender::script::do_string(lua_code);
>     lua_bender::script::do_file(lua_filer);
>
>     lua_close(L);
> }
> ```

### **4. Lua any type**

The **lua_any_t** type is a generic way to access data from a **Lua** stack.  
The main goal behind this structure is to provide an easy way to keep all kind of data returned by a script in the same collection.

This structures also allows to keep without deep copy the **user data** allocated from the lua side by simply passing the wrapped data to the caller and disabeling the garbage collection for this variable.

> ```lua
> do
> test_object = test_struct.new()
> return test_object, 42, 53, "res string"
> end
> ```

> ```cpp
> // Create the context
> lua_State* L = luaL_newstate();
>
> // ... Bind the test_struct and run the above script ...
>
> // Recovering the results
> std::vector<lua_any_t> res;
> lua_any_t::get_results(L, res);
>
> // Close the context.
> lua_close(L);
>
> // Memory is not released past this point and memory managment responsability gets back to the user.
> ```

### **5. Complete test**

All those concepts are implemented and easily executable from the **test.hpp** header.  
Further documentation can be found in the other headers for more in depth under


## **License**

This project is distributed under the **Apache License 2.0** see the complete text in the **LICENSE** text file for more information.

## **Bonus for CMake beginners**

As of **Lua 5.3.5**, the C API comes with only support for the **GNU Make** build system which is quite inconvenient for cross-platform projects.

Below is quick and dirty **CMakeLists.txt** for the beginners who would want a quick start with a Lua static build using CMake.  

> ```CMake
> cmake_minimum_required(VERSION 3.12)
>
> if(UNIX)
>     add_compile_definitions(LUA_USE_POSIX)
> endif()
>
> file(GLOB_RECURSE SRC_LIB src/*.c)
> add_library(liblua_static STATIC ${SRC_LIB})
> ```

The Lua C source code will comme with the following architecture : 

root /
- doc / 
- src /
- Makefile
- README

Simply put this script to the root directory and add it to your main project along with the **src** to your include system.

> ```CMake
> set(LUA_ROOT "${THIRD_PARTIES_ROOT}/lua-5.3.5")
> add_subdirectory("${LUA_ROOT}")
> include_directories("${LUA_ROOT}/src")
> ```

## **Other projects and contact**

If this library was of some use, you may be interested in some of my others projects described on my github page, along with informations to contact me if necessary.  

[https://yongaro.github.io/](https://yongaro.github.io/)
