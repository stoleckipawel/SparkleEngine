#include "PCH.h"

#include "Shaders/Authoring/GlobalShader.h"

#include <cstdint>

void RegisterHelloTriangleShaders() noexcept {}

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
