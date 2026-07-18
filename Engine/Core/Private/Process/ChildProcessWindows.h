#pragma once

#include "Process/ChildProcess.h"

namespace Process::Detail
{
	ChildProcessResult RunWindowsChildProcess(const ChildProcessRequest& request);
}
