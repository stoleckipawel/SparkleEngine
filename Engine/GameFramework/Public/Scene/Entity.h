#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "Component.h"

#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

class SPARKLE_ENGINE_API Entity
{
  private:
	std::string name;
	bool bActive = true;
	std::vector<std::unique_ptr<Component>> components;

  public:
	explicit Entity(const std::string& entityName);
	~Entity();

	const std::string& GetName() const;
	bool IsActive() const;
	void SetActive(bool isActive);

	void Initialize();

	void Update(float deltaTime);

	void Render();

	template <typename T, typename... Args> T* AddComponent(Args&&... args)
	{
		static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");

		// Create new component
		auto component = std::make_unique<T>(std::forward<Args>(args)...);
		T* componentPtr = component.get();
		componentPtr->SetOwner(this);
		components.push_back(std::move(component));
		return componentPtr;
	}

	template <typename T> T* GetComponent()
	{
		for (auto& component : components)
		{
			if (T* result = dynamic_cast<T*>(component.get()))
			{
				return result;
			}
		}
		return nullptr;
	}

	template <typename T> bool RemoveComponent()
	{
		for (auto it = components.begin(); it != components.end(); ++it)
		{
			if (dynamic_cast<T*>(it->get()))
			{
				components.erase(it);
				return true;
			}
		}
		return false;
	}
};