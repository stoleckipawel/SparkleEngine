#include "PCH.h"

#include "Shaders/Authoring/GlobalShader.h"

#include <string_view>

void RegisterHelloTriangleShaders() noexcept
{
}

class HelloTriangleVS final : public TGlobalShader<HelloTriangleVS>
{
  public:
	static constexpr std::string_view kShaderName = "HelloTriangleVS";
	static constexpr std::string_view kShaderPackageName = "HelloTriangle";
	static constexpr std::string_view kBindingLayoutId = "Empty";

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER(HelloTriangleVS, "HelloWorld/HelloTriangle.hlsl", "VSMain", Vertex);

class HelloTrianglePS final : public TGlobalShader<HelloTrianglePS>
{
  public:
	static constexpr std::string_view kShaderName = "HelloTrianglePS";
	static constexpr std::string_view kShaderPackageName = "HelloTriangle";
	static constexpr std::string_view kBindingLayoutId = "Empty";

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER(HelloTrianglePS, "HelloWorld/HelloTriangle.hlsl", "PSMain", Pixel);