#pragma once

#include "Diagnostics/RhiExternalCapture.h"

#include <filesystem>
#include <memory>
#include <string>

struct ExternalCaptureNativeTarget final
{
	void* Root = nullptr;
	void* Window = nullptr;
};

struct ExternalCaptureNativeResult final
{
	ExternalCaptureState State = ExternalCaptureState::Capturing;
	std::filesystem::path Artifact;
	std::string Message;
};

class ExternalCaptureAdapter
{
public:
	virtual ~ExternalCaptureAdapter() noexcept = default;

	virtual bool Begin(const ExternalCaptureNativeTarget& target, const std::filesystem::path& path, std::string& error) = 0;
	virtual void End(const ExternalCaptureNativeTarget& target) noexcept = 0;
	virtual ExternalCaptureNativeResult Poll() = 0;
};

std::unique_ptr<ExternalCaptureAdapter> CreatePixCaptureAdapter(std::string& error);
std::unique_ptr<ExternalCaptureAdapter> CreateNsightCaptureAdapter(std::string& error);
std::unique_ptr<ExternalCaptureAdapter> CreateRenderDocCaptureAdapter(std::string& error);
std::unique_ptr<ExternalCaptureAdapter> CreateExternalCaptureAdapter(ERhiBackendApi api, ExternalCaptureProvider provider, std::string& error);

bool IsPixCaptureInstalled() noexcept;
bool IsNsightCaptureInstalled() noexcept;
bool IsRenderDocCaptureInstalled() noexcept;

class RhiExternalCaptureBinding final
{
public:
	static void Bind(RhiExternalCapture& capture, void* root, void* window) noexcept { capture.Bind(root, window); }
};
