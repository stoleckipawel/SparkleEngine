#pragma once

#include "SparkleLauncher/OperationModel.h"
#include "SparkleLauncher/ProcessRunner.h"

#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace SparkleLauncher
{
	enum class LauncherOperationCategory;
	struct LauncherOperationRequest;

	class LauncherOperationService final
	{
	  public:
		using ProcessRunnerFactory = std::function<std::unique_ptr<IProcessRunner>()>;
		using OutputCallback = std::function<void(std::string_view)>;
		using CompletionCallback = std::function<void(OperationRecord)>;

		explicit LauncherOperationService(ProcessRunnerFactory processRunnerFactory);
		~LauncherOperationService();

		LauncherOperationService(const LauncherOperationService&) = delete;
		LauncherOperationService& operator=(const LauncherOperationService&) = delete;

		void Launch(
		    LauncherOperationCategory category,
		    LauncherOperationRequest request,
		    std::string title,
		    OutputCallback outputCallback,
		    CompletionCallback completionCallback);

	  private:
		struct Implementation;
		std::unique_ptr<Implementation> m_implementation;
	};
}
