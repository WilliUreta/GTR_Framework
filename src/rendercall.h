#pragma once
#ifndef RENDERCALL_H
#define RENDERCALL_H

#include "framework.h"
#include "prefab.h"


//forward declarations
class Camera;

namespace GTR {

	class Prefab;
	class Material;


	class RenderCall {
	public:

		//ctor
		//RenderCall();

		//dtor
		//virtual ~RenderCall();
		//

		struct data
		{
			Matrix44 node_model;
			Node* node;		//Node te mesh, material, Aixi o shader i Texture
			//Camera* camera;	//Necessitem camera???
			//Shader* shader = NULL;
			//Texture* texture = NULL; 
		};
		
		std::vector<data> renderCall_data;


		//Node* NodeVector;

		//Guardar les dades de mesh, material, flags, model... enlloc de passar-les al shader.
		//Copia't de renderer, recorrer tots els nodes, i al final, guardar les dades
		void saveScene(GTR::Scene* scene, Camera* camera);
		void savePrefab(const Matrix44& model, GTR::Prefab* prefab, Camera* camera);
		void saveNode(const Matrix44& prefab_model, GTR::Node* node, Camera* camera);
		void saveMeshWithMaterial(const Matrix44 model, Mesh* mesh, GTR::Material* material, Camera* camera);

		//sobra save?
		void saveRenderCall(const Matrix44& prefab_model, GTR::Node* node, Camera* camera);
		void orderRenderCall(const Matrix44 model, Mesh* mesh, GTR::Material* material, Camera* camera);

		//push_back 
	};

}
#endif