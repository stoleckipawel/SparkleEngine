#include "PCH.h"

#include "Shaders/Authoring/GlobalShader.h"

#include <cstdint>
#include <string_view>

class HelloRayGen final : public FRayGenerationShader
{
  public:
	static constexpr std::string_view kShaderName = "HelloRayGen";
	static constexpr std::string_view kShaderPackageName = "HelloRayTracingLibrary";
	static constexpr std::string_view kBindingLayoutId = "HelloRayTracingLibrary";
	static constexpr std::uint32_t kRayTracingPayloadSizeInBytes = 16;
	static constexpr std::uint32_t kRayTracingAttributeSizeInBytes = 8;
	static constexpr std::uint32_t kRayTracingMaxRecursionDepth = 1;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_ACCELERATION_STRUCTURE(SceneAccelerationStructure)
	SHADER_PARAMETER_UAV(RWTexture2D, OutputTexture)
	END_SHADER_PARAMETER_STRUCT()
};

class HelloMiss final : public FMissShader
{
  public:
	static constexpr std::string_view kShaderName = "HelloMiss";
	static constexpr std::string_view kShaderPackageName = "HelloRayTracingLibrary";
	static constexpr std::string_view kBindingLayoutId = "HelloRayTracingLibrary";
	static constexpr std::uint32_t kRayTracingPayloadSizeInBytes = 16;
	static constexpr std::uint32_t kRayTracingAttributeSizeInBytes = 8;
	static constexpr std::uint32_t kRayTracingMaxRecursionDepth = 1;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	END_SHADER_PARAMETER_STRUCT()
};

class HelloClosestHit final : public FClosestHitShader
{
  public:
	static constexpr std::string_view kShaderName = "HelloClosestHit";
	static constexpr std::string_view kShaderPackageName = "HelloRayTracingLibrary";
	static constexpr std::string_view kBindingLayoutId = "HelloRayTracingLibrary";
	static constexpr std::uint32_t kRayTracingPayloadSizeInBytes = 16;
	static constexpr std::uint32_t kRayTracingAttributeSizeInBytes = 8;
	static constexpr std::uint32_t kRayTracingMaxRecursionDepth = 1;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	END_SHADER_PARAMETER_STRUCT()
};

class HelloAnyHit final : public FAnyHitShader
{
  public:
	static constexpr std::string_view kShaderName = "HelloAnyHit";
	static constexpr std::string_view kShaderPackageName = "HelloRayTracingLibrary";
	static constexpr std::string_view kBindingLayoutId = "HelloRayTracingLibrary";
	static constexpr std::uint32_t kRayTracingPayloadSizeInBytes = 16;
	static constexpr std::uint32_t kRayTracingAttributeSizeInBytes = 8;
	static constexpr std::uint32_t kRayTracingMaxRecursionDepth = 1;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	END_SHADER_PARAMETER_STRUCT()
};

class HelloIntersection final : public FIntersectionShader
{
  public:
	static constexpr std::string_view kShaderName = "HelloIntersection";
	static constexpr std::string_view kShaderPackageName = "HelloRayTracingLibrary";
	static constexpr std::string_view kBindingLayoutId = "HelloRayTracingLibrary";
	static constexpr std::uint32_t kRayTracingPayloadSizeInBytes = 16;
	static constexpr std::uint32_t kRayTracingAttributeSizeInBytes = 8;
	static constexpr std::uint32_t kRayTracingMaxRecursionDepth = 1;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	END_SHADER_PARAMETER_STRUCT()
};

class HelloCallable final : public FCallableShader
{
  public:
	static constexpr std::string_view kShaderName = "HelloCallable";
	static constexpr std::string_view kShaderPackageName = "HelloRayTracingLibrary";
	static constexpr std::string_view kBindingLayoutId = "HelloRayTracingLibrary";
	static constexpr std::uint32_t kRayTracingPayloadSizeInBytes = 16;
	static constexpr std::uint32_t kRayTracingAttributeSizeInBytes = 8;
	static constexpr std::uint32_t kRayTracingMaxRecursionDepth = 1;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	END_SHADER_PARAMETER_STRUCT()
};

class HelloPrimaryHitGroup final : public FRayTracingHitGroup
{
  public:
	static constexpr std::string_view kShaderPackageName = "HelloRayTracingLibrary";
	static constexpr std::string_view kHitGroupName = "HelloPrimaryHitGroup";

	using ClosestHit = HelloClosestHit;
	using AnyHit = HelloAnyHit;
	using Intersection = HelloIntersection;
};

IMPLEMENT_RAY_TRACING_SHADER(HelloRayGen, "HelloWorld/RayTracing/HelloRayGen.hlsl", "HelloRayGen");

IMPLEMENT_RAY_TRACING_SHADER(HelloMiss, "HelloWorld/RayTracing/HelloMiss.hlsl", "HelloMiss");

IMPLEMENT_RAY_TRACING_SHADER(HelloClosestHit, "HelloWorld/RayTracing/HelloClosestHit.hlsl", "HelloClosestHit");

IMPLEMENT_RAY_TRACING_SHADER(HelloAnyHit, "HelloWorld/RayTracing/HelloAnyHit.hlsl", "HelloAnyHit");

IMPLEMENT_RAY_TRACING_SHADER(HelloIntersection, "HelloWorld/RayTracing/HelloIntersection.hlsl", "HelloIntersection");

IMPLEMENT_RAY_TRACING_SHADER(HelloCallable, "HelloWorld/RayTracing/HelloCallable.hlsl", "HelloCallable");

IMPLEMENT_RAY_TRACING_HIT_GROUP(HelloPrimaryHitGroup);

void RegisterHelloRayGenShader() noexcept
{
	(void)AutoRegisterRayTracingShader_HelloRayGen;
}

void RegisterHelloMissShader() noexcept
{
	(void)AutoRegisterRayTracingShader_HelloMiss;
}

void RegisterHelloClosestHitShader() noexcept
{
	(void)AutoRegisterRayTracingShader_HelloClosestHit;
}

void RegisterHelloAnyHitShader() noexcept
{
	(void)AutoRegisterRayTracingShader_HelloAnyHit;
}

void RegisterHelloIntersectionShader() noexcept
{
	(void)AutoRegisterRayTracingShader_HelloIntersection;
}

void RegisterHelloCallableShader() noexcept
{
	(void)AutoRegisterRayTracingShader_HelloCallable;
}

void RegisterHelloPrimaryHitGroup() noexcept
{
	(void)AutoRegisterRayTracingHitGroup_HelloPrimaryHitGroup;
}
