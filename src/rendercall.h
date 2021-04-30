#pragma once
#ifndef RENDERCALL_H
#define RENDERCALL_H

#include "framework.h"
#include "prefab.h"
#include "renderer.h"


//forward declarations
class Camera;

namespace GTR {

	class Prefab;
	class Material;
	class Renderer;


	class RenderCall {
	public:

		//ctor
		//RenderCall();

		//dtor
		//virtual ~RenderCall();
		//
	
		struct orderer_distance {		//https://stackoverflow.com/questions/1380463/sorting-a-vector-of-custom-objects

			inline bool operator() (RenderCall& rc_a, RenderCall& rc_b) {
				
				return(rc_a.distance < rc_b.distance);		//Si esta mes lluny, primer
			}
		};

		struct orderer_alpha {		//https://stackoverflow.com/questions/1380463/sorting-a-vector-of-custom-objects

			inline bool operator() (RenderCall& rc_a, RenderCall& rc_b) {	//rc_a.node->material->alpha_mode == Opaque ...

				return((((rc_a.node->material->alpha_mode == GTR::eAlphaMode::NO_ALPHA) && (rc_b.node->material->alpha_mode == GTR::eAlphaMode::NO_ALPHA)) || ((rc_a.node->material->alpha_mode == GTR::eAlphaMode::BLEND) && (rc_b.node->material->alpha_mode == GTR::eAlphaMode::BLEND))));		//Blend = 2, Mask 1, no alpha =0, volem  primer els 2.
				
			
			}
		};

		Matrix44 node_model;
		Node* node;		//Node te mesh, material
		float distance;
		//void RenderCall::RenderCall();

		//Guardar les dades de mesh, material, flags, model... enlloc de passar-les al shader.
		//Copia't de renderer, recorrer tots els nodes, i al final, guardar les dades


		//void orderRenderCall(const Matrix44 model, Mesh* mesh, GTR::Material* material, Camera* camera);


		//push_back 
	};

}
#endif