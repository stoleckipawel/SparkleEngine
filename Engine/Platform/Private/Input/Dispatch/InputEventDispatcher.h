#pragma once

#include "Input/InputSystem.h"

#include <cstddef>
#include <type_traits>
#include <algorithm>
#include <array>
#include <cstdint>
#include <tuple>
#include <utility>
#include <vector>

class InputEventDispatcher final
{
public:
	static constexpr std::size_t LayerCount = static_cast<std::size_t>(InputLayer::Count);
	using LayerAvailability = std::array<bool, LayerCount>;

	template <typename TEvent> EventHandle Subscribe(InputCallback<TEvent> callback, InputLayer layer, DispatchMode mode)
	{
		EventHandle handle{m_nextCallbackId++};
		GetCallbacks<TEvent>().push_back({std::move(callback), handle, layer, mode});
		return handle;
	}

	void Unsubscribe(EventHandle handle)
	{
		if (!handle.IsValid())
		{
			return;
		}

		auto removeByHandle = [&handle](auto& callbacks)
		{
			callbacks.erase(
			    std::remove_if(callbacks.begin(), callbacks.end(), [&handle](const auto& entry) { return entry.Handle == handle; }),
			    callbacks.end());
		};

		std::apply([&removeByHandle](auto&... callbacks) { (removeByHandle(callbacks), ...); }, m_callbacks);
	}

	template <typename TEvent>
	void DispatchImmediate(const TEvent& event, InputLayer targetLayer, const LayerAvailability& layerAvailability)
	{
		DispatchToCallbacks(event, DispatchMode::Immediate, targetLayer, layerAvailability);
	}

	template <typename TEvent> void QueueDeferred(const TEvent& event, InputLayer targetLayer, const LayerAvailability& layerAvailability)
	{
		for (const auto& entry : GetCallbacks<TEvent>())
		{
			if (entry.Mode == DispatchMode::Deferred && ShouldDispatchToLayer(entry.Layer, targetLayer, layerAvailability))
			{
				GetDeferredQueue<TEvent>().push_back({event, targetLayer});
				return;
			}
		}
	}

	template <typename PrepareDispatch>
	void ProcessDeferredEvents(const LayerAvailability& layerAvailability, PrepareDispatch&& prepareDispatch)
	{
		if (m_isProcessingDeferredEvents)
		{
			return;
		}

		m_isProcessingDeferredEvents = true;
		try
		{
			std::forward<PrepareDispatch>(prepareDispatch)();
			ProcessDeferredEventsForType<KeyboardEvent>(layerAvailability);
			ProcessDeferredEventsForType<MouseButtonEvent>(layerAvailability);
			ProcessDeferredEventsForType<MouseMoveEvent>(layerAvailability);
			ProcessDeferredEventsForType<MouseWheelEvent>(layerAvailability);
		}
		catch (...)
		{
			m_isProcessingDeferredEvents = false;
			throw;
		}
		m_isProcessingDeferredEvents = false;
	}

	void ClearDeferredEvents()
	{
		GetDeferredQueue<KeyboardEvent>().clear();
		GetDeferredQueue<MouseButtonEvent>().clear();
		GetDeferredQueue<MouseMoveEvent>().clear();
		GetDeferredQueue<MouseWheelEvent>().clear();
	}

private:
	template <typename TEvent> struct CallbackEntry final
	{
		InputCallback<TEvent> Callback;
		EventHandle Handle;
		InputLayer Layer = InputLayer::Gameplay;
		DispatchMode Mode = DispatchMode::Immediate;
	};

	template <typename TEvent> struct RoutedInputEvent final
	{
		TEvent Event;
		InputLayer TargetLayer = InputLayer::System;
	};

	using CallbackTuple = std::tuple<
	    std::vector<CallbackEntry<KeyboardEvent>>,
	    std::vector<CallbackEntry<MouseButtonEvent>>,
	    std::vector<CallbackEntry<MouseMoveEvent>>,
	    std::vector<CallbackEntry<MouseWheelEvent>>>;

	using DeferredQueueTuple = std::tuple<
	    std::vector<RoutedInputEvent<KeyboardEvent>>,
	    std::vector<RoutedInputEvent<MouseButtonEvent>>,
	    std::vector<RoutedInputEvent<MouseMoveEvent>>,
	    std::vector<RoutedInputEvent<MouseWheelEvent>>>;

	static bool ShouldDispatchToLayer(
	    InputLayer registeredLayer,
	    InputLayer targetLayer,
	    const LayerAvailability& layerAvailability) noexcept
	{
		if (registeredLayer == InputLayer::System)
		{
			return true;
		}
		if (registeredLayer != targetLayer)
		{
			return false;
		}

		const auto index = static_cast<std::size_t>(registeredLayer);
		return index < layerAvailability.size() && layerAvailability[index];
	}

	template <typename TEvent> std::vector<CallbackEntry<TEvent>>& GetCallbacks()
	{
		return std::get<std::vector<CallbackEntry<TEvent>>>(m_callbacks);
	}

	template <typename TEvent> std::vector<RoutedInputEvent<TEvent>>& GetDeferredQueue()
	{
		return std::get<std::vector<RoutedInputEvent<TEvent>>>(m_deferredQueues);
	}

	template <typename TEvent> void DispatchToCallbacks(
	    const TEvent& event,
	    DispatchMode targetMode,
	    InputLayer targetLayer,
	    const LayerAvailability& layerAvailability)
	{
		// Registry changes affect the next dispatch; nested dispatches take a fresh snapshot.
		std::vector<InputCallback<TEvent>> callbacks;
		for (const auto& entry : GetCallbacks<TEvent>())
		{
			if (entry.Mode == targetMode && ShouldDispatchToLayer(entry.Layer, targetLayer, layerAvailability) && entry.Callback)
			{
				callbacks.push_back(entry.Callback);
			}
		}
		for (const auto& callback : callbacks)
		{
			callback(event);
		}
	}

	template <typename TEvent> void ProcessDeferredEventsForType(const LayerAvailability& layerAvailability)
	{
		auto& queue = GetDeferredQueue<TEvent>();
		for (std::size_t index = 0; index < queue.size(); ++index)
		{
			const RoutedInputEvent<TEvent> routedEvent = queue[index];
			DispatchToCallbacks(routedEvent.Event, DispatchMode::Deferred, routedEvent.TargetLayer, layerAvailability);
		}
		queue.clear();
	}

	CallbackTuple m_callbacks;
	DeferredQueueTuple m_deferredQueues;
	std::uint32_t m_nextCallbackId = 1;
	bool m_isProcessingDeferredEvents = false;
};
