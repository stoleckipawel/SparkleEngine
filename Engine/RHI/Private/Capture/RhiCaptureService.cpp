#include "Capture/RhiCaptureService.h"

RhiCaptureService::~RhiCaptureService() noexcept = default;

RhiCaptureTicket::operator bool() const noexcept
{
	return Value != 0;
}
