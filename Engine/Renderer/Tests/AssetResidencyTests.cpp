#include "Resources/Residency/AssetResidency.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace AssetResidencyTests
{
	void Require(bool condition, std::string_view message)
	{
		if (!condition)
		{
			throw std::runtime_error(std::string(message));
		}
	}

	void AdmissionIsBoundedAndReusable()
	{
		AssetResidencyBudget budget;
		budget.MaximumRequestBacklog = 2;
		AssetResidency residency(budget);

		const std::optional<AssetGenerationHandle> first = residency.BeginGeneration(1, 1);
		const std::optional<AssetGenerationHandle> second = residency.BeginGeneration(2, 1);
		Require(first.has_value() && second.has_value(), "Valid generations were not admitted.");
		Require(residency.GetCounters().RequestBacklog == 2, "Admitted generations were not charged to the backlog.");
		Require(residency.BeginGeneration(1, 1) == first, "An existing generation could not be resolved while the backlog was full.");
		Require(!residency.BeginGeneration(3, 1), "A generation was admitted beyond the backlog limit.");

		Require(residency.Cancel(*first), "Cancellation did not retire the first generation.");
		Require(residency.GetCounters().RequestBacklog == 1, "Cancellation did not release backlog capacity.");
		const std::optional<AssetGenerationHandle> retried = residency.BeginGeneration(1, 1);
		Require(retried == first, "A cancelled generation could not be admitted again.");
		Require(residency.GetCounters().RequestBacklog == 2, "Re-admission was not charged to the backlog.");
	}

	void OlderGenerationCannotReplaceNewerWork()
	{
		AssetResidencyBudget budget;
		budget.MaximumRequestBacklog = 2;
		AssetResidency residency(budget);
		Require(residency.BeginGeneration(7, 2).has_value(), "Newer generation was not admitted.");
		Require(!residency.BeginGeneration(7, 1), "An older generation replaced newer in-flight work.");
		Require(residency.GetCounters().RequestBacklog == 1, "Rejected stale work changed backlog accounting.");
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
	using namespace AssetResidencyTests;
	int failureCount = 0;
	failureCount += Run("bounded reusable admission", AdmissionIsBoundedAndReusable);
	failureCount += Run("newer generation precedence", OlderGenerationCannotReplaceNewerWork);
	return failureCount == 0 ? 0 : 1;
}
