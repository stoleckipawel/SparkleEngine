#pragma once

#include "World/ECS/ComponentTypeRegistry.h"

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace ECS
{
	template <ComponentStorageCompatible T> struct Read final
	{
		using Component = T;
	};

	template <ComponentStorageCompatible T> struct Write final
	{
		using Component = T;
	};

	template <ComponentStorageCompatible T> struct Exclude final
	{
		using Component = T;
	};

	enum class ComponentAccessMode : std::uint8_t
	{
		Read,
		Write,
		Exclude,
	};

	struct ComponentAccessDesc final
	{
		RuntimeComponentTypeId Type;
		ComponentAccessMode Mode = ComponentAccessMode::Read;
	};

	enum class QueryIterationStatus : std::uint8_t
	{
		Success,
		InvalidEpoch,
		StaleView,
	};

	struct QueryIterationResult final
	{
		QueryIterationStatus Status = QueryIterationStatus::InvalidEpoch;
		std::size_t EntityCount = 0;

		bool Succeeded() const noexcept { return Status == QueryIterationStatus::Success; }
	};

	template <typename T> struct QueryAccessTraits;

	template <ComponentStorageCompatible T> struct QueryAccessTraits<Read<T>> final
	{
		using Component = T;
		static constexpr ComponentAccessMode Mode = ComponentAccessMode::Read;
		static constexpr bool Included = true;
		static constexpr bool Writable = false;
	};

	template <ComponentStorageCompatible T> struct QueryAccessTraits<Write<T>> final
	{
		using Component = T;
		static constexpr ComponentAccessMode Mode = ComponentAccessMode::Write;
		static constexpr bool Included = true;
		static constexpr bool Writable = true;
	};

	template <ComponentStorageCompatible T> struct QueryAccessTraits<Exclude<T>> final
	{
		using Component = T;
		static constexpr ComponentAccessMode Mode = ComponentAccessMode::Exclude;
		static constexpr bool Included = false;
		static constexpr bool Writable = false;
	};

	template <typename T> concept QueryAccessSpec = requires {
		typename QueryAccessTraits<T>::Component;
		{ QueryAccessTraits<T>::Mode } -> std::convertible_to<ComponentAccessMode>;
	};

	template <typename First, typename... Rest> consteval bool HaveUniqueQueryComponents()
	{
		using Component = typename QueryAccessTraits<First>::Component;
		if constexpr (sizeof...(Rest) == 0)
		{
			return true;
		}
		else
		{
			return ((!std::is_same_v<Component, typename QueryAccessTraits<Rest>::Component>) && ...)
			    && HaveUniqueQueryComponents<Rest...>();
		}
	}
}
