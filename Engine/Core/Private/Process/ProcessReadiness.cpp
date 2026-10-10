#include "PCH.h"

#include "Process/ProcessReadiness.h"

#include "Core/Public/Environment/EnvironmentVariables.h"
#include "Process/ProcessReadinessEnvironment.h"

#include <string>

void Process::SignalParentReadiness(std::string_view value) noexcept
{
#if defined(_WIN32)
	std::string expectedValue;
	std::string eventName;
	if (!Environment::TryGetVariable(Detail::ReadinessValueEnvironmentVariable, expectedValue) || !Environment::TryGetVariable(Detail::ReadinessEventEnvironmentVariable, eventName)
	    || value != expectedValue)
	{
		return;
	}

	const int nameLength = MultiByteToWideChar(CP_UTF8, 0, eventName.data(), static_cast<int>(eventName.size()), nullptr, 0);
	if (nameLength <= 0)
	{
		return;
	}
	std::wstring wideEventName(static_cast<std::size_t>(nameLength), L'\0');
	MultiByteToWideChar(CP_UTF8, 0, eventName.data(), static_cast<int>(eventName.size()), wideEventName.data(), nameLength);

	const HANDLE eventHandle = OpenEventW(EVENT_MODIFY_STATE, FALSE, wideEventName.c_str());
	if (eventHandle == nullptr)
	{
		return;
	}
	SetEvent(eventHandle);
	CloseHandle(eventHandle);
#else
	(void) value;
#endif
}
