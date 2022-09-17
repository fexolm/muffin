#include "Scene.h"

void Scene::AddObject(RenderObjectRef object)
{
	objects.push_back(object);
}

RenderObjectRef Scene::GetObject(const std::string& name)
{
	for (RenderObjectRef& obj : objects) {
		if (obj->Name() == name) {
			return obj;
		}
	}
	return nullptr;
}

const std::vector<RenderObjectRef>& Scene::GetObjects()
{
	return objects;
}