#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace SparkleLauncher
{
	enum class CapabilityState : std::uint8_t
	{
		Ready,
		NeedsDependencies,
		NeedsOperation,
		Blocked
	};

	template <typename RequestT> struct CapabilityEvaluation
	{
		CapabilityState State = CapabilityState::Blocked;
		std::optional<RequestT> OperationRequest;
		std::string StatusMessage;

		static CapabilityEvaluation Ready()
		{
			CapabilityEvaluation evaluation;
			evaluation.State = CapabilityState::Ready;
			return evaluation;
		}

		static CapabilityEvaluation DependenciesRequired(std::string statusMessage = {})
		{
			CapabilityEvaluation evaluation;
			evaluation.State = CapabilityState::NeedsDependencies;
			evaluation.StatusMessage = std::move(statusMessage);
			return evaluation;
		}

		static CapabilityEvaluation RunOperation(RequestT request)
		{
			CapabilityEvaluation evaluation;
			evaluation.State = CapabilityState::NeedsOperation;
			evaluation.OperationRequest = std::move(request);
			return evaluation;
		}

		static CapabilityEvaluation Blocked(std::string statusMessage)
		{
			CapabilityEvaluation evaluation;
			evaluation.StatusMessage = std::move(statusMessage);
			return evaluation;
		}
	};

	template <typename RequestT> struct CapabilityDefinition
	{
		std::string Id;
		std::vector<std::string> DependencyIds;
		std::function<CapabilityEvaluation<RequestT>()> Evaluate;
	};

	template <typename RequestT> struct CapabilityResolution
	{
		enum class Kind : std::uint8_t
		{
			Ready,
			RunOperation,
			Blocked
		};

		Kind Result = Kind::Blocked;
		std::string CapabilityId;
		std::vector<std::string> DependencyPath;
		std::optional<RequestT> OperationRequest;
		std::string StatusMessage;
		bool CompletesGoal = false;
	};

	template <typename RequestT> class CapabilityGraph final
	{
	public:
		std::string Register(CapabilityDefinition<RequestT> definition)
		{
			if (definition.Id.empty())
			{
				return "A capability cannot be registered without an id.";
			}
			if (!definition.Evaluate)
			{
				return "Capability " + definition.Id + " has no readiness evaluator.";
			}
			if (m_definitions.contains(definition.Id))
			{
				return "Capability " + definition.Id + " was registered more than once.";
			}

			std::set<std::string> uniqueDependencyIds;
			for (const std::string& dependencyId : definition.DependencyIds)
			{
				if (dependencyId.empty())
				{
					return "Capability " + definition.Id + " contains an empty dependency id.";
				}
				if (!uniqueDependencyIds.insert(dependencyId).second)
				{
					return "Capability " + definition.Id + " registers dependency " + dependencyId + " more than once.";
				}
			}

			const std::string capabilityId = definition.Id;
			m_definitions.emplace(capabilityId, std::move(definition));
			return {};
		}

		CapabilityResolution<RequestT> Resolve(const std::string& goalCapabilityId) const
		{
			if (!m_definitions.contains(goalCapabilityId))
			{
				return BlockedResolution(
				    goalCapabilityId,
				    "Required capability is not registered: " + goalCapabilityId + ".",
				    {goalCapabilityId});
			}

			const std::string validationError = Validate();
			if (!validationError.empty())
			{
				return BlockedResolution(goalCapabilityId, validationError, {goalCapabilityId});
			}

			std::set<std::string> resolved;
			return ResolveCapability(goalCapabilityId, goalCapabilityId, resolved, {});
		}

		std::string Validate() const
		{
			std::set<std::string> visiting;
			std::set<std::string> validated;
			for (const auto& [capabilityId, definition] : m_definitions)
			{
				(void) definition;
				const std::string error = ValidateCapability(capabilityId, visiting, validated);
				if (!error.empty())
				{
					return error;
				}
			}
			return {};
		}

	private:
		static CapabilityResolution<RequestT> ReadyResolution(const std::string& capabilityId, std::vector<std::string> dependencyPath)
		{
			CapabilityResolution<RequestT> resolution;
			resolution.Result = CapabilityResolution<RequestT>::Kind::Ready;
			resolution.CapabilityId = capabilityId;
			resolution.DependencyPath = std::move(dependencyPath);
			return resolution;
		}

		static CapabilityResolution<RequestT> BlockedResolution(
		    const std::string& capabilityId,
		    std::string message,
		    std::vector<std::string> dependencyPath)
		{
			CapabilityResolution<RequestT> resolution;
			resolution.CapabilityId = capabilityId;
			resolution.DependencyPath = std::move(dependencyPath);
			resolution.StatusMessage = std::move(message);
			return resolution;
		}

		std::string ValidateCapability(
		    const std::string& capabilityId,
		    std::set<std::string>& visiting,
		    std::set<std::string>& validated) const
		{
			if (validated.contains(capabilityId))
			{
				return {};
			}
			if (visiting.contains(capabilityId))
			{
				return "Capability dependency cycle detected at " + capabilityId + ".";
			}

			const auto found = m_definitions.find(capabilityId);
			if (found == m_definitions.end())
			{
				return "Required capability is not registered: " + capabilityId + ".";
			}

			visiting.insert(capabilityId);
			for (const std::string& dependencyId : found->second.DependencyIds)
			{
				const std::string error = ValidateCapability(dependencyId, visiting, validated);
				if (!error.empty())
				{
					visiting.erase(capabilityId);
					return error;
				}
			}
			visiting.erase(capabilityId);
			validated.insert(capabilityId);
			return {};
		}

		CapabilityResolution<RequestT> ResolveCapability(
		    const std::string& capabilityId,
		    const std::string& goalCapabilityId,
		    std::set<std::string>& resolved,
		    std::vector<std::string> dependencyPath) const
		{
			dependencyPath.push_back(capabilityId);
			if (resolved.contains(capabilityId))
			{
				return ReadyResolution(capabilityId, std::move(dependencyPath));
			}
			const CapabilityDefinition<RequestT>& definition = m_definitions.at(capabilityId);
			CapabilityEvaluation<RequestT> evaluation = definition.Evaluate();
			if (evaluation.State == CapabilityState::Ready)
			{
				resolved.insert(capabilityId);
				return ReadyResolution(capabilityId, std::move(dependencyPath));
			}
			if (evaluation.State == CapabilityState::Blocked)
			{
				return BlockedResolution(capabilityId, std::move(evaluation.StatusMessage), std::move(dependencyPath));
			}

			for (const std::string& dependencyId : definition.DependencyIds)
			{
				CapabilityResolution<RequestT> dependency = ResolveCapability(dependencyId, goalCapabilityId, resolved, dependencyPath);
				if (dependency.Result != CapabilityResolution<RequestT>::Kind::Ready)
				{
					return dependency;
				}
			}

			evaluation = definition.Evaluate();
			if (evaluation.State == CapabilityState::Ready)
			{
				resolved.insert(capabilityId);
				return ReadyResolution(capabilityId, std::move(dependencyPath));
			}
			if (evaluation.State == CapabilityState::NeedsOperation)
			{
				if (!evaluation.OperationRequest.has_value())
				{
					return BlockedResolution(
					    capabilityId,
					    "Capability " + capabilityId + " requested an operation without providing its request.",
					    std::move(dependencyPath));
				}

				CapabilityResolution<RequestT> resolution;
				resolution.Result = CapabilityResolution<RequestT>::Kind::RunOperation;
				resolution.CapabilityId = capabilityId;
				resolution.DependencyPath = std::move(dependencyPath);
				resolution.OperationRequest = std::move(evaluation.OperationRequest);
				resolution.StatusMessage = std::move(evaluation.StatusMessage);
				resolution.CompletesGoal = capabilityId == goalCapabilityId;
				return resolution;
			}
			if (evaluation.State == CapabilityState::Blocked)
			{
				return BlockedResolution(capabilityId, std::move(evaluation.StatusMessage), std::move(dependencyPath));
			}

			const std::string message = evaluation.StatusMessage.empty()
			    ? "Capability " + capabilityId + " is still unavailable after its registered dependencies became ready."
			    : std::move(evaluation.StatusMessage);
			return BlockedResolution(capabilityId, message, std::move(dependencyPath));
		}

		std::map<std::string, CapabilityDefinition<RequestT>> m_definitions;
	};
}
