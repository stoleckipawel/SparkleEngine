#include "SparkleLauncher/MaintenanceOperations.h"

namespace SparkleLauncher
{
	const std::vector<std::string>& GetKnownValidationGateTargets()
	{
		static const std::vector<std::string> targets = [] {
			std::vector<std::string> knownTargets;
			for (const ValidationGateDefinition& definition : GetValidationGateDefinitions())
			{
				knownTargets.push_back(definition.Target);
			}
			return knownTargets;
		}();
		return targets;
	}

	const std::vector<ValidationGateDefinition>& GetValidationGateDefinitions()
	{
		static const std::vector<ValidationGateDefinition> definitions = {
		    {"sparkle_validation_check", "aggregate", "Full Validation Suite", "Runs every current Sparkle CMake validation gate.", "Keep as the default explicit validation target."},
		    {"runtime_cooked_boundary_check", "boundaries", "Runtime Cooked Boundary", "Keeps runtime modules on cooked asset/package consumption instead of tool-time cooking logic.", "Keep; this protects the launcher-first cooked workflow."},
		    {"framegraph_boundary_check", "boundaries", "FrameGraph Boundary", "Protects Renderer FrameGraph public/private ownership and compiler/execution separation.", "Keep; this is a high-value architecture boundary."},
		    {"rhi_backend_boundary_check", "boundaries", "RHI Backend Boundary", "Keeps backend-neutral engine layers free of D3D12/Vulkan implementation leakage.", "Keep; this is core renderer architecture."},
		    {"rhi_memory_boundary_check", "boundaries", "RHI Memory Boundary", "Prevents D3D12MA/VMA allocator details from leaking outside backend-owned memory code.", "Keep; this protects backend isolation."},
		    {"shader_compiler_boundary_check", "boundaries", "Shader Compiler Boundary", "Keeps compiler backends in ShaderCompiler while runtime consumes cooked shader packages.", "Keep; this directly guards no-runtime-compile policy."},
		    {"texture_cooker_boundary_check", "boundaries", "Texture Cooker Boundary", "Separates runtime texture use, asset conversion, and TextureCooker implementation ownership.", "Keep; this guards the cooked texture pipeline."},
		    {"tools_architecture_boundary_check", "boundaries", "Tools Architecture Boundary", "Keeps tool implementation private and shared tool seams explicit.", "Keep; especially relevant as Sparkle Launcher grows."},
		    {"logging_boundary_check", "boundaries", "Logging Boundary", "Protects repo-wide logging ownership and prevents deleted logging facade patterns returning.", "Keep; it prevents cross-cutting logging drift."},
		    {"rhi_backend_parity_check", "parity", "RHI Backend Parity", "Checks D3D12/Vulkan parity for diagnostics, shader variants, and backend smoke evidence.", "Keep; run when touching RHI/backend behavior."},
		    {"shader_package_parity_check", "parity", "Shader Package Parity", "Checks DXIL/SPIR-V cooked package parity across compiler and runtime selection.", "Keep; run when touching shader cooking or backend format selection."},
		    {"geometry_instancing_readiness_check", "readiness", "Geometry Instancing Readiness", "Guards import/cook/runtime/renderer/shader/editor prerequisites for future instancing work.", "Keep for now; retire or merge only after that roadmap phase is complete."},
		    {"threading_readiness_check", "readiness", "Threading Readiness", "Guards data-flow and ownership seams needed before worker-thread rendering work.", "Keep for now; it documents and enforces future concurrency prerequisites."},
		    {"advanced_feature_readiness_check", "readiness", "Advanced Feature Readiness", "Guards proposal gates and blockers for advanced renderer features.", "Keep for now; revisit when those blockers graduate into concrete feature gates."},
		};
		return definitions;
	}

	const std::vector<ValidationGateGroupDefinition>& GetValidationGateGroupDefinitions()
	{
		static const std::vector<ValidationGateGroupDefinition> definitions = {
		    {"aggregate", "Aggregate", "Convenience target that runs the complete current validation suite.", {"sparkle_validation_check"}},
		    {"boundaries", "Boundary Invariants", "Architecture boundary checks that are useful on demand while refactoring launcher, runtime, renderer, RHI, cooking, tools, or logging code.", {"runtime_cooked_boundary_check", "framegraph_boundary_check", "rhi_backend_boundary_check", "rhi_memory_boundary_check", "shader_compiler_boundary_check", "texture_cooker_boundary_check", "tools_architecture_boundary_check", "logging_boundary_check"}},
		    {"parity", "Backend And Package Parity", "Cross-backend and cross-format equivalence checks for RHI and cooked shader packages.", {"rhi_backend_parity_check", "shader_package_parity_check"}},
		    {"readiness", "Future Feature Readiness", "Roadmap guardrails for features that are prepared but not always active day to day.", {"geometry_instancing_readiness_check", "threading_readiness_check", "advanced_feature_readiness_check"}},
		};
		return definitions;
	}
}