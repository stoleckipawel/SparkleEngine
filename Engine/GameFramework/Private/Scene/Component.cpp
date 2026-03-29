#include "PCH.h"
#include "Component.h"

Component::~Component() = default;

void Component::Initialize()
{
}

void Component::Update(float deltaTime)
{
	static_cast<void>(deltaTime);
}

void Component::Render()
{
}

void Component::SetOwner(Entity* entity)
{
	owner = entity;
}

Entity* Component::GetOwner() const
{
	return owner;
}