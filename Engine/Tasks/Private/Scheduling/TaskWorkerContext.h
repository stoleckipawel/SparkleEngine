#pragma once

class TaskWorkerContext final
{
  public:
	static void Enter(const void* executorIdentity) noexcept;
	static void Leave() noexcept;
	static bool IsWorkerFor(const void* executorIdentity) noexcept;

  private:
	static thread_local const void* s_executorIdentity;
};
