#include "PCH.h"
#include "Scene/Entity.h"

Entity::Entity(const std::string& entityName) : name(entityName) {}

Entity::~Entity() = default;

const std::string& Entity::GetName() const
{
	return name;
}

bool Entity::IsActive() const
{
	return bActive;
}

void Entity::SetActive(bool isActive)
{
	bActive = isActive;
}

void Entity::Initialize()
{
	for (auto& component : components)
	{
		component->Initialize();
	}
}

void Entity::Update(float deltaTime)
{
	if (!bActive)
	{
		return;
	}

	for (auto& component : components)
	{
		component->Update(deltaTime);
	}
}

void Entity::Render()
{
	if (!bActive)
	{
		return;
	}

	for (auto& component : components)
	{
		component->Render();
	}
}