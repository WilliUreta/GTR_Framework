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
			Node* node;		//Node te mesh, material
			//Camera* camera;	//Necessitem camera???
			//Shader* shader = NULL;
			//Texture* texture = NULL; 
		};
		
		struct orderer_alpha {		//https://stackoverflow.com/questions/1380463/sorting-a-vector-of-custom-objects

			inline bool operator() (data& rc_a, data& rc_b) {

				return(rc_a.node->material->alpha_mode <= rc_b.node->material->alpha_mode);		//Blend = 2, Mask 1, no alpha =0, volem  primer els 2.
			}
		};

		/*
		struct orderer_distance {	

			inline bool operator() (data& rc_a, data& rc_b) {

				return(rc_a.node_model.getXYZ );		//Blend = 2, Mask 1, no alpha =0, volem  primer els 2.
			}
		};*/
		

		std::vector<data> renderCall_data;


		//Guardar les dades de mesh, material, flags, model... enlloc de passar-les al shader.
		//Copia't de renderer, recorrer tots els nodes, i al final, guardar les dades
		void saveScene(GTR::Scene* scene, Camera* camera);
		void savePrefab(const Matrix44& model, GTR::Prefab* prefab, Camera* camera);
		void saveNode(const Matrix44& prefab_model, GTR::Node* node, Camera* camera);
		void saveMeshWithMaterial(const Matrix44 model, Mesh* mesh, GTR::Material* material, Camera* camera);

		//sobra save?
		void saveRenderCall(const Matrix44& prefab_model, GTR::Node* node, Camera* camera);
		void orderRenderCall();

		//void orderRenderCall(const Matrix44 model, Mesh* mesh, GTR::Material* material, Camera* camera);


		//push_back 
	};

}
#endif