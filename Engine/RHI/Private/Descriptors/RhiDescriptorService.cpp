#include "PCH.h"

#include "Descriptors/RhiDescriptorService.h"

const char* RhiDescriptorAllocatorTypeToString(ERhiDescriptorAllocatorType type) noexcept
{
	switch (type)
	{
		case ERhiDescriptorAllocatorType::ShaderResource:
			return "ShaderResource";
		case ERhiDescriptorAllocatorType::Sampler:
			return "Sampler";
		case ERhiDescriptorAllocatorType::RenderTarget:
			return "RenderTarget";
		case ERhiDescriptorAllocatorType::DepthStencil:
			return "DepthStencil";
	}
	return "Unknown";
}

RhiDescriptorService::~RhiDescriptorService() noexcept = default;
