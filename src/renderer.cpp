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
#include "application.h"
#include <algorithm>


using namespace GTR;

Renderer::Renderer() {

	render_mode = eRenderMode::MULTI_PATH;
	use_shadowmap = 1;
}


void Renderer::renderScene(GTR::Scene* scene, Camera* camera)
{
	//set the clear color (the background color)
	glClearColor(scene->background_color.x, scene->background_color.y, scene->background_color.z, 1.0);

	this->renderCall_vector.clear();
	this->renderCall_blend_vector.clear();

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
			if(node->material->alpha_mode== GTR::eAlphaMode::NO_ALPHA)
				this->renderCall_vector.push_back(temp_data);
			else
				this->renderCall_blend_vector.push_back(temp_data);

		}
	}

	//iterate recursively with children
	for (int i = 0; i < node->children.size(); ++i)
		renderNode(prefab_model, node->children[i], camera);
}

void GTR::Renderer::orderRenderCalls()
{
	std::sort(this->renderCall_vector.begin(), this->renderCall_vector.end(), RenderCall::orderer_distance());
	std::sort(this->renderCall_blend_vector.begin(), this->renderCall_blend_vector.end(), RenderCall::orderer_distance());
}


void GTR::Renderer::renderRenderCall(Camera* camera)
{

	for (int i = 0; i < this->renderCall_vector.size(); ++i) {			//Render directe del vector de renderCalls opacs, "ordenat"
		//Podre accedir a scene->ambient_light?
		this->renderMeshWithMaterial(this->renderCall_vector[i].node_model, this->renderCall_vector[i].node->mesh, this->renderCall_vector[i].node->material, camera);
	}
	for (int i = 0; i < this->renderCall_blend_vector.size(); ++i) {			//Render directe del vector de renderCalls blend, "ordenat"
		//Podre accedir a scene->ambient_light?
		this->renderMeshWithMaterial(this->renderCall_blend_vector[i].node_model, this->renderCall_blend_vector[i].node->mesh, this->renderCall_blend_vector[i].node->material, camera);
	}

}

void GTR::Renderer::generateShadowMaps(GTR::Scene* scene)
{
	//GTR::Scene* scene = GTR::Scene::instance;
	
	for (int i = 0; i < scene->light_entities.size(); ++i) {
		
		LightEntity* light = scene->light_entities[i];

		//light->light_camera->move(light->model.getTranslation());		//0,0,-1 es el vector endevant 
		//light->light_camera->lookAt(light->model.getTranslation(), light->model * Vector3(0.0,0.0,-1.0), Vector3(0, 1, 0));
		light->light_camera->lookAt(light->model.getTranslation(), light->model.getTranslation() + light->model.frontVector(), Vector3(0, 1, 0));

		if (light->light_type == SPOT) {
			light->light_camera->setPerspective(light->cone_angle*2,1, 1.0f, light->max_distance);	//aspect ratio 1 per cabre a 512x512, manera perque pugui no ser quadrat?
																																									
		}
		else	//directional
		{
			
			light->light_camera->setOrthographic(0,1000,0,1000,1.0,10000);	//light->max_distance
			//shadow_camera->frustum = ??	//DEFINIR

		}
						
		if (light->fbo == NULL) {
			light->fbo = new FBO();
			light->fbo->setDepthOnly(1024, 1024);
		}
		
		//Engegar la camera 
		light->light_camera->enable();	

		light->fbo->bind();
		//scene->light_entities[i]->fbo->depth_texture->clear();
		glColorMask(false, false, false, false);
		glClear(GL_DEPTH_BUFFER_BIT);

		for (int j = 0; j < this->renderCall_vector.size(); ++j) {			
			
			this->renderShadowMap(this->renderCall_vector[j].node_model, this->renderCall_vector[j].node->mesh, this->renderCall_vector[j].node->material, light->light_camera);
		}

		light->fbo->unbind();
		glColorMask(true, true, true, true);	
		//scene->light_entities[i]->viewprojection_matrix = shadow_camera->viewprojection_matrix;
		
	}
	
}
void GTR::Renderer::renderShadowMap(const Matrix44 model, Mesh* mesh, GTR::Material* material, Camera* camera)
{
	if (!mesh || !mesh->getNumVertices() || !material)
		return;
	assert(glGetError() == GL_NO_ERROR);

	//Sense blends i amb depth (obviament)
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);

	//define locals to simplify coding
	Shader* shader = NULL;

	if (material->two_sided)
		glDisable(GL_CULL_FACE);
	else
		glEnable(GL_CULL_FACE);
	assert(glGetError() == GL_NO_ERROR);

	shader = Shader::Get("flat");

	assert(glGetError() == GL_NO_ERROR);

	
	//no shader? then nothing to render
	if (!shader)
		return;
	shader->enable();

	//set Uniforms
	shader->setUniform("u_viewprojection", camera->viewprojection_matrix);
	shader->setUniform("u_camera_pos", camera->eye);
	shader->setUniform("u_model", model);
	shader->setUniform("u_color", Vector4(1.0, 1.0, 1.0,1.0));
	float t = getTime();
	shader->setUniform("u_time", t);

	mesh->render(GL_TRIANGLES);

	shader->disable();

}

