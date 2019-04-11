/*
** RPI Game Architecture Engine
**
** Portions adapted from:
** Viper Engine - Copyright (C) 2016 Velan Studios - All Rights Reserved
**
** This file is distributed under the MIT License. See LICENSE.txt.
*/

#include "ga_lua_component.h"

#include "entity/ga_entity.h"
#include "framework/ga_frame_params.h"

#include <lua.hpp>

#include <cassert>
#include <iostream>
#include <string>

ga_lua_component::ga_lua_component(ga_entity* ent, const char* path) : ga_component(ent)
{
	// Create a new Lua state. The Lua library has no global variables,
	// so this state must be passed in as the first argument to all
	// other Lua library function calls
	_lua = luaL_newstate(); //new Lua state

	// Open all standard Lua libraries into the state
	luaL_openlibs(_lua);

	extern char g_root_path[256];
	std::string fullpath = g_root_path;
	fullpath += path;

	// Loads (but does not execute) a file as a Lua "chunk" (sequence of statements)
	int status = luaL_loadfile(_lua, fullpath.c_str());
	if (status)
	{
		std::cerr << "Failed to load script " << path << ": " << lua_tostring(_lua, -1);
		lua_close(_lua);
		_lua = nullptr;
		return;
	}

	// Loading the file will push an anonymous function to the top of the Lua stack
	// We (protected) call this function to execute the script and make the script's
	// functions and global variables available
	lua_pcall(_lua, 0, 0, 0);

	// Find the globally-accessible "update" function and push it on the stack
	lua_getglobal(_lua, "update");

	// The index of the top element in the stack. Lua indices start at 1 (!!) so
	// 0 means an empty stack
	int top = lua_gettop(_lua);
	if (top == 0 || lua_isfunction(_lua, top) == 0)
	{
		std::cerr << "Script " << path << " does not contain 'update' function." << std::endl;
		lua_close(_lua);
		_lua = nullptr;
		return;
	}

	// We were just checking to see if "update" existed. We don't want to execute
	// it right now.
	lua_pop(_lua, 1);

	// Set correspondences from function names in the Lua script to code defined here
	lua_register(_lua, "frame_params_get_input_left", lua_frame_params_get_input_left);
	lua_register(_lua, "frame_params_get_input_right", lua_frame_params_get_input_right);
	lua_register(_lua, "component_get_entity", lua_component_get_entity);
	lua_register(_lua, "entity_translate", lua_entity_translate);
}

ga_lua_component::~ga_lua_component()
{
	if (_lua)
	{
		lua_close(_lua);
	}
}

void ga_lua_component::update(ga_frame_params* params)
{
	if (_lua)
	{
		// Call the Lua script's update function. We put the function
		// on the stack, then the arguments in order (push the first 
		// argument first, etc.), then execute the function, specifying
		// the number of arguments, the number of results, and the
		// stack index of an error-catching function (0 for none)
		lua_getglobal(_lua, "update");
		// Lua has many push functions (boolean, string, etc.)
		// lua_pushlightdata is for pointers
		lua_pushlightuserdata(_lua, this);
		lua_pushlightuserdata(_lua, params);
		int status = lua_pcall(_lua, 2, 0, 0);
		if (status)
		{
			std::cerr << "Error: " << lua_tostring(_lua, -1);
			assert(0);
		}
	}
}

int ga_lua_component::lua_frame_params_get_input_left(lua_State* state)
{
	int arg_count = lua_gettop(state);
	if (arg_count == 1)
	{
		// get the pointer to the data at the given stack index
		ga_frame_params* params = (ga_frame_params*)lua_touserdata(state, 1);
		lua_pushboolean(state, (params->_button_mask & k_button_j) != 0);
	}
	else
	{
		std::cerr << "Function frame_params_get_input_left expected 1 argument but got " << arg_count << "." << std::endl;
	}
	return 1;
}

int ga_lua_component::lua_frame_params_get_input_right(lua_State* state)
{
	int arg_count = lua_gettop(state);
	if (arg_count == 1)
	{
		ga_frame_params* params = (ga_frame_params*)lua_touserdata(state, 1);
		lua_pushboolean(state, (params->_button_mask & k_button_l) != 0);
	}
	else
	{
		std::cerr << "Function frame_params_get_input_right expected 1 argument but got " << arg_count << "." << std::endl;
	}
	return 1;
}

int ga_lua_component::lua_component_get_entity(lua_State* state)
{
	int arg_count = lua_gettop(state);
	if (arg_count == 1)
	{
		ga_lua_component* component = (ga_lua_component*)lua_touserdata(state, 1);
		lua_pushlightuserdata(state, component->get_entity());
	}
	else
	{
		std::cerr << "Function component_get_entity expected 1 argument but got " << arg_count << "." << std::endl;
	}
	return 1;
}

int ga_lua_component::lua_entity_translate(lua_State* state)
{
	int arg_count = lua_gettop(state);
	if (arg_count == 4)
	{
		ga_entity* ent = (ga_entity*)lua_touserdata(state, 1);

		ga_vec3f vec;
		vec.x = (float)lua_tonumber(state, 2);
		vec.y = (float)lua_tonumber(state, 3);
		vec.z = (float)lua_tonumber(state, 4);

		ent->translate(vec);
	}
	else
	{
		std::cerr << "Function entity_translate expected 4 arguments but got " << arg_count << "." << std::endl;
	}
	return 0;
}
