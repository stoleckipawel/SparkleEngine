#pragma once

#include "../Capture/RhiCaptureService.h"
#include "../Core/RhiBackendApi.h"
#include "../Core/RhiCapabilities.h"
#include "../Descriptors/RhiDescriptorService.h"
#include "../Diagnostics/RhiDiagnostics.h"
#include "../Interop/RhiInteropService.h"
#include "../Pipeline/RhiPipelineService.h"
#include "../Presentation/RhiPresentationService.h"
#include "../RayTracing/RhiRayTracingService.h"
#include "../Resources/RhiResourceService.h"
#include "../Resources/RhiUploadService.h"
#include "../RHIAPI.h"

#include <cstdint>

class SPARKLE_RHI_API RenderHardwareInterface
{
  public:
	virtual ~RenderHardwareInterface() noexcept = default;

	virtual const RhiCapabilities& GetCapabilities() const noexcept = 0;
	virtual ERhiBackendApi GetBackendApi() const noexcept = 0;
	virtual CookedShaderBinaryFormat GetRequiredShaderBinaryFormat() const noexcept = 0;
	virtual std::uint32_t GetCurrentFrameIndex() const noexcept = 0;

	virtual RhiResourceService& GetResourceService() noexcept = 0;
	virtual const RhiResourceService& GetResourceService() const noexcept = 0;
	virtual RhiDescriptorService& GetDescriptorService() noexcept = 0;
	virtual const RhiDescriptorService& GetDescriptorService() const noexcept = 0;
	virtual RhiPipelineService& GetPipelineService() noexcept = 0;
	virtual RhiUploadService& GetUploadService() noexcept = 0;
	virtual const RhiUploadService& GetUploadService() const noexcept = 0;
	virtual RhiRayTracingService& GetRayTracingService() noexcept = 0;
	virtual const RhiRayTracingService& GetRayTracingService() const noexcept = 0;
	virtual RhiInteropService& GetInteropService() noexcept = 0;
	virtual const RhiInteropService& GetInteropService() const noexcept = 0;
	virtual RhiCaptureService& GetCaptureService() noexcept = 0;
	virtual RenderDiagnostics& GetDiagnostics() noexcept = 0;
	virtual const RenderDiagnostics& GetDiagnostics() const noexcept = 0;
	virtual RhiPresentationService& GetPresentationService() noexcept = 0;
	virtual const RhiPresentationService& GetPresentationService() const noexcept = 0;
};