void GTR::Renderer::showShadowMap(GTR::LightEntity* light)
{
	glDisable(GL_DEPTH_TEST);
	Shader* zshader = Shader::Get("depth");
	zshader->enable();
	zshader->setUniform("u_camera_nearfar", Vector2(light->light_camera->near_plane, light->light_camera->far_plane));
	light->fbo->depth_texture->toViewport(zshader);
	zshader->disable();
		
	glEnable(GL_DEPTH_TEST);
}


//renders a mesh given its transform and material
void Renderer::renderMeshWithMaterial(const Matrix44 model, Mesh* mesh, GTR::Material* material, Camera* camera)
{
	//in case there is nothing to do
	if (!mesh || !mesh->getNumVertices() || !material )
		return;
    assert(glGetError() == GL_NO_ERROR);

	glEnable(GL_DEPTH_TEST);

	//define locals to simplify coding
	Shader* shader = NULL;
	GTR::Scene* scene = GTR::Scene::instance;	//Per tenir background i ambient light

	Texture* color_texture = material->color_texture.texture;
	if (color_texture == NULL)
		color_texture = Texture::getWhiteTexture(); //a 1x1 white texture

	Texture* metallic_roughness_texture = material->metallic_roughness_texture.texture;
	if (!metallic_roughness_texture)//Pot ser null
		metallic_roughness_texture  = Texture::getWhiteTexture(); //a 1x1 white texture

	Texture* emissive_texture = material->emissive_texture.texture;
	if (!emissive_texture)//Pot ser null
		emissive_texture = Texture::getWhiteTexture(); //a 1x1 white texture

	Texture* normal_texture = material->normal_texture.texture;
	int normalmap_flag = 1; //it has texture
	if (!normal_texture) {//Pot ser null
		normal_texture = Texture::getWhiteTexture(); //a 1x1 white texture
		normalmap_flag = 0; //it does not have texture
	}
	
	//select the blending
	if (material->alpha_mode == GTR::eAlphaMode::BLEND)		//Si te alpha_mode blend, vol dir que té transparencies
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
	else if (render_mode == MULTI_PATH)	//6
		shader = Shader::Get("light_multipass");

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
	
	if(color_texture)
		shader->setUniform("u_texture", color_texture, 0);
	if (metallic_roughness_texture)
		shader->setUniform("u_metallic_roughness_texture", metallic_roughness_texture, 1);
	if (normal_texture)
		shader->setUniform("u_normalmap_texture", normal_texture, 2);
	if (emissive_texture)
		shader->setUniform("u_emissive_texture", emissive_texture, 3);

	shader->setUniform("u_ambient_light", scene->ambient_light);
	
	
	if (scene->light_entities.size()>0 && this->render_mode== eRenderMode::MULTI_PATH) {
		glDepthFunc(GL_LEQUAL);		//Permet pintar al mateix depth

		for (int i = 0; i < scene->light_entities.size(); ++i) {			//Render directe del vector de renderCalls, "ordenat"
			
			if (i == 0 && !(material->alpha_mode == BLEND)) {	//Primera passada i no blend, pinta sense blend
				glDisable(GL_BLEND);
			}
			else {
				glEnable(GL_BLEND);
				shader->setUniform("u_ambient_light", Vector3(0.0,0.0,0.0));
				shader->setUniform("u_emissive_factor", Vector3(0.0, 0.0, 0.0));

				glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			}
			if (material->alpha_mode == BLEND) {

				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				
			}
			//ShadowMaps, nomes si es spot o directional
			if (use_shadowmap == 1) {

				Texture* shadowmap = scene->light_entities[i]->fbo->depth_texture;
				Matrix44 shadow_viewproj = scene->light_entities[i]->light_camera->viewprojection_matrix;

				shader->setTexture("u_shadowmap", shadowmap, 7);
				shader->setUniform("u_shadow_viewproj", shadow_viewproj);
				shader->setUniform("u_shadow_bias", scene->light_entities[i]->bias);
				shader->setUniform("u_shadowmap_flag", (int)use_shadowmap);

			}
			else {
				shader->setUniform("u_shadowmap_flag", (int)use_shadowmap);
			}
			
			shader->setUniform("u_light_color", scene->light_entities[i]->color);
			shader->setUniform("u_light_intensity", scene->light_entities[i]->intensity);
			shader->setUniform("u_light_max_distance", scene->light_entities[i]->max_distance);
			shader->setUniform("u_light_cone_angle", scene->light_entities[i]->cone_angle);
			shader->setUniform("u_light_exponent", scene->light_entities[i]->spot_exponent);
					
			shader->setUniform("u_light_type", scene->light_entities[i]->light_type);
			shader->setUniform("u_light_position", scene->light_entities[i]->model.getTranslation());
			shader->setUniform("u_light_direction", scene->light_entities[i]->model.frontVector());
			
			//this is used to say which is the alpha threshold to what we should not paint a pixel on the screen (to cut polygons according to texture alpha)
			shader->setUniform("u_alpha_cutoff", material->alpha_mode == GTR::eAlphaMode::MASK ? material->alpha_cutoff : 0);
			shader->setUniform("u_normalmap_flag", normalmap_flag);

			//do the draw call that renders the mesh into the screen
			mesh->render(GL_TRIANGLES);

		}
		cont:	//https://stackoverflow.com/questions/41179629/how-to-use-something-like-a-continue-statement-in-nested-for-loops
		glDepthFunc(GL_LESS);	//Default
		glDisable(GL_BLEND);
	}
	else if(scene->light_entities.size() > 0 && this->render_mode == eRenderMode::SINGLE_PATH) {

		Vector3 light_color[GTR::Scene::max_lights] = {};
		float light_intensity[GTR::Scene::max_lights] = {};
		float light_max_dist[GTR::Scene::max_lights] = {};
		float light_cone_angle[GTR::Scene::max_lights] = {};
				
		float light_exponent[GTR::Scene::max_lights] = {};
		int light_type[GTR::Scene::max_lights] = {};
		Vector3 light_position[GTR::Scene::max_lights] = {};
		Vector3 light_direction[GTR::Scene::max_lights] = {};
		
		for (int i = 0; i < scene->light_entities.size(); ++i) {			//Render directe del vector de renderCalls, "ordenat"
			if (scene->light_entities[i]->entity_type == LIGHT) {

				light_color[i] = scene->light_entities[i]->color;
				light_intensity[i] = scene->light_entities[i]->intensity;
				light_max_dist[i] = scene->light_entities[i]->max_distance;
				light_cone_angle[i] = scene->light_entities[i]->cone_angle;

				light_exponent[i] = scene->light_entities[i]->spot_exponent;
				light_type[i] = scene->light_entities[i]->light_type;
				light_position[i] = scene->light_entities[i]->model.getTranslation();
				light_direction[i] = scene->light_entities[i]->model.frontVector();
				//light_direction[i] = scene->light_entities[i]->model.frontVector() * Vector3(-1.0, -1.0, -1.0);
			}
		}
		
		int light_size = scene->light_entities.size();

		shader->setUniform3Array("u_light_color", (float*)light_color, light_size);
		shader->setUniform1Array("u_light_intensity", (float*)light_intensity, light_size);
		shader->setUniform1Array("u_light_max_distance", (float*)light_max_dist, light_size);
		shader->setUniform1Array("u_light_cone_angle", (float*)light_cone_angle, light_size);
	
		shader->setUniform1Array("u_light_exponent", (float*)light_exponent, light_size);
		shader->setUniform1Array("u_light_type", light_type, light_size);
		shader->setUniform3Array("u_light_position", (float*)light_position, light_size);
		//shader->setUniform("u_light_direction", scene->light_entities[i]->temporal_dir);	//arbitrari
		shader->setUniform3Array("u_light_direction", (float*)light_direction, light_size);
		//shader->setUniform("u_light_direction", scene->light_entities[i]->model.frontVector());

		shader->setUniform("u_num_lights", (int)scene->light_entities.size());
	
		//this is used to say which is the alpha threshold to what we should not paint a pixel on the screen (to cut polygons according to texture alpha)
		shader->setUniform("u_alpha_cutoff", material->alpha_mode == GTR::eAlphaMode::MASK ? material->alpha_cutoff : 0);
		shader->setUniform("u_normalmap_flag", normalmap_flag);

		//do the draw call that renders the mesh into the screen
		mesh->render(GL_TRIANGLES);

	
	}
	else {
		//this is used to say which is the alpha threshold to what we should not paint a pixel on the screen (to cut polygons according to texture alpha)
		//select the blending
		if (material->alpha_mode == GTR::eAlphaMode::BLEND)		//Si te alpha_mode blend, vol dir que té transparencies
		{
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);	// Millor metode per transparencies. De + far a + close https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/glBlendFunc.xml
		}
		else
			glDisable(GL_BLEND);
				
		shader->setUniform("u_alpha_cutoff", material->alpha_mode == GTR::eAlphaMode::MASK ? material->alpha_cutoff : 0);

		//do the draw call that renders the mesh into the screen
		mesh->render(GL_TRIANGLES);

	}
	
	//disable shader
	shader->disable();

	//set the render state as it was before to avoid problems with future renders
	glDisable(GL_BLEND);
	
	//Draw the floor grid, helpful to have a reference point
	/*if (Application::instance->render_debug)
		drawGrid();
	*/
}

void GTR::Renderer::renderMeshWithMaterialSingle(const Matrix44 model, Mesh* mesh, GTR::Material* material, Camera* camera)
{
}

void GTR::Renderer::renderMeshWithMaterialMulti(const Matrix44 model, Mesh* mesh, GTR::Material* material, Camera* camera)
{


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

