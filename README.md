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
>     double a1  = luaL_checknumber(L, 1);
>     int    a2  = luaL_checkinteger(L, 2);
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

 ```cpp
 // This header also takes care of including lua.hpp
 #include <lua_bender/lua_bender.hpp>
 ```

A complete and easy to use set of examples can be found and executed from the **test.hpp** header (Lua code is also there for convenience).

```cpp
#include <lua_bender/tests.hpp>

int main(){
    lua_bender::launch_test();
    return 0;
}
```

### **1. Function bindings**

#### **a) Classic functions**

There is two ways to bind both classic functions and member functions.  
The first is to directly use the template api as follow.

```cpp
// Example of C++ function
template<typename T>
T test_template(const T& val){
    std::cout << "C++ called from lua with : " << val << " ";
    return val;
}

// The function adapter template will provide a lua_CFunction but requires
// all these parameters in the right order.
// 1. The function pointer type.
// 2. The function address.
// 3. The return type (primitive type, custom types or void).
// 4. A variadic list of arguments types in order.
// Of course the test_template here is just an example.
lua_CFunction f =  lua_bender::function<int(*)(const int&), test_template<int>, int, const int&>::adapter
```

This approach exposes already two drawbacks, it remains quite technical since it forces the user to know the underlying function pointer systems, and it is quite verbose.

This is why a set of macros are provided which only use function addresses and its signature as input.  
Applying the macro system to the previous example we obtain the following.

```cpp
lua_CFunction f = lua_bender_function(test_template<int>, int, const int&);
```

#### **b) Member functions**

Member functions use the same logic with another template system that will also load the caller instance from Lua user data.

```cpp
struct test_struct{
    void  some_func_1(int val){ /* Some code */ }
    float some_func_2() const { return 42.0f; }
    static void some_static_func(int val){ /* Some code */ }
};

// Using the template API
// 1. The structure/class type.
// 2. The function pointer type.
// 3. The function address.
// 4. The return type (primitive type, custom types or void).
// 5. A variadic list of arguments types in order.
// Of course the test_template here is just an example.
lua_CFunction tp_f1 = lua_bender::member_function<test_struct, void(test_struct::*)(int), &test_struct::some_func1, void, int>

// Using the macros
lua_CFunction f1 = lua_bender_member_function(test_struct, some_func_1, void, int);
lua_CFunction f2 = lua_bender_const_member_function(test_struct, some_func_2, float);

// Static functions fit the classic function category
lua_CFunction sf1 = lua_bender_function(test_struct::some_static_func, void, int);
```

This part of the API can be used alone as is to build your own class bindings, but Lua Bender also provides helpers to wrap some more concepts of the C API that we will describe in the following part.

### **2. Metatables and user data**

Lua offers **metatables** to customize the behavior of its data structures.  
As the name may have hinted, these tables are function collections bound to Lua variables.  

From the Lua side, this allows for instance to customize classic **tables** behaviors, and get some object oriented programming working.

On the C/C++ side of things, **metatables** can be combined with another Lua data type called **user data**.  
These last are a way to pass pointers to some data to the Lua language and, using a metatable containing member functions bindings,
conveniently create, use and garbage collect C/C++ data structures.

Lua Bender implements shorthands for both **metatables** and **user data** to support C/C++ manipulation from Lua.

- For **metatables** this API covers the redundancy of creating the table and keeping a lua_CFunction registry efficiently.  
There is also generic constructors and destructors for C/C++ data that fit the **new** and **__gc** (garbage collection) slots of the table.  
  ```cpp
    // Using the previously defined test_struct.
    // The LuaMetatable is the basic interface that allows different implementations and storing in collections
    // (arrays, vectors, ...).
    lua_bender::LuaMetatable* mtable = new lua_bender::LuaClassMetatable<test_struct>();
    lua_State* L = luaL_newstate();

    // Set both creation, garbage collection and some functions.
    mtable->set_function("new",  lua_bender::LuaClassMetatable<test_struct>::create_instance);
    mtable->set_function("__gc", lua_bender::LuaClassMetatable<test_struct>::destroy_instance);

    mtable->set_function("some_func_1", lua_bender_member_function(test_struct, some_func_1, void, int));
    mtable->set_function("some_func_2", lua_bender_const_member_function(test_struct, some_func_2, float);

    // Finally create the metatable for the given state.
    mtable->create_metatable(L);
  ```

  Which allows to do in Lua

  ```lua
    local var = test_struct.new()
    var:some_func_1(42)
    -- Of course no need to call the __gc destructor who will be used automatically once the state is close if needed.
  ```

