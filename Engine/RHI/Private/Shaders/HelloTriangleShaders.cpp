#include "PCH.h"

#include "Shaders/Authoring/GlobalShader.h"

#include <cstdint>

void RegisterHelloTriangleShaders() noexcept
{
}

class HelloTriangleVS final : public TGlobalShader<HelloTriangleVS>
{
  public:
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER(HelloTriangleVS, "HelloWorld/HelloTriangle.hlsl", "VSMain", Vertex);

class HelloTrianglePS final : public TGlobalShader<HelloTrianglePS>
{
  public:
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER(HelloTrianglePS, "HelloWorld/HelloTriangle.hlsl", "PSMain", Pixel);

enum class HelloPermutationShadeMode : std::uint32_t
{
	VertexColor = 0,
	Warm = 1,
	Cool = 2,
	Count
};

class HelloPermutationVS final : public TGlobalShader<HelloPermutationVS>
{
  public:
	BEGIN_SHADER_PERMUTATION_DOMAIN(FPermutationDomain)
		SHADER_PERMUTATION_BOOL("HELLO_PERMUTATION_USE_TINT")
		SHADER_PERMUTATION_ENUM(HelloPermutationShadeMode, "HELLO_PERMUTATION_SHADE_MODE")
	END_SHADER_PERMUTATION_DOMAIN()

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER(HelloPermutationVS, "HelloWorld/HelloPermutation.hlsl", "VSMain", Vertex);

class HelloPermutationPS final : public TGlobalShader<HelloPermutationPS>
{
  public:
	BEGIN_SHADER_PERMUTATION_DOMAIN(FPermutationDomain)
		SHADER_PERMUTATION_BOOL("HELLO_PERMUTATION_USE_TINT")
		SHADER_PERMUTATION_ENUM(HelloPermutationShadeMode, "HELLO_PERMUTATION_SHADE_MODE")
	END_SHADER_PERMUTATION_DOMAIN()

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER(HelloPermutationPS, "HelloWorld/HelloPermutation.hlsl", "PSMain", Pixel);