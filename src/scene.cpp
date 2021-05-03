#include "scene.h"
#include "utils.h"

#include "prefab.h"
#include "extra/cJSON.h"

GTR::Scene* GTR::Scene::instance = NULL;

GTR::Scene::Scene()
{
	instance = this;
	//this->max_lights = 3;
}

void GTR::Scene::clear()
{
	for (int i = 0; i < entities.size(); ++i)
	{
		BaseEntity* ent = entities[i];
		delete ent;
	}
	entities.resize(0);
}


void GTR::Scene::addEntity(BaseEntity* entity)
{
	entities.push_back(entity); entity->scene = this;
}

bool GTR::Scene::load(const char* filename)
{
	std::string content;

	this->filename = filename;
	std::cout << " + Reading scene JSON: " << filename << "..." << std::endl;

	if (!readFile(filename, content))
	{
		std::cout << "- ERROR: Scene file not found: " << filename << std::endl;
		return false;
	}

	//parse json string 
	cJSON* json = cJSON_Parse(content.c_str());
	if (!json)
	{
		std::cout << "ERROR: Scene JSON has errors: " << filename << std::endl;
		return false;
	}

	//read global properties
	background_color = readJSONVector3(json, "background_color", background_color);
	ambient_light = readJSONVector3(json, "ambient_light", ambient_light );

	//entities
	cJSON* entities_json = cJSON_GetObjectItemCaseSensitive(json, "entities");
	cJSON* entity_json;
	cJSON_ArrayForEach(entity_json, entities_json)
	{
		std::string type_str = cJSON_GetObjectItem(entity_json, "type")->valuestring;
		BaseEntity* ent = createEntity(type_str);
		if (!ent)
		{
			std::cout << " - ENTITY TYPE UNKNOWN: " << type_str << std::endl;
			//continue;
			ent = new BaseEntity();
		}

		addEntity(ent);

		if (cJSON_GetObjectItem(entity_json, "name"))
		{
			ent->name = cJSON_GetObjectItem(entity_json, "name")->valuestring;
			stdlog(std::string(" + entity: ") + ent->name);
		}

		//read transform
		if (cJSON_GetObjectItem(entity_json, "position"))
		{
			ent->model.setIdentity();
			Vector3 position = readJSONVector3(entity_json, "position", Vector3());
			ent->model.translate(position.x, position.y, position.z);
		}

		if (cJSON_GetObjectItem(entity_json, "angle"))
		{
			float angle = cJSON_GetObjectItem(entity_json, "angle")->valuedouble;
			ent->model.rotate(angle * DEG2RAD, Vector3(0, 1, 0));
		}

		if (cJSON_GetObjectItem(entity_json, "rotation"))
		{
			Vector4 rotation = readJSONVector4(entity_json, "rotation");
			Quaternion q(rotation.x, rotation.y, rotation.z, rotation.w);
			Matrix44 R;
			q.toMatrix(R);
			ent->model = R * ent->model;
		}

		if (cJSON_GetObjectItem(entity_json, "scale"))
		{
			Vector3 scale = readJSONVector3(entity_json, "scale", Vector3(1, 1, 1));
			ent->model.scale(scale.x, scale.y, scale.z);
		}

		ent->configure(entity_json);
		if (ent->entity_type == LIGHT) {

			LightEntity* lent = (GTR::LightEntity*)ent;
			if (this->light_entities.size() < this->max_lights) {
				this->light_entities.push_back(lent);
			}
			else {
				printf("Too much lights in the scene, the max is: %d ", this->max_lights);

			}
		}

	}

	//free memory
	cJSON_Delete(json);

	return true;
}


GTR::BaseEntity* GTR::Scene::createEntity(std::string type)		//Modificar quan volguem més identitats diferents
{
	if (type == "PREFAB")
		return new GTR::PrefabEntity();
	if (type == "LIGHT")
		return new GTR::LightEntity();
	else
		return NULL;
}

void GTR::BaseEntity::renderInMenu()
{
#ifndef SKIP_IMGUI
	ImGui::Text("Name: %s", name.c_str()); // Edit 3 floats representing a color
	ImGui::Checkbox("Visible", &visible); // Edit 3 floats representing a color
	//Model edit
	ImGuiMatrix44(model, "Model");
#endif
}



GTR::PrefabEntity::PrefabEntity()
{
	entity_type = PREFAB;
	prefab = NULL;
}

void GTR::PrefabEntity::configure(cJSON* json)		//Modificar per altres entitats
{
	if (cJSON_GetObjectItem(json, "filename"))
	{
		filename = cJSON_GetObjectItem(json, "filename")->valuestring;
		prefab = GTR::Prefab::Get( (std::string("data/") + filename).c_str());
	}
}

void GTR::PrefabEntity::renderInMenu()
{
	BaseEntity::renderInMenu();

#ifndef SKIP_IMGUI
	ImGui::Text("filename: %s", filename.c_str()); // Edit 3 floats representing a color
	if (prefab && ImGui::TreeNode(prefab, "Prefab Info"))
	{
		prefab->root.renderInMenu();
		ImGui::TreePop();
	}
#endif
}


GTR::LightEntity::LightEntity()
{
	entity_type = LIGHT;
	color = Vector3(1.0,1.0,1.0);
	intensity = 1.0;
	light_type = SPOT;

	max_distance = 300.0;
	cone_angle = 20;
	area_size = 50;
}

void GTR::LightEntity::configure(cJSON* json)		//Modificar per altres entitats
{
	if (cJSON_GetObjectItem(json, "distance"))
	{
		this->max_distance = cJSON_GetObjectItem(json, "distance")->valuedouble;
	}
	if (cJSON_GetObjectItem(json, "cone_angle"))
	{
		this->cone_angle = cJSON_GetObjectItem(json, "cone_angle")->valuedouble;
	}
	if (cJSON_GetObjectItem(json, "area_size"))
	{
		this->area_size = cJSON_GetObjectItem(json, "area_size")->valuedouble;
	}
	if (cJSON_GetObjectItem(json, "color"))
	{
		this->color = readJSONVector3(json, "color", Vector3(1, 1, 1));
	}
	if (cJSON_GetObjectItem(json, "light_type"))
	{
		std::string str = cJSON_GetObjectItem(json, "light_type")->valuestring;
		if (str == "POINT")
			this->light_type = eLightType(POINT);
		if (str == "SPOT")
			this->light_type = eLightType(SPOT);
		if (str == "DIRECTIONAL")
			this->light_type = eLightType(DIRECTIONAL);
		
	}
	if (cJSON_GetObjectItem(json, "direction"))
	{
		this->model.setFrontAndOrthonormalize((readJSONVector3(json, "direction", Vector3(0, 0, 0))- this->model.getTranslation()));//target-light position
		this->model.rotateVector(readJSONVector3(json, "direction", Vector3(0, 0, 0)));	//rotateVector rota el vector SENSE la translacio. Multiplica nomes la part de rotacio de la matriu model
//		this->model.lookAt(this->model.getTranslation(), readJSONVector3(json, "direction",Vector3(0,0,0)), Vector3(0.0, 1.0, 0.0));
	}
	if (cJSON_GetObjectItem(json, "spot_exponent"))
	{
		this->spot_exponent = cJSON_GetObjectItem(json, "spot_exponent")->valuedouble;
	}
}

void GTR::LightEntity::renderInMenu()
{
	//BaseEntity::renderInMenu();

#ifndef SKIP_IMGUI
	
	ImGui::ColorEdit4("Color", this->color.v);
	ImGui::DragFloat3("Direction", this->temporal_dir.v);

	ImGui::SliderFloat("Cone Angle", &this->cone_angle, 1.0,180.0);
	ImGui::SliderFloat("Area size",&this->area_size,0.0, 50.0);
	ImGui::SliderFloat("Intensity", &this->intensity, 0.0, 10.0);
	ImGui::SliderFloat("Max Distance", &this->max_distance, 10.0, 1000.0);
	ImGui::SliderFloat("Spot exponent", &this->spot_exponent, 0.0, 50.0);

	
	ImGui::Combo("Light Type", (int*)&this->light_type, "POINT\0SPOT\0DIRECTIONAL", 3);

#endif
}