#include "CapabilityGraph.h"

#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace SparkleLauncher::CapabilityGraphTests
{
	using TestGraph = CapabilityGraph<std::string>;
	using TestEvaluation = CapabilityEvaluation<std::string>;
	using TestDefinition = CapabilityDefinition<std::string>;
	using TestResolution = CapabilityResolution<std::string>;

	void Require(bool condition, std::string_view message)
	{
		if (!condition)
		{
			throw std::runtime_error(std::string(message));
		}
	}

	TestEvaluation Ready()
	{
		return TestEvaluation::Ready();
	}

	TestEvaluation NeedsDependencies(std::string message = {})
	{
		return TestEvaluation::DependenciesRequired(std::move(message));
	}

	TestEvaluation Operation(std::string operationId)
	{
		return TestEvaluation::RunOperation(std::move(operationId));
	}

	TestEvaluation Blocked(std::string message)
	{
		return TestEvaluation::Blocked(std::move(message));
	}

	void RegisterOrThrow(TestGraph& graph, TestDefinition definition)
	{
		const std::string error = graph.Register(std::move(definition));
		Require(error.empty(), error);
	}

	void HierarchyReturnsDeepestUnreadyDependency()
	{
		bool sourceReady = false;
		bool buildReady = false;
		TestGraph graph;
		RegisterOrThrow(graph, {"source", {}, [&sourceReady](bool) { return sourceReady ? Ready() : Operation("sync-source"); }});
		RegisterOrThrow(graph, {"build", {"source"}, [&buildReady](bool) { return buildReady ? Ready() : Operation("build-product"); }});
		RegisterOrThrow(graph, {"run", {"build"}, [](bool) { return Operation("launch-product"); }});

		TestResolution resolution = graph.Resolve("run");
		Require(resolution.Result == TestResolution::Kind::RunOperation, "Source resolution was not actionable.");
		Require(resolution.CapabilityId == "source", "The deepest missing dependency was not selected first.");
		Require(resolution.OperationRequest == "sync-source", "The source preparation operation was not selected.");
		Require(
		    resolution.DependencyPath == std::vector<std::string>({"run", "build", "source"}),
		    "The resolved dependency path was not preserved.");

		sourceReady = true;
		resolution = graph.Resolve("run");
		Require(resolution.CapabilityId == "build", "The product capability did not follow source readiness.");
		Require(resolution.OperationRequest == "build-product", "The product build operation was not selected.");

		buildReady = true;
		resolution = graph.Resolve("run");
		Require(resolution.CapabilityId == "run", "The goal capability was not selected after its dependencies became ready.");
		Require(resolution.CompletesGoal, "The goal operation was not identified as terminal.");
	}

	void ResolutionRejectsStructurallyInvalidReadyGraph()
	{
		TestGraph graph;
		RegisterOrThrow(graph, {"ready", {"not-registered"}, [](bool) { return Ready(); }});
		const TestResolution resolution = graph.Resolve("ready");
		Require(resolution.Result == TestResolution::Kind::Blocked, "Resolution bypassed structural graph validation.");
		Require(resolution.StatusMessage.find("not-registered") != std::string::npos, "Structural validation missed a dependency typo.");
	}

	void ArbitraryDependencyCountIsSupported()
	{
		TestGraph graph;
		std::vector<std::string> dependencyIds;
		for (int index = 0; index < 128; ++index)
		{
			const std::string id = "dependency-" + std::to_string(index);
			dependencyIds.push_back(id);
			RegisterOrThrow(graph, {id, {}, [](bool) { return Ready(); }});
		}
		RegisterOrThrow(graph, {"goal", dependencyIds, [](bool) { return Operation("run-goal"); }});

		const TestResolution resolution = graph.Resolve("goal");
		Require(resolution.OperationRequest == "run-goal", "A large registered dependency set did not resolve.");
	}

	void DirectDependentsAreInvalidatedByTopology()
	{
		TestGraph graph;
		RegisterOrThrow(graph, {"source", {}, [](bool) { return Operation("sync-source"); }});
		RegisterOrThrow(graph, {"direct-consumer", {"source"}, [](bool) { return NeedsDependencies(); }});
		RegisterOrThrow(graph, {"indirect-consumer", {"direct-consumer"}, [](bool) { return NeedsDependencies(); }});

		const TestResolution resolution = graph.Resolve("indirect-consumer");
		Require(resolution.InvalidatedCapabilityIds.contains("direct-consumer"), "A direct dependent was not invalidated.");
		Require(
		    !resolution.InvalidatedCapabilityIds.contains("indirect-consumer"),
		    "Invalidation spread transitively instead of following the next readiness evaluation.");
	}

	void ReadyCapabilitiesSettlePriorInvalidation()
	{
		TestGraph graph;
		RegisterOrThrow(graph, {"dependency", {}, [](bool) { return Ready(); }});
		RegisterOrThrow(graph, {"goal", {"dependency"}, [](bool) { return Operation("run-goal"); }});

		const TestResolution resolution = graph.Resolve("goal", {"dependency"});
		Require(resolution.OperationRequest == "run-goal", "The goal operation was not selected after dependency revalidation.");
		Require(resolution.RevalidatedCapabilityIds.contains("dependency"), "A ready capability did not settle its prior invalidation.");
	}

	void MissingRegistrationBlocksWithEvidence()
	{
		const TestResolution missingGoal = TestGraph().Resolve("missing-goal");
		Require(missingGoal.Result == TestResolution::Kind::Blocked, "An unregistered goal capability was accepted.");
		Require(missingGoal.StatusMessage.find("missing-goal") != std::string::npos, "The missing goal was not identified.");

		TestGraph graph;
		RegisterOrThrow(graph, {"goal", {"missing"}, [](bool) { return NeedsDependencies(); }});
		const TestResolution resolution = graph.Resolve("goal");
		Require(resolution.Result == TestResolution::Kind::Blocked, "A missing dependency registration was accepted.");
		Require(resolution.StatusMessage.find("missing") != std::string::npos, "The missing capability was not identified.");
	}

	void CyclesAreRejected()
	{
		TestGraph graph;
		RegisterOrThrow(graph, {"first", {"second"}, [](bool) { return NeedsDependencies(); }});
		RegisterOrThrow(graph, {"second", {"first"}, [](bool) { return NeedsDependencies(); }});
		Require(graph.Validate().find("cycle") != std::string::npos, "Structural validation missed a dependency cycle.");
		const TestResolution resolution = graph.Resolve("first");
		Require(resolution.Result == TestResolution::Kind::Blocked, "A dependency cycle was accepted.");
		Require(resolution.StatusMessage.find("cycle") != std::string::npos, "The cycle diagnostic was not preserved.");
	}

	void BlockedDependencyStopsParentOperation()
	{
		TestGraph graph;
		RegisterOrThrow(graph, {"machine", {}, [](bool) { return Blocked("Install the required host SDK."); }});
		RegisterOrThrow(graph, {"goal", {"machine"}, [](bool) { return Operation("run-goal"); }});
		const TestResolution resolution = graph.Resolve("goal");
		Require(resolution.Result == TestResolution::Kind::Blocked, "A blocked dependency was ignored.");
		Require(resolution.StatusMessage == "Install the required host SDK.", "The dependency blocker was not propagated.");
	}

	void InvalidRegistrationsAreRejectedAtTheirOwner()
	{
		TestGraph graph;
		Require(
		    graph.Register({"goal", {"dependency", "dependency"}, [](bool) { return Ready(); }}).find("more than once")
		        != std::string::npos,
		    "Duplicate dependency edges were accepted.");
		RegisterOrThrow(graph, {"goal", {}, [](bool) { return Ready(); }});
		Require(
		    graph.Register({"goal", {}, [](bool) { return Ready(); }}).find("more than once") != std::string::npos,
		    "Duplicate capability registration was accepted.");
	}

	void MissingOperationRequestBlocksResolution()
	{
		TestGraph graph;
		RegisterOrThrow(
		    graph,
		    {"goal",
		        {},
		        [](bool)
		        {
			        TestEvaluation evaluation;
			        evaluation.State = CapabilityState::NeedsOperation;
			        return evaluation;
		        }});
		const TestResolution resolution = graph.Resolve("goal");
		Require(resolution.Result == TestResolution::Kind::Blocked, "An operation capability without a request was accepted.");
		Require(
		    resolution.StatusMessage.find("without providing") != std::string::npos,
		    "The missing operation request was not diagnosed.");
	}

	struct NonDefaultRequest
	{
		explicit NonDefaultRequest(int value) :
		    Value(value)
		{
		}

		int Value;
	};

	void RequestsDoNotNeedDefaultConstruction()
	{
		CapabilityGraph<NonDefaultRequest> graph;
		const std::string error =
		    graph.Register({"goal", {}, [](bool) { return CapabilityEvaluation<NonDefaultRequest>::RunOperation(NonDefaultRequest(42)); }});
		Require(error.empty(), error);
		const CapabilityResolution<NonDefaultRequest> resolution = graph.Resolve("goal");
		Require(resolution.OperationRequest.has_value(), "A non-default-constructible request was lost.");
		Require(resolution.OperationRequest->Value == 42, "The non-default-constructible request changed during resolution.");
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
	using namespace SparkleLauncher::CapabilityGraphTests;
	int failureCount = 0;
	failureCount += Run("hierarchical resolution", HierarchyReturnsDeepestUnreadyDependency);
	failureCount += Run("structural validation", ResolutionRejectsStructurallyInvalidReadyGraph);
	failureCount += Run("arbitrary dependency count", ArbitraryDependencyCountIsSupported);
	failureCount += Run("topology invalidation", DirectDependentsAreInvalidatedByTopology);
	failureCount += Run("invalidation settlement", ReadyCapabilitiesSettlePriorInvalidation);
	failureCount += Run("missing registration", MissingRegistrationBlocksWithEvidence);
	failureCount += Run("cycle rejection", CyclesAreRejected);
	failureCount += Run("blocked dependency", BlockedDependencyStopsParentOperation);
	failureCount += Run("registration validation", InvalidRegistrationsAreRejectedAtTheirOwner);
	failureCount += Run("operation request validation", MissingOperationRequestBlocksResolution);
	failureCount += Run("non-default request", RequestsDoNotNeedDefaultConstruction);
	return failureCount == 0 ? 0 : 1;
}
