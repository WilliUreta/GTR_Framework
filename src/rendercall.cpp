#include "rendercall.h"

#include "framework.h"
#include "camera.h"
#include "shader.h"
#include "mesh.h"
#include "texture.h"
#include "prefab.h"
#include "material.h"
#include "utils.h"
#include "scene.h"
#include "extra/hdre.h"

#include <algorithm>	//Afegit per mi



using namespace GTR;

/*
GTR::RenderCall::RenderCall()
{
	//this->NodeVec = 
}*/

/*
GTR::RenderCall::RenderCall()
{

	//this->renderCall_data = NULL;
}*/

void GTR::RenderCall::saveScene(GTR::Scene* scene, Camera* camera)
{
	for (int i = 0; i < scene->entities.size(); ++i)
	{
		BaseEntity* ent = scene->entities[i];
		if (!ent->visible)
			continue;

		//is a prefab!
		if (ent->entity_type == PREFAB)
		{
			PrefabEntity* pent = (GTR::PrefabEntity*)ent;
			if (pent->prefab)
				savePrefab(ent->model, pent->prefab, camera);
		}
	}
}


void GTR::RenderCall::savePrefab(const Matrix44& model, GTR::Prefab* prefab, Camera* camera)
{
	assert(prefab && "PREFAB IS NULL");
	//assign the model to the root node
	saveNode(model, &prefab->root, camera);
}


void GTR::RenderCall::saveNode(const Matrix44& prefab_model, GTR::Node* node, Camera* camera)
{
	if (!node->visible)
		return;

	//compute global matrix
	Matrix44 node_model = node->getGlobalMatrix(true) * prefab_model;

	//does this node have a mesh? then we must render it
	if (node->mesh && node->material)
	{
		//compute the bounding box of the object in world space (by using the mesh bounding box transformed to world space)
		BoundingBox world_bounding = transformBoundingBox(node_model, node->mesh->box);

		//if bounding box is inside the camera frustum then the object is probably visible
		if (camera->testBoxInFrustum(world_bounding.center, world_bounding.halfsize))
		{

			//render node mesh
			saveMeshWithMaterial(node_model, node->mesh, node->material, camera);
			//node->mesh->renderBounding(node_model, true);

			//cal temp_data? repassar instanciacio de structs, punters etc............
			
			float dist = camera->eye.distance(world_bounding.center);

			data temp_data = { node_model, node, dist};
			this->renderCall_data.push_back(temp_data);

		}
	}

	//iterate recursively with children
	for (int i = 0; i < node->children.size(); ++i)
		saveNode(prefab_model, node->children[i], camera);
}

void GTR::RenderCall::saveMeshWithMaterial(const Matrix44 model, Mesh* mesh, GTR::Material* material, Camera* camera)
{
	//in case there is nothing to do
	if (!mesh || !mesh->getNumVertices() || !material)
		return;
	assert(glGetError() == GL_NO_ERROR);

	//material->alpha_mode diu si té transparencies

}

void GTR::RenderCall::orderRenderCall()
{
	//Reordenar
	std::sort(this->renderCall_data.begin(), this->renderCall_data.end(), orderer_distance());
	std::sort(this->renderCall_data.begin(), this->renderCall_data.end(), orderer_alpha());
}

void GTR::RenderCall::renderScene(GTR::Scene* scene, Camera* camera, GTR::Renderer* renderer)
{
	
	//Buidar rendercall_data
	this->renderCall_data.clear();

	this->saveScene(scene, camera);
	this->orderRenderCall();		//Necessita passar-li algo? ho te tot guardat a this?

	//Posar-ho dins una funcio??									
	//set the clear color (the background color)
	glClearColor(scene->background_color.x, scene->background_color.y, scene->background_color.z, 1.0);

	// Clear the color and the depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	checkGLErrors();

	for (int i = 0; i < this->renderCall_data.size(); ++i) {			//Render directe del vector de renderCalls, "ordenat"
		//Podre accedir a scene->ambient_light?

		renderer->renderMeshWithMaterial(this->renderCall_data[i].node_model, this->renderCall_data[i].node->mesh, this->renderCall_data[i].node->material, camera);
	}


}



