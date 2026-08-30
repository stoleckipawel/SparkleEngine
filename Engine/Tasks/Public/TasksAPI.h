#pragma once

#if defined(SPARKLE_STATIC)
  #define SPARKLE_TASKS_API
#elif defined(SPARKLE_TASKS_EXPORTS)
  #define SPARKLE_TASKS_API __declspec(dllexport)
#else
  #define SPARKLE_TASKS_API __declspec(dllimport)
#endif
