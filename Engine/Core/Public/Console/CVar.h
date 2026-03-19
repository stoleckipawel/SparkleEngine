#pragma once

#include "Core/Public/Console/CVarRegistry.h"
#include "Core/Public/CoreAPI.h"

#include <string_view>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <utility>

template <typename T>
concept CVarValueType = std::is_copy_constructible_v<T> && std::is_copy_assignable_v<T>;

class SPARKLE_CORE_API ConsoleVariableBase
{
  public:
	ConsoleVariableBase(std::string_view name, std::string_view description, std::type_index valueType) noexcept;
	virtual ~ConsoleVariableBase() = default;

	ConsoleVariableBase(const ConsoleVariableBase&) = delete;
	ConsoleVariableBase& operator=(const ConsoleVariableBase&) = delete;
	ConsoleVariableBase(ConsoleVariableBase&&) = delete;
	ConsoleVariableBase& operator=(ConsoleVariableBase&&) = delete;

	std::string_view GetName() const noexcept { return m_name; }
	std::string_view GetDescription() const noexcept { return m_description; }
	std::type_index GetValueType() const noexcept { return m_valueType; }

  private:
	std::string_view m_name;
	std::string_view m_description;
	std::type_index m_valueType;
};

template <CVarValueType T> class ConsoleVariable final : public ConsoleVariableBase
{
  public:
	ConsoleVariable(std::string_view name, T defaultValue, std::string_view description = {}) noexcept :
	    ConsoleVariableBase(name, description, std::type_index(typeid(T))), m_value(std::move(defaultValue))
	{
		ConsoleVariableRegistry::Get().Register(*this);
	}

	~ConsoleVariable() override = default;

	ConsoleVariable(const ConsoleVariable&) = delete;
	ConsoleVariable& operator=(const ConsoleVariable&) = delete;
	ConsoleVariable(ConsoleVariable&&) = delete;
	ConsoleVariable& operator=(ConsoleVariable&&) = delete;

	T Get() const noexcept { return m_value; }

	void Set(const T& value) noexcept { m_value = value; }

  private:
	T m_value;
};