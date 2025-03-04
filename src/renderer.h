#pragma once
#include "prefab.h"

#include "framework.h"
#include "rendercall.h"
#include "fbo.h"
#include "application.h"
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
		bool use_shadowmap;
		bool show_shadowmap;
		std::vector<GTR::RenderCall> renderCall_vector;
		std::vector<GTR::RenderCall> renderCall_blend_vector;

		Renderer();

		//add here your functions
		//...

		//renders several elements of the scene
		void getSceneRenderCalls(GTR::Scene* scene, Camera* camera);
	
		//to render a whole prefab (with all its nodes)
		void renderPrefab(const Matrix44& model, GTR::Prefab* prefab, Camera* camera);

		//to render one node from the prefab and its children
		void renderNode(const Matrix44& model, GTR::Node* node, Camera* camera);

		void orderRenderCalls();

		//to render one mesh given its material and transformation matrix
		void renderMeshWithMaterial(const Matrix44 model, Mesh* mesh, GTR::Material* material, Camera* camera);
		void renderMeshWithMaterialSingle(const Matrix44 model, Mesh* mesh, GTR::Material* material, Camera* camera, Scene* scene, Shader* shader, int normalmap_flag);
		void renderMeshWithMaterialMulti(const Matrix44 model, Mesh* mesh, GTR::Material* material, Camera* camera, Scene* scene, Shader* shader, int normalmap_flag);
	
		void renderRenderCall(Camera* camera);

		void generateShadowMaps(GTR::Scene* scene);

		void renderShadowMap(const Matrix44 model, Mesh* mesh, GTR::Material* material, Camera* camera);

		void showShadowMaps( int w, int h);

		void showShadowMap(GTR::LightEntity* light);
	};


	Texture* CubemapFromHDRE(const char* filename);

};
