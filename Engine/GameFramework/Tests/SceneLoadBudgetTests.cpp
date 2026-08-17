#include "Level/Loading/SceneLoadBudget.h"

#include <atomic>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace SceneLoadBudgetTests
{
	void Require(bool condition, std::string_view message)
	{
		if (!condition)
		{
			throw std::runtime_error(std::string(message));
		}
	}

	void AggregateReservationsAreBounded()
	{
		Assets::SceneLoadBudget budget(10);
		Require(budget.TryReserve(6), "Initial reservation was rejected.");
		Require(!budget.TryReserve(5), "Aggregate overflow was accepted.");
		Require(budget.GetRetainedBytes() == 6, "Rejected reservation changed the retained byte count.");
		budget.Release(4);
		Require(budget.TryReserve(5), "Released capacity was not reusable.");
		Require(budget.GetRetainedBytes() == 7, "Retained byte count did not track reserve and release operations.");
	}

	void ConcurrentReservationsCannotExceedCapacity()
	{
		constexpr std::size_t ReservationBytes = 64;
		constexpr std::size_t ReservationCount = 32;
		constexpr std::size_t MaximumReservations = 16;
		Assets::SceneLoadBudget budget(ReservationBytes * MaximumReservations);
		std::atomic<std::size_t> acceptedReservations = 0;
		std::vector<std::thread> workers;
		workers.reserve(ReservationCount);
		for (std::size_t index = 0; index < ReservationCount; ++index)
		{
			workers.emplace_back(
			    [&budget, &acceptedReservations]()
			    {
				    if (budget.TryReserve(ReservationBytes))
				    {
					    acceptedReservations.fetch_add(1, std::memory_order_relaxed);
				    }
			    });
		}
		for (std::thread& worker : workers)
		{
			worker.join();
		}

		Require(
		    acceptedReservations.load(std::memory_order_relaxed) == MaximumReservations,
		    "Concurrent admission did not stop at capacity.");
		Require(
		    budget.GetRetainedBytes() == budget.GetMaximumBytes(),
		    "Concurrent admission did not produce the expected high-water mark.");
	}

	using TestFunction = void (*)();

	int Run(std::string_view name, TestFunction test)
	{
		try
		{
			test();
			std::cout << "[PASS] " << name << '\n';
			return 0;
		}
		catch (const std::exception& error)
		{
			std::cerr << "[FAIL] " << name << ": " << error.what() << '\n';
			return 1;
		}
	}
}

int main()
{
	using namespace SceneLoadBudgetTests;
	int failureCount = 0;
	failureCount += Run("aggregate reservation bound", AggregateReservationsAreBounded);
	failureCount += Run("concurrent reservation bound", ConcurrentReservationsCannotExceedCapacity);
	return failureCount == 0 ? 0 : 1;
}
