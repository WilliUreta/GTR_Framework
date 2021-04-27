#include "renderer.h"

#include "camera.h"
#include "shader.h"
#include "mesh.h"
#include "texture.h"
#include "prefab.h"
#include "material.h"
#include "utils.h"
#include "scene.h"
#include "extra/hdre.h"

#include "rendercall.h"
#include <algorithm>


using namespace GTR;

Renderer::Renderer() {

	render_mode = eRenderMode::TEXTURE;
}


void Renderer::renderScene(GTR::Scene* scene, Camera* camera)
{
	//set the clear color (the background color)
	glClearColor(scene->background_color.x, scene->background_color.y, scene->background_color.z, 1.0);

	this->renderCall_vector.clear();

	// Clear the color and the depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	checkGLErrors();

	//render entities
	for (int i = 0; i < scene->entities.size(); ++i)
	{
		BaseEntity* ent = scene->entities[i];
		if (!ent->visible)
			continue;

		//is a prefab!
		if (ent->entity_type == PREFAB)
		{
			PrefabEntity* pent = (GTR::PrefabEntity*)ent;
			if(pent->prefab)
				renderPrefab(ent->model, pent->prefab, camera);
		}
		/*else if (ent->entity_type == LIGHT) {

			LightEntity* lent = (GTR::LightEntity*)ent;
			int temp = scene->light_entities.size();
			if (scene->light_entities.size() < scene->max_lights) {
				scene->light_entities.push_back(lent);
				
			}
			else {
				printf("Too much lights in the scene, the max is: %d ", scene->max_lights);
				int temp = scene->light_entities.size();
			}
		}*/

	}
}

//renders all the prefab
void Renderer::renderPrefab(const Matrix44& model, GTR::Prefab* prefab, Camera* camera)
{
	assert(prefab && "PREFAB IS NULL");
	//assign the model to the root node
	renderNode(model, &prefab->root, camera);
}

//renders a node of the prefab and its children
void Renderer::renderNode(const Matrix44& prefab_model, GTR::Node* node, Camera* camera)
{
	if (!node->visible)
		return;

	//compute global matrix
	Matrix44 node_model = node->getGlobalMatrix(true) * prefab_model;

	//does this node have a mesh? then we must render it
	if (node->mesh && node->material)
	{
		//compute the bounding box of the object in world space (by using the mesh bounding box transformed to world space)
		BoundingBox world_bounding = transformBoundingBox(node_model,node->mesh->box);
		
		//if bounding box is inside the camera frustum then the object is probably visible
		if (camera->testBoxInFrustum(world_bounding.center, world_bounding.halfsize) )
		{

			//render node mesh

			//renderMeshWithMaterial( node_model, node->mesh, node->material, camera );
			
			//node->mesh->renderBounding(node_model, true);

			float dist = camera->eye.distance(world_bounding.center);
			RenderCall temp_data = { node_model, node, dist };

			this->renderCall_vector.push_back(temp_data);

		}
	}

	//iterate recursively with children
	for (int i = 0; i < node->children.size(); ++i)
		renderNode(prefab_model, node->children[i], camera);
}


void GTR::Renderer::renderRenderCall(Camera* camera)
{

	std::sort(this->renderCall_vector.begin(), this->renderCall_vector.end(), RenderCall::orderer_distance());
	std::sort(this->renderCall_vector.begin(), this->renderCall_vector.end(), RenderCall::orderer_alpha());

	for (int i = 0; i < this->renderCall_vector.size(); ++i) {			//Render directe del vector de renderCalls, "ordenat"
		//Podre accedir a scene->ambient_light?

		this->renderMeshWithMaterial(this->renderCall_vector[i].node_model, this->renderCall_vector[i].node->mesh, this->renderCall_vector[i].node->material, camera);
	}


}

