/*
** RPI Game Architecture Engine
**
** Portions adapted from:
** Viper Engine - Copyright (C) 2016 Velan Studios - All Rights Reserved
**
** This file is distributed under the MIT License. See LICENSE.txt.
*/

#include "ga_model_component.h"
#include "ga_material.h"
#include "math/ga_math.h"
#include "entity/ga_entity.h"
#include "graphics/ga_node.h"
#include "graphics/ga_mesh.h"
/* assimp include files. These three are usually needed. */

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/postprocess.h>     // Post processing flags

#define GLEW_STATIC
#include <GL/glew.h>
#include <iostream>
#include <cassert>
#include <string>


ga_model_component::ga_model_component(ga_entity* ent, const char* model_file) : ga_component(ent)
{
	extern char g_root_path[256];
	Assimp::Importer importer;
	std::string model_path = g_root_path;
	model_path += model_file;

	_scene = importer.ReadFile(model_path,
		aiProcess_CalcTangentSpace |
		aiProcess_Triangulate |
		aiProcess_JoinIdenticalVertices |
		aiProcess_SortByPType);

	if (!_scene)
	{
		std::cout << "error: couldn't load obj\n";
	}
	else
	{
		// process the scene
		std::cout << "number of meshes: " << _scene->mNumMeshes << std::endl;	

		// process the meshes
		for (int i = 0; i < _scene->mNumMeshes; i++)
		{
			ga_mesh* mesh = new ga_mesh();
			mesh->create_from_aiMesh(_scene->mMeshes[i], _scene);
			mesh->make_buffers();
			_meshes.push_back(mesh);
		}

		// recursively process the node tree
		process_node_recursive(_scene->mRootNode, _scene, &_root);

	}
}

ga_model_component::~ga_model_component()
{
	
}


void ga_model_component::process_node_recursive(aiNode* ai_node, const aiScene* scene, ga_node* node)
{
	// apply transformation

	ai_mat4_to_ga_mat4(ai_node->mTransformation, node->_transform);
	node->_transform.transpose();
	node->_name = ai_node->mName.C_Str();
	// process my meshes
	for (int i = 0; i < ai_node->mNumMeshes; i++)
	{
		node->_meshes.push_back(_meshes[ai_node->mMeshes[i]]);
	}

	// process my children
	for (int i = 0; i < ai_node->mNumChildren; i++)
	{
		ga_node* child_node = new ga_node(node);
		process_node_recursive(ai_node->mChildren[i], scene, child_node);
		node->_children.push_back(child_node);
	}
}

void ga_model_component::update(ga_frame_params* params)
{
	const float dt = std::chrono::duration_cast<std::chrono::duration<float>>(params->_delta_time).count();

	// update the world matrix for all nodes in the scenegraph
	_root.update(params, get_entity()->get_transform());

	// draw the scenegraph
	_root.draw_recursive(params);
}