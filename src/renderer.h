#pragma once
#include "prefab.h"

#include "framework.h"
#include "rendercall.h"
#include <algorithm>	

//forward declarations
class Camera;

namespace GTR {


	enum eRenderMode { 
		SINGLE_PATH, 
		MULTI_PATH, 
		NORMALS,
		TEXTURE,
		UVS 
	}; //types of cameras available

	class Prefab;
	class Material;
	class RenderCall;
	
	// This class is in charge of rendering anything in our system.
	// Separating the render from anything else makes the code cleaner
	class Renderer
	{

	public:

		eRenderMode render_mode;
		std::vector<GTR::RenderCall> renderCall_vector;

		Renderer();

		//add here your functions
		//...

		//renders several elements of the scene
		void renderScene(GTR::Scene* scene, Camera* camera);
	
		//to render a whole prefab (with all its nodes)
		void renderPrefab(const Matrix44& model, GTR::Prefab* prefab, Camera* camera);

		//to render one node from the prefab and its children
		void renderNode(const Matrix44& model, GTR::Node* node, Camera* camera);

		//to render one mesh given its material and transformation matrix
		void renderMeshWithMaterial(const Matrix44 model, Mesh* mesh, GTR::Material* material, Camera* camera);
	
		void renderRenderCall(Camera* camera);
	};


	Texture* CubemapFromHDRE(const char* filename);

};
