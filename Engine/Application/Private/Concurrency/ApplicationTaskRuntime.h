#pragma once

#include <memory>

class TaskExecutor;
class TaskScope;
struct TaskExecutorConfig;

class ApplicationTaskRuntime final
{
  public:
	ApplicationTaskRuntime();
	~ApplicationTaskRuntime();

	ApplicationTaskRuntime(const ApplicationTaskRuntime&) = delete;
	ApplicationTaskRuntime& operator=(const ApplicationTaskRuntime&) = delete;

	TaskExecutor& GetExecutor() noexcept;
	TaskScope& GetApplicationScope() noexcept;

  private:
	static TaskExecutorConfig BuildExecutorConfig() noexcept;

	std::unique_ptr<TaskExecutor> m_executor;
	std::unique_ptr<TaskScope> m_applicationScope;
};