//renders a mesh given its transform and material
void Renderer::renderMeshWithMaterial(const Matrix44 model, Mesh* mesh, GTR::Material* material, Camera* camera)
{
	//in case there is nothing to do
	if (!mesh || !mesh->getNumVertices() || !material )
		return;
    assert(glGetError() == GL_NO_ERROR);

	//define locals to simplify coding
	Shader* shader = NULL;
	Texture* texture = NULL;
	GTR::Scene* scene = GTR::Scene::instance;	//Per tenir background i ambient light

	texture = material->color_texture.texture;
	//texture = material->emissive_texture;
	//texture = material->metallic_roughness_texture.texture; //te oclusio metallic i roughness
	//texture = material->normal_texture;
	//texture = material->occlusion_texture.texture;

	if (texture == NULL)
		texture = Texture::getWhiteTexture(); //a 1x1 white texture

	Texture* metallic_roughness_texture = material->metallic_roughness_texture.texture;
	if (!metallic_roughness_texture)//Pot ser null
		metallic_roughness_texture  = Texture::getWhiteTexture(); //a 1x1 white texture

	Texture* emissive_texture = material->emissive_texture.texture;
	if (!emissive_texture)//Pot ser null
		emissive_texture = Texture::getWhiteTexture(); //a 1x1 white texture

													 
	//select the blending
	if (material->alpha_mode == GTR::eAlphaMode::BLEND)		//Si te alpha_mode blend, vol dir que t� transparencies
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);	// Millor metode per transparencies. De + far a + close https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/glBlendFunc.xml
	}
	else
		glDisable(GL_BLEND);

	//select if render both sides of the triangles
	if(material->two_sided)
		glDisable(GL_CULL_FACE);
	else
		glEnable(GL_CULL_FACE);
    assert(glGetError() == GL_NO_ERROR);

	//chose a shader
	if (render_mode == NORMALS)			//1
		shader = Shader::Get("normal");
	else if (render_mode == TEXTURE)	//2
		shader = Shader::Get("texture");
	else if (render_mode == UVS)			//3
		shader = Shader::Get("uvs");
	else if (render_mode == SINGLE_PATH)	//4
		shader = Shader::Get("light_singlepass");

    assert(glGetError() == GL_NO_ERROR);


	//no shader? then nothing to render
	if (!shader)
		return;
	shader->enable();
	
	//upload uniforms
	shader->setUniform("u_viewprojection", camera->viewprojection_matrix);
	shader->setUniform("u_camera_position", camera->eye);
	shader->setUniform("u_model", model );
	float t = getTime();
	shader->setUniform("u_time", t );

	shader->setUniform("u_color", material->color);
	shader->setUniform("u_emissive_factor", material->emissive_factor);
	
	if(texture)
		shader->setUniform("u_texture", texture, 0);
	if (metallic_roughness_texture)
		shader->setUniform("u_metallic_roughness_texture", metallic_roughness_texture, 1);
	if (metallic_roughness_texture)
		shader->setUniform("u_metallic_roughness_texture", metallic_roughness_texture, 1);
	if (emissive_texture)
		shader->setUniform("u_emissive_texture", emissive_texture, 2);

	
	shader->setUniform("u_ambient_light", scene->ambient_light);
	
	
	if (scene->light_entities.size()>0) {

		for (int i = 0; i < scene->light_entities.size(); ++i) {			//Render directe del vector de renderCalls, "ordenat"
				if (scene->light_entities[i]->entity_type == LIGHT) {
						
					//shader->setUniform("u_light_color", scene->light_entities[i]->color);
					shader->setUniform("u_light_color", scene->light_entities[i]->color);
					shader->setUniform("u_light_type", scene->light_entities[i]->light_type);

					shader->setUniform("u_light_position", scene->light_entities[i]->model.getTranslation());
				}
		}

	}
	

	//this is used to say which is the alpha threshold to what we should not paint a pixel on the screen (to cut polygons according to texture alpha)
	shader->setUniform("u_alpha_cutoff", material->alpha_mode == GTR::eAlphaMode::MASK ? material->alpha_cutoff : 0);

	//do the draw call that renders the mesh into the screen
	mesh->render(GL_TRIANGLES);

	//disable shader
	shader->disable();

	//set the render state as it was before to avoid problems with future renders
	glDisable(GL_BLEND);
}


Texture* GTR::CubemapFromHDRE(const char* filename)
{
	HDRE* hdre = new HDRE();
	if (!hdre->load(filename))
	{
		delete hdre;
		return NULL;
	}

	/*
	Texture* texture = new Texture();
	texture->createCubemap(hdre->width, hdre->height, (Uint8**)hdre->getFaces(0), hdre->header.numChannels == 3 ? GL_RGB : GL_RGBA, GL_FLOAT );
	for(int i = 1; i < 6; ++i)
		texture->uploadCubemap(texture->format, texture->type, false, (Uint8**)hdre->getFaces(i), GL_RGBA32F, i);
	return texture;
	*/
	return NULL;
}