- **User data** is implemented as a pointer wrapper with control over the Lua garbage collection.  
  A set of helper functions is also available to push or access data.  
  It is however **mendatory** for to create new **user data** to have a corresponding metatable registered in the Lua state  
  (by using Lua Bender or any other mean).
  ```cpp
    lua_State* L = luaL_newstate();
    // Pushing some new user data.
    test_struct* struct_a = new test_struct();
    // The required metatable mentionned above is searched in the Lua state by name.
    // For this example the API name format is used but of course any custom implementation name system will work.
    lua_bender::user_data::push(L, struct_a, lua_bender::get_luaL_type_name<test_struct>());

    /* .. Running a Lua script that returns a test_struct. */

    // Accessing the user data just returned and thus located at index 1.
    lua_bender::user_data* udata = lua_bender::user_data::check(L, 1);
    test_struct* struct_b = (test_struct*)udata->m_data;
    // If the struct_b pointer must remain valid after the script lifespan,
    // the garbage collection for this user data must be disabled.
    udata->m_garbage_collected = false;

    lua_close(L);
    // Past this point struct_a and struct_b are still valid and up to date.
  ```

  As shown above, the **user data** wrapper is  **ALWAYS** destroyed when the lua_State is closed, but the data **is not** garbage collected by default if created from C/C++ instead of Lua, or if it is manually specifyed to the API of course.  


### **3. Lua library**

The **lua_library** structure is a collection of **metatables** and **lua_CFunction** organized by names.  
A very basic way to store a set of bindings and share it with differents scripts.

Some quick function wrapper are also available in script.hpp to run code from a file or a string.

```cpp
// Creating the library and adding some function.
lua_bender::lua_library test_lib;
test_lib.set_function("some_func", lua_bender_function(test_template<int>, int, const int&));

// Adding a metatable for the previously defined test_struct.
std::shared_ptr<LuaMetatable> metatable = std::make_shared<LuaClassMetatable<test_struct>>();
// ... Add some functions to the metatable ...
test_lib.set_metatable(get_luaL_type_name<test_struct>().c_str(), test_table_ptr);

// Creating the execution context.
lua_State* L  = luaL_newstate();

// Binding the libraries, should be done only once per state.
luaL_openlibs(L);
test_lib.bind(L);

// Run some code.
const char* lua_code = "print(\"THIS IS SOME LUA CODE\")";
const char* lua_file = "some/path/file.lua";
lua_bender::script::do_string(lua_code);
lua_bender::script::do_file(lua_filer);

// Close the context.
lua_close(L);
```  

### **4. Lua any type**

The **lua_any_t** type is a generic way to access data from a **Lua** stack.  
The main goal behind this structure is to provide an easy way to keep all kind of data returned by a script in the same collection.

This structures also allows to keep without deep copy the **user data** allocated from the lua side by simply passing the wrapped data to the caller and disabeling the garbage collection for this variable.

```lua
do
test_object = test_struct.new()
return test_object, 42, 53, "res string"
end
```

```cpp
// Create the context
lua_State* L = luaL_newstate();

// ... Bind the test_struct and run the above script ...

// Recovering the results
std::vector<lua_any_t> res;
lua_any_t::get_results(L, res);

// Close the context.
lua_close(L);

// Results are still valid past this point.
```

### **5. Complete test**

All those concepts are implemented and easily executable from the **test.hpp** header.  
Further documentation can be found in the other headers for more in depth under


## **License**

This project is distributed under the **Apache License 2.0** see the complete text in the **LICENSE** text file for more information.