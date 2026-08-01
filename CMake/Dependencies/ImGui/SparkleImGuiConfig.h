#pragma once

struct ImGuiContext;

extern thread_local ImGuiContext* SparkleImGuiThreadContext;

#define GImGui SparkleImGuiThreadContext
