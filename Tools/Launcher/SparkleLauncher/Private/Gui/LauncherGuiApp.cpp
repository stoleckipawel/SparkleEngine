#include "LauncherGuiApp.h"

#include "SparkleLauncher/BuildWorkspaceOperations.h"
#include "SparkleLauncher/CookOperations.h"
#include "SparkleLauncher/LaunchOperations.h"
#include "SparkleLauncher/MaintenanceOperations.h"
#include "SparkleLauncher/ProjectDiscovery.h"
#include "SparkleLauncher/RepositoryLocator.h"

#include <algorithm>
#include <filesystem>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#if defined(_WIN32)
	#define NOMINMAX
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#include <Windows.h>
#endif

namespace SparkleLauncher
{
#if defined(_WIN32)
	struct GuiOperation
	{
		std::string Group;
		std::string Id;
		std::string DisplayName;
		std::string Description;
	};

	struct LauncherGuiState
	{
		std::optional<RepositoryRoot> Repository;
		std::vector<SparkleProject> Projects;
		std::string ErrorMessage;
		std::string SelectedProjectId = "Showcase";
		std::string EditorProfile = "DevelopmentEditor";
		std::string RuntimeProfile = "DevelopmentGame";
		std::vector<GuiOperation> Operations;
		std::vector<HWND> OperationButtons;
		HWND ProjectList = nullptr;
		HWND EditorProfileBox = nullptr;
		HWND RuntimeProfileBox = nullptr;
		HWND OutputBox = nullptr;
	};

	static constexpr int kProjectListId = 1001;
	static constexpr int kEditorProfileId = 1002;
	static constexpr int kRuntimeProfileId = 1003;
	static constexpr int kOperationButtonBaseId = 2000;

	static std::wstring ToWide(std::string_view text)
	{
		if (text.empty())
		{
			return {};
		}

		const int requiredLength = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
		if (requiredLength <= 0)
		{
			return std::wstring(text.begin(), text.end());
		}

		std::wstring result(static_cast<std::size_t>(requiredLength), L'\0');
		MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), result.data(), requiredLength);
		return result;
	}

	static void AppendLine(std::ostringstream& output, std::string_view text)
	{
		output << text << "\r\n";
	}

	static void AppendPlanDetails(
	    std::ostringstream& output,
	    const OperationRecord& operation,
	    bool canRun,
	    const std::vector<std::string>& readinessMessages,
	    const std::vector<std::string>& plannedEffects)
	{
		AppendLine(output, operation.DisplayName + std::string(canRun ? " [Ready]" : " [Blocked]"));
		AppendLine(output, "Operation: " + operation.Id);
		if (!operation.LogPath.empty())
		{
			AppendLine(output, "Latest log: " + operation.LogPath.string());
		}
		if (operation.RequiresConfirmation)
		{
			AppendLine(output, "Confirmation required: " + ToString(operation.DestructiveScope));
		}
		AppendLine(output, "");
		for (const std::string& message : readinessMessages)
		{
			AppendLine(output, "Readiness: " + message);
		}
		for (const std::string& effect : plannedEffects)
		{
			AppendLine(output, "Effect: " + effect);
		}
		if (!operation.DryRunText.empty())
		{
			AppendLine(output, "");
			AppendLine(output, operation.DryRunText);
		}
	}

	static std::vector<GuiOperation> CollectOperations()
	{
		std::vector<GuiOperation> operations;
		for (const BuildWorkspaceOperationDefinition& definition : GetBuildWorkspaceOperationDefinitions())
		{
			operations.push_back({definition.Group, definition.Id, definition.DisplayName, definition.Description});
		}
		for (const CookOperationDefinition& definition : GetCookOperationDefinitions())
		{
			operations.push_back({definition.Group, definition.Id, definition.DisplayName, definition.Description});
		}
		for (const MaintenanceOperationDefinition& definition : GetMaintenanceOperationDefinitions())
		{
			operations.push_back({definition.Group, definition.Id, definition.DisplayName, definition.Description});
		}
		for (const LaunchOperationDefinition& definition : GetLaunchOperationDefinitions())
		{
			operations.push_back({definition.Group, definition.Id, definition.DisplayName, definition.Description});
		}
		return operations;
	}

	static std::string ChooseInitialProjectId(const std::vector<SparkleProject>& projects)
	{
		const auto showcase = std::find_if(projects.begin(), projects.end(), [](const SparkleProject& project) {
			return project.Id == "Showcase";
		});
		if (showcase != projects.end())
		{
			return showcase->Id;
		}
		return projects.empty() ? std::string() : projects.front().Id;
	}

	static std::string GetSelectedComboText(HWND comboBox, std::string fallback)
	{
		const int selectedIndex = static_cast<int>(SendMessageW(comboBox, CB_GETCURSEL, 0, 0));
		if (selectedIndex == CB_ERR)
		{
			return fallback;
		}

		wchar_t buffer[128] = {};
		SendMessageW(comboBox, CB_GETLBTEXT, static_cast<WPARAM>(selectedIndex), reinterpret_cast<LPARAM>(buffer));
		const int utf8Length = WideCharToMultiByte(CP_UTF8, 0, buffer, -1, nullptr, 0, nullptr, nullptr);
		if (utf8Length <= 1)
		{
			return fallback;
		}

		std::string result(static_cast<std::size_t>(utf8Length - 1), '\0');
		WideCharToMultiByte(CP_UTF8, 0, buffer, -1, result.data(), utf8Length, nullptr, nullptr);
		return result;
	}

	static void RefreshSelections(LauncherGuiState& state)
	{
		if (state.ProjectList != nullptr)
		{
			const int selectedIndex = static_cast<int>(SendMessageW(state.ProjectList, LB_GETCURSEL, 0, 0));
			if (selectedIndex >= 0 && selectedIndex < static_cast<int>(state.Projects.size()))
			{
				state.SelectedProjectId = state.Projects[static_cast<std::size_t>(selectedIndex)].Id;
			}
		}

		state.EditorProfile = GetSelectedComboText(state.EditorProfileBox, state.EditorProfile);
		state.RuntimeProfile = GetSelectedComboText(state.RuntimeProfileBox, state.RuntimeProfile);
	}

	static std::string PreviewOperation(LauncherGuiState& state, std::string_view operationId)
	{
		RefreshSelections(state);
		if (!state.Repository.has_value())
		{
			return "Repository discovery failed. " + state.ErrorMessage;
		}

		std::ostringstream output;
		AppendLine(output, "Sparkle Launcher native GUI preview");
		AppendLine(output, "Project: " + state.SelectedProjectId);
		AppendLine(output, "Editor profile: " + state.EditorProfile);
		AppendLine(output, "Runtime profile: " + state.RuntimeProfile);
		AppendLine(output, "");

		if (FindBuildWorkspaceOperationDefinition(operationId).has_value())
		{
			BuildWorkspaceOperationRequest request;
			request.RepositoryRoot = state.Repository->RootPath;
			request.ProjectId = state.SelectedProjectId;
			request.EditorProfile = state.EditorProfile;
			request.RuntimeProfile = state.RuntimeProfile;
			const BuildWorkspaceOperationPlan plan = PlanBuildWorkspaceOperation(operationId, request);
			AppendPlanDetails(output, plan.Operation, plan.CanRun, plan.ReadinessMessages, plan.PlannedEffects);
			return output.str();
		}

		if (FindCookOperationDefinition(operationId).has_value())
		{
			CookOperationRequest request;
			request.RepositoryRoot = state.Repository->RootPath;
			request.ProjectId = state.SelectedProjectId;
			request.RuntimeProfile = state.RuntimeProfile;
			const CookOperationPlan plan = PlanCookOperation(operationId, request);
			AppendPlanDetails(output, plan.Operation, plan.CanRun, plan.ReadinessMessages, plan.PlannedEffects);
			return output.str();
		}

		if (FindMaintenanceOperationDefinition(operationId).has_value())
		{
			MaintenanceOperationRequest request;
			request.RepositoryRoot = state.Repository->RootPath;
			request.ProjectId = state.SelectedProjectId;
			request.EditorProfile = state.EditorProfile;
			const MaintenanceOperationPlan plan = PlanMaintenanceOperation(operationId, request);
			AppendPlanDetails(output, plan.Operation, plan.CanRun, plan.ReadinessMessages, plan.PlannedEffects);
			return output.str();
		}

		if (FindLaunchOperationDefinition(operationId).has_value())
		{
			LaunchOperationRequest request;
			request.RepositoryRoot = state.Repository->RootPath;
			request.ProjectId = state.SelectedProjectId;
			request.EditorProfile = state.EditorProfile;
			request.RuntimeProfile = state.RuntimeProfile;
			const LaunchOperationPlan plan = PlanLaunchOperation(operationId, request);
			AppendPlanDetails(output, plan.Operation, plan.CanRun, plan.ReadinessMessages, plan.PlannedEffects);
			return output.str();
		}

		return "Unknown launcher operation.";
	}

	static HWND CreateLabel(HWND parent, std::string_view text, int x, int y, int width, int height)
	{
		return CreateWindowExW(0, L"STATIC", ToWide(text).c_str(), WS_CHILD | WS_VISIBLE, x, y, width, height, parent, nullptr, GetModuleHandleW(nullptr), nullptr);
	}

	static void AddComboItem(HWND comboBox, std::string_view text, bool selected)
	{
		const std::wstring wideText = ToWide(text);
		const LRESULT index = SendMessageW(comboBox, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(wideText.c_str()));
		if (selected)
		{
			SendMessageW(comboBox, CB_SETCURSEL, static_cast<WPARAM>(index), 0);
		}
	}

	static void SetOutputText(LauncherGuiState& state, std::string_view text)
	{
		SetWindowTextW(state.OutputBox, ToWide(text).c_str());
	}

	static void CreateLauncherControls(HWND window, LauncherGuiState& state)
	{
		CreateLabel(window, "Sparkle Launcher", 20, 18, 220, 28);
		CreateLabel(window, state.Repository.has_value() ? "Repository: " + state.Repository->RootPath.string() : "Repository: not found", 20, 48, 1080, 22);
		CreateLabel(window, "Projects", 20, 88, 220, 22);

		state.ProjectList = CreateWindowExW(
		    WS_EX_CLIENTEDGE,
		    L"LISTBOX",
		    L"",
		    WS_CHILD | WS_VISIBLE | LBS_NOTIFY,
		    20,
		    114,
		    260,
		    154,
		    window,
		    reinterpret_cast<HMENU>(static_cast<INT_PTR>(kProjectListId)),
		    GetModuleHandleW(nullptr),
		    nullptr);

		for (const SparkleProject& project : state.Projects)
		{
			const std::wstring text = ToWide(project.DisplayName);
			const LRESULT index = SendMessageW(state.ProjectList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(text.c_str()));
			if (project.Id == state.SelectedProjectId)
			{
				SendMessageW(state.ProjectList, LB_SETCURSEL, static_cast<WPARAM>(index), 0);
			}
		}

		CreateLabel(window, "Editor Profile", 20, 286, 120, 22);
		state.EditorProfileBox = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 20, 312, 260, 160, window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kEditorProfileId)), GetModuleHandleW(nullptr), nullptr);
		AddComboItem(state.EditorProfileBox, "DevelopmentEditor", true);
		AddComboItem(state.EditorProfileBox, "DebugEditor", false);
		AddComboItem(state.EditorProfileBox, "ShippingEditor", false);

		CreateLabel(window, "Runtime / Cook Profile", 20, 352, 180, 22);
		state.RuntimeProfileBox = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 20, 378, 260, 160, window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kRuntimeProfileId)), GetModuleHandleW(nullptr), nullptr);
		AddComboItem(state.RuntimeProfileBox, "DevelopmentGame", true);
		AddComboItem(state.RuntimeProfileBox, "DebugGame", false);
		AddComboItem(state.RuntimeProfileBox, "ShippingGame", false);

		CreateLabel(window, "Operations", 320, 88, 220, 22);
		int x = 320;
		int y = 114;
		std::string currentGroup;
		for (std::size_t index = 0; index < state.Operations.size(); ++index)
		{
			const GuiOperation& operation = state.Operations[index];
			if (operation.Group != currentGroup)
			{
				currentGroup = operation.Group;
				CreateLabel(window, currentGroup, x, y, 180, 20);
				y += 24;
			}

			HWND button = CreateWindowExW(
			    0,
			    L"BUTTON",
			    ToWide(operation.DisplayName).c_str(),
			    WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			    x,
			    y,
			    210,
			    30,
			    window,
			    reinterpret_cast<HMENU>(static_cast<INT_PTR>(kOperationButtonBaseId + index)),
			    GetModuleHandleW(nullptr),
			    nullptr);
			state.OperationButtons.push_back(button);
			y += 36;
			if (y > 680)
			{
				y = 114;
				x += 230;
			}
		}

		CreateLabel(window, "Preview / Job Output", 810, 88, 240, 22);
		state.OutputBox = CreateWindowExW(
		    WS_EX_CLIENTEDGE,
		    L"EDIT",
		    L"",
		    WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL,
		    810,
		    114,
		    520,
		    566,
		    window,
		    nullptr,
		    GetModuleHandleW(nullptr),
		    nullptr);

		if (state.Repository.has_value())
		{
			SetOutputText(state, "Select an operation to preview its native SparkleLauncherCore plan.");
		}
		else
		{
			SetOutputText(state, "Repository discovery failed. " + state.ErrorMessage);
		}
	}

	static LRESULT CALLBACK LauncherWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
	{
		LauncherGuiState* state = reinterpret_cast<LauncherGuiState*>(GetWindowLongPtrW(window, GWLP_USERDATA));
		switch (message)
		{
		case WM_NCCREATE:
		{
			const CREATESTRUCTW* createStruct = reinterpret_cast<const CREATESTRUCTW*>(lParam);
			SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(createStruct->lpCreateParams));
			return TRUE;
		}
		case WM_CREATE:
			if (state != nullptr)
			{
				CreateLauncherControls(window, *state);
			}
			return 0;
		case WM_COMMAND:
			if (state != nullptr)
			{
				const int controlId = LOWORD(wParam);
				if (controlId >= kOperationButtonBaseId)
				{
					const std::size_t operationIndex = static_cast<std::size_t>(controlId - kOperationButtonBaseId);
					if (operationIndex < state->Operations.size())
					{
						SetOutputText(*state, PreviewOperation(*state, state->Operations[operationIndex].Id));
					}
				}
			}
			return 0;
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		}

		return DefWindowProcW(window, message, wParam, lParam);
	}

	static LauncherGuiState CreateInitialState()
	{
		LauncherGuiState state;
		state.Operations = CollectOperations();

		std::string errorMessage;
		state.Repository = TryFindRepositoryRoot(std::filesystem::current_path(), errorMessage);
		if (!state.Repository.has_value())
		{
			state.ErrorMessage = errorMessage;
			return state;
		}

		state.Projects = DiscoverProjects(state.Repository->RootPath, errorMessage);
		state.ErrorMessage = errorMessage;
		state.SelectedProjectId = ChooseInitialProjectId(state.Projects);
		return state;
	}

	int RunLauncherGui()
	{
		LauncherGuiState state = CreateInitialState();
		const HINSTANCE instance = GetModuleHandleW(nullptr);
		const wchar_t* className = L"SparkleLauncherWindow";

		WNDCLASSEXW windowClass = {};
		windowClass.cbSize = sizeof(windowClass);
		windowClass.lpfnWndProc = LauncherWindowProc;
		windowClass.hInstance = instance;
		windowClass.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
		windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
		windowClass.lpszClassName = className;
		RegisterClassExW(&windowClass);
 
		HWND window = CreateWindowExW(
		    0,
		    className,
		    L"Sparkle Launcher",
		    WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		    CW_USEDEFAULT,
		    CW_USEDEFAULT,
		    1360,
		    760,
		    nullptr,
		    nullptr,
		    instance,
		    &state);

		if (window == nullptr)
		{
			return 1;
		}

		MSG message = {};
		while (GetMessageW(&message, nullptr, 0, 0) > 0)
		{
			TranslateMessage(&message);
			DispatchMessageW(&message);
		}

		return static_cast<int>(message.wParam);
	}
#else
	int RunLauncherGui()
	{
		return 1;
	}
#endif
}