#pragma once

#include "Core/Public/Console/CVarRegistry.h"
#include "Core/Public/CoreAPI.h"
#include "Core/Public/Strings/StringUtils.h"

#include <string>
#include <string_view>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <utility>

template <typename T>
concept CVarValueType = std::is_default_constructible_v<T> && std::is_copy_constructible_v<T> && std::is_copy_assignable_v<T>;

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

	virtual std::string GetValueAsString() const = 0;
	virtual std::string GetValueTypeName() const = 0;
	virtual bool TrySetValueFromString(std::string_view value, std::string& errorMessage) = 0;

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

	std::string GetValueAsString() const override { return FormatValue(m_value); }

	std::string GetValueTypeName() const override { return ResolveValueTypeName(); }

	bool TrySetValueFromString(std::string_view value, std::string& errorMessage) override
	{
		T parsedValue{};
		if (!ParseValue(value, parsedValue, errorMessage))
		{
			return false;
		}

		Set(parsedValue);
		return true;
	}

  private:
	static std::string FormatValue(const T& value)
	{
		if constexpr (std::is_same_v<T, bool>)
		{
			return value ? "true" : "false";
		}
		else if constexpr (std::is_enum_v<T>)
		{
			return std::to_string(static_cast<std::underlying_type_t<T>>(value));
		}
		else if constexpr (std::is_integral_v<T>)
		{
			return std::to_string(value);
		}
		else if constexpr (std::is_floating_point_v<T>)
		{
			return std::to_string(value);
		}
		else if constexpr (std::is_same_v<T, std::string>)
		{
			return value;
		}
		else
		{
			return "<unsupported>";
		}
	}

	static bool ParseValue(std::string_view value, T& parsedValue, std::string& errorMessage)
	{
		if constexpr (std::is_same_v<T, bool>)
		{
			if (Engine::Strings::TryParseBool(value, parsedValue))
			{
				return true;
			}

			errorMessage = "expected bool value: true/false, 1/0, on/off, or yes/no";
			return false;
		}
		else if constexpr (std::is_enum_v<T>)
		{
			std::underlying_type_t<T> underlyingValue{};
			if (!Engine::Strings::TryParseNumber(value, underlyingValue))
			{
				errorMessage = "expected numeric enum value";
				return false;
			}

			parsedValue = static_cast<T>(underlyingValue);
			return true;
		}
		else if constexpr (std::is_integral_v<T>)
		{
			if (!Engine::Strings::TryParseNumber(value, parsedValue))
			{
				errorMessage = "expected integer value";
				return false;
			}
			return true;
		}
		else if constexpr (std::is_floating_point_v<T>)
		{
			if (!Engine::Strings::TryParseNumber(value, parsedValue))
			{
				errorMessage = "expected floating-point value";
				return false;
			}
			return true;
		}
		else if constexpr (std::is_same_v<T, std::string>)
		{
			parsedValue = std::string(value);
			return true;
		}
		else
		{
			errorMessage = "unsupported CVar value type";
			return false;
		}
	}

	static std::string ResolveValueTypeName()
	{
		if constexpr (std::is_same_v<T, bool>)
		{
			return "bool";
		}
		else if constexpr (std::is_enum_v<T>)
		{
			return "enum";
		}
		else if constexpr (std::is_integral_v<T>)
		{
			return std::is_signed_v<T> ? "integer" : "unsigned integer";
		}
		else if constexpr (std::is_floating_point_v<T>)
		{
			return "float";
		}
		else if constexpr (std::is_same_v<T, std::string>)
		{
			return "string";
		}
		else
		{
			return "unsupported";
		}
	}

	T m_value;
};
