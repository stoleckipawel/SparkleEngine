#include "PCH.h"
#include "D3D12/Diagnostics/D3D12DebugLayer.h"

#if ENGINE_GPU_VALIDATION

	#include "Core/Public/Formatting/HexFormat.h"

	#include <format>

static const auto g_d3d12DiagnosticsLogger = Logging::GetOrCreateLogger("RHI.D3D12.Diagnostics");

namespace D3D12DebugLayerDiagnostics
{
	constexpr std::size_t kMaxBreadcrumbNodes = 16;
	constexpr std::size_t kMaxAllocationNodes = 8;

	std::string NarrowString(const wchar_t* text) noexcept
	{
		if (text == nullptr || text[0] == L'\0')
		{
			return {};
		}

		const int requiredSize = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
		if (requiredSize <= 1)
		{
			return {};
		}

		std::string result(static_cast<std::size_t>(requiredSize), '\0');
		WideCharToMultiByte(CP_UTF8, 0, text, -1, result.data(), requiredSize, nullptr, nullptr);
		result.resize(static_cast<std::size_t>(requiredSize - 1));
		return result;
	}

	std::string ResolveDebugName(const char* narrowName, const wchar_t* wideName, std::string_view fallback) noexcept
	{
		if (narrowName != nullptr && narrowName[0] != '\0')
		{
			return narrowName;
		}

		std::string convertedWideName = NarrowString(wideName);
		if (!convertedWideName.empty())
		{
			return convertedWideName;
		}

		return std::string(fallback);
	}

	std::string FormatHRESULT(HRESULT hr)
	{
		return Formatting::FormatPrefixedHexUInt32(static_cast<std::uint32_t>(hr));
	}

	void AppendDiagnosticMessage(
	    std::deque<RhiDiagnosticMessage>& messages,
	    ERhiDiagnosticMessageSeverity severity,
	    ERhiDiagnosticMessageCategory category,
	    std::string text)
	{
		messages.push_back(RhiDiagnosticMessage{.Severity = severity, .Category = category, .Text = std::move(text)});
	}

	ERhiDiagnosticMessageSeverity ToDiagnosticSeverity(D3D12_MESSAGE_SEVERITY severity) noexcept
	{
		switch (severity)
		{
			case D3D12_MESSAGE_SEVERITY_CORRUPTION:
				return ERhiDiagnosticMessageSeverity::Fatal;
			case D3D12_MESSAGE_SEVERITY_ERROR:
				return ERhiDiagnosticMessageSeverity::Error;
			case D3D12_MESSAGE_SEVERITY_WARNING:
				return ERhiDiagnosticMessageSeverity::Warning;
			case D3D12_MESSAGE_SEVERITY_INFO:
				return ERhiDiagnosticMessageSeverity::Info;
			case D3D12_MESSAGE_SEVERITY_MESSAGE:
			default:
				return ERhiDiagnosticMessageSeverity::Verbose;
		}
	}

	ERhiDiagnosticMessageCategory ToDiagnosticCategory(D3D12_MESSAGE_CATEGORY category) noexcept
	{
		switch (category)
		{
			case D3D12_MESSAGE_CATEGORY_COMPILATION:
			case D3D12_MESSAGE_CATEGORY_SHADER:
				return ERhiDiagnosticMessageCategory::Shader;
			case D3D12_MESSAGE_CATEGORY_RESOURCE_MANIPULATION:
				return ERhiDiagnosticMessageCategory::ResourceLifetime;
			case D3D12_MESSAGE_CATEGORY_STATE_CREATION:
			case D3D12_MESSAGE_CATEGORY_STATE_SETTING:
			case D3D12_MESSAGE_CATEGORY_STATE_GETTING:
			case D3D12_MESSAGE_CATEGORY_EXECUTION:
				return ERhiDiagnosticMessageCategory::Validation;
			case D3D12_MESSAGE_CATEGORY_APPLICATION_DEFINED:
			case D3D12_MESSAGE_CATEGORY_MISCELLANEOUS:
			case D3D12_MESSAGE_CATEGORY_INITIALIZATION:
			case D3D12_MESSAGE_CATEGORY_CLEANUP:
			default:
				return ERhiDiagnosticMessageCategory::General;
		}
	}

	std::string_view GetBreadcrumbOperationName(D3D12_AUTO_BREADCRUMB_OP op) noexcept
	{
		switch (op)
		{
			case D3D12_AUTO_BREADCRUMB_OP_SETMARKER:
				return "SetMarker";
			case D3D12_AUTO_BREADCRUMB_OP_BEGINEVENT:
				return "BeginEvent";
			case D3D12_AUTO_BREADCRUMB_OP_ENDEVENT:
				return "EndEvent";
			case D3D12_AUTO_BREADCRUMB_OP_DRAWINSTANCED:
				return "DrawInstanced";
			case D3D12_AUTO_BREADCRUMB_OP_DRAWINDEXEDINSTANCED:
				return "DrawIndexedInstanced";
			case D3D12_AUTO_BREADCRUMB_OP_DISPATCH:
				return "Dispatch";
			case D3D12_AUTO_BREADCRUMB_OP_COPYBUFFERREGION:
				return "CopyBufferRegion";
			case D3D12_AUTO_BREADCRUMB_OP_COPYTEXTUREREGION:
				return "CopyTextureRegion";
			case D3D12_AUTO_BREADCRUMB_OP_COPYRESOURCE:
				return "CopyResource";
			case D3D12_AUTO_BREADCRUMB_OP_CLEARRENDERTARGETVIEW:
				return "ClearRenderTargetView";
			case D3D12_AUTO_BREADCRUMB_OP_CLEARDEPTHSTENCILVIEW:
				return "ClearDepthStencilView";
			case D3D12_AUTO_BREADCRUMB_OP_RESOURCEBARRIER:
				return "ResourceBarrier";
			case D3D12_AUTO_BREADCRUMB_OP_BARRIER:
				return "Barrier";
			case D3D12_AUTO_BREADCRUMB_OP_RESOLVEQUERYDATA:
				return "ResolveQueryData";
			case D3D12_AUTO_BREADCRUMB_OP_BEGINSUBMISSION:
				return "BeginSubmission";
			case D3D12_AUTO_BREADCRUMB_OP_ENDSUBMISSION:
				return "EndSubmission";
			default:
				return "Other";
		}
	}

	std::string_view GetAllocationTypeName(D3D12_DRED_ALLOCATION_TYPE allocationType) noexcept
	{
		switch (allocationType)
		{
			case D3D12_DRED_ALLOCATION_TYPE_COMMAND_QUEUE:
				return "CommandQueue";
			case D3D12_DRED_ALLOCATION_TYPE_COMMAND_ALLOCATOR:
				return "CommandAllocator";
			case D3D12_DRED_ALLOCATION_TYPE_PIPELINE_STATE:
				return "PipelineState";
			case D3D12_DRED_ALLOCATION_TYPE_COMMAND_LIST:
				return "CommandList";
			case D3D12_DRED_ALLOCATION_TYPE_FENCE:
				return "Fence";
			case D3D12_DRED_ALLOCATION_TYPE_DESCRIPTOR_HEAP:
				return "DescriptorHeap";
			case D3D12_DRED_ALLOCATION_TYPE_HEAP:
				return "Heap";
			case D3D12_DRED_ALLOCATION_TYPE_RESOURCE:
				return "Resource";
			case D3D12_DRED_ALLOCATION_TYPE_QUERY_HEAP:
				return "QueryHeap";
			default:
				return "Other";
		}
	}

	std::string ResolveBreadcrumbContext(const D3D12_AUTO_BREADCRUMB_NODE1& node, UINT completedOperations) noexcept
	{
		std::string activeContext;
		for (UINT contextIndex = 0; contextIndex < node.BreadcrumbContextsCount; ++contextIndex)
		{
			const D3D12_DRED_BREADCRUMB_CONTEXT& context = node.pBreadcrumbContexts[contextIndex];
			if (context.BreadcrumbIndex > completedOperations)
			{
				break;
			}

			std::string convertedContext = NarrowString(context.pContextString);
			if (!convertedContext.empty())
			{
				activeContext = std::move(convertedContext);
			}
		}

		return activeContext;
	}

	void AppendBreadcrumbMessages(std::deque<RhiDiagnosticMessage>& messages, const D3D12_AUTO_BREADCRUMB_NODE1* node) noexcept
	{
		std::size_t reportedNodes = 0;
		for (const D3D12_AUTO_BREADCRUMB_NODE1* currentNode = node; currentNode != nullptr && reportedNodes < kMaxBreadcrumbNodes;
		     currentNode = currentNode->pNext, ++reportedNodes)
		{
			const UINT completedOperations = currentNode->pLastBreadcrumbValue != nullptr ? *currentNode->pLastBreadcrumbValue : 0u;
			const UINT historyIndex = completedOperations > 0 && currentNode->BreadcrumbCount > 0
			                              ? (std::min) (completedOperations, currentNode->BreadcrumbCount) - 1
			                              : 0u;
			const D3D12_AUTO_BREADCRUMB_OP lastOperation = currentNode->pCommandHistory != nullptr && currentNode->BreadcrumbCount > 0
			                                                   ? currentNode->pCommandHistory[historyIndex]
			                                                   : D3D12_AUTO_BREADCRUMB_OP_BEGINSUBMISSION;
			const std::string commandListName =
			    ResolveDebugName(currentNode->pCommandListDebugNameA, currentNode->pCommandListDebugNameW, "UnnamedCommandList");
			const std::string commandQueueName =
			    ResolveDebugName(currentNode->pCommandQueueDebugNameA, currentNode->pCommandQueueDebugNameW, "UnnamedCommandQueue");
			const std::string breadcrumbContext = ResolveBreadcrumbContext(*currentNode, completedOperations);

			std::string message = std::format(
			    "DRED breadcrumb queue='{}' commandList='{}' completedOps={}/{} lastOp='{}'",
			    commandQueueName,
			    commandListName,
			    completedOperations,
			    currentNode->BreadcrumbCount,
			    GetBreadcrumbOperationName(lastOperation));
			if (!breadcrumbContext.empty())
			{
				message += std::format(" context='{}'", breadcrumbContext);
			}

			AppendDiagnosticMessage(
			    messages,
			    ERhiDiagnosticMessageSeverity::Error,
			    ERhiDiagnosticMessageCategory::Driver,
			    std::move(message));
		}

		if (node != nullptr)
		{
			std::size_t totalNodes = 0;
			for (const D3D12_AUTO_BREADCRUMB_NODE1* currentNode = node; currentNode != nullptr; currentNode = currentNode->pNext)
			{
				++totalNodes;
			}

			if (totalNodes > kMaxBreadcrumbNodes)
			{
				AppendDiagnosticMessage(
				    messages,
				    ERhiDiagnosticMessageSeverity::Warning,
				    ERhiDiagnosticMessageCategory::Driver,
				    std::format("DRED omitted {} additional breadcrumb nodes", totalNodes - kMaxBreadcrumbNodes));
			}
		}
	}

	template <typename TAllocationNode>
	void AppendAllocationMessages(
	    std::deque<RhiDiagnosticMessage>& messages,
	    const TAllocationNode* node,
	    std::string_view allocationListName) noexcept
	{
		std::size_t reportedAllocations = 0;
		for (const TAllocationNode* currentNode = node; currentNode != nullptr && reportedAllocations < kMaxAllocationNodes;
		     currentNode = currentNode->pNext, ++reportedAllocations)
		{
			AppendDiagnosticMessage(
			    messages,
			    ERhiDiagnosticMessageSeverity::Error,
			    ERhiDiagnosticMessageCategory::Driver,
			    std::format(
			        "DRED {} allocation type='{}' name='{}'",
			        allocationListName,
			        GetAllocationTypeName(currentNode->AllocationType),
			        ResolveDebugName(currentNode->ObjectNameA, currentNode->ObjectNameW, "UnnamedObject")));
		}

		std::size_t totalAllocations = 0;
		for (const TAllocationNode* currentNode = node; currentNode != nullptr; currentNode = currentNode->pNext)
		{
			++totalAllocations;
		}

		if (totalAllocations > kMaxAllocationNodes)
		{
			AppendDiagnosticMessage(
			    messages,
			    ERhiDiagnosticMessageSeverity::Warning,
			    ERhiDiagnosticMessageCategory::Driver,
			    std::format("DRED omitted {} additional {} allocations", totalAllocations - kMaxAllocationNodes, allocationListName));
		}
	}
}

using namespace D3D12DebugLayerDiagnostics;

D3D12DebugLayer::D3D12DebugLayer()
{
	InitDredSettings();
	InitD3D12Debug();
	InitDXGIDebug();
	SPDLOG_LOGGER_INFO(g_d3d12DiagnosticsLogger, "D3D12 diagnostics bootstrap completed.");
}

D3D12DebugLayer::~D3D12DebugLayer() noexcept
{
	m_messages.clear();
	m_infoQueue.Reset();
	m_dxgiDebug.Reset();
	m_d3d12Debug.Reset();
}

void D3D12DebugLayer::InitializeInfoQueue(ID3D12Device* device)
{
	if (!device)
	{
		m_infoQueue.Reset();
		m_messages.clear();
		SPDLOG_LOGGER_WARN(
		    g_d3d12DiagnosticsLogger,
		    "D3D12 debug messages unavailable: no device was available when attaching the info queue.");
		return;
	}

	const HRESULT infoQueueHr = device->QueryInterface(IID_PPV_ARGS(m_infoQueue.ReleaseAndGetAddressOf()));
	if (FAILED(infoQueueHr))
	{
		m_infoQueue.Reset();
		m_messages.clear();
		SPDLOG_LOGGER_WARN(
		    g_d3d12DiagnosticsLogger,
		    "D3D12 debug messages unavailable: ID3D12InfoQueue query failed with HRESULT {}.",
		    FormatHRESULT(infoQueueHr));
		return;
	}

	ConfigureInfoQueue(device);
	ApplyInfoQueueFilters(device);
	ClearMessages();
	SPDLOG_LOGGER_INFO(g_d3d12DiagnosticsLogger, "D3D12 debug messages enabled through ID3D12InfoQueue.");
}

bool D3D12DebugLayer::SupportsDebugMessages() const noexcept
{
	return m_infoQueue != nullptr;
}

bool D3D12DebugLayer::TryPopMessage(RhiDiagnosticMessage& outMessage) noexcept
{
	if (!SupportsDebugMessages())
	{
		return false;
	}

	if (m_messages.empty())
	{
		DrainStoredMessages();
	}

	if (m_messages.empty())
	{
		return false;
	}

	outMessage = std::move(m_messages.front());
	m_messages.pop_front();
	return true;
}

void D3D12DebugLayer::ClearMessages() noexcept
{
	m_messages.clear();
	if (m_infoQueue != nullptr)
	{
		m_infoQueue->ClearStoredMessages();
	}
}

bool D3D12DebugLayer::SupportsLiveObjectReports() const noexcept
{
	#if ENGINE_REPORT_LIVE_OBJECTS
	return m_dxgiDebug != nullptr;
	#else
	return false;
	#endif
}

bool D3D12DebugLayer::SupportsCrashDiagnostics() const noexcept
{
	return m_supportsCrashDiagnostics;
}

void D3D12DebugLayer::ReportLiveObjects(ID3D12Device* device)
{
	ReportLiveDeviceObjects(device);
	ReportLiveDXGIObjects();
}

void D3D12DebugLayer::CollectCrashDiagnostics(ID3D12Device* device) noexcept
{
	if (device == nullptr || !m_supportsCrashDiagnostics)
	{
		return;
	}

	const HRESULT deviceRemovedReason = device->GetDeviceRemovedReason();
	if (SUCCEEDED(deviceRemovedReason))
	{
		return;
	}

	DrainStoredMessages();
	AppendDiagnosticMessage(
	    m_messages,
	    ERhiDiagnosticMessageSeverity::Fatal,
	    ERhiDiagnosticMessageCategory::Driver,
	    std::format("D3D12 device removal detected with HRESULT {}", FormatHRESULT(deviceRemovedReason)));

	ComPtr<ID3D12DeviceRemovedExtendedData1> dred1;
	if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(dred1.ReleaseAndGetAddressOf()))))
	{
		D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT1 autoBreadcrumbs{};
		if (SUCCEEDED(dred1->GetAutoBreadcrumbsOutput1(&autoBreadcrumbs)))
		{
			AppendBreadcrumbMessages(m_messages, autoBreadcrumbs.pHeadAutoBreadcrumbNode);
		}

		D3D12_DRED_PAGE_FAULT_OUTPUT1 pageFaultOutput{};
		if (SUCCEEDED(dred1->GetPageFaultAllocationOutput1(&pageFaultOutput)) && pageFaultOutput.PageFaultVA != 0)
		{
			AppendDiagnosticMessage(
			    m_messages,
			    ERhiDiagnosticMessageSeverity::Fatal,
			    ERhiDiagnosticMessageCategory::Driver,
			    std::format("DRED page fault at GPU VA {}", Formatting::FormatPrefixedHexUInt64(pageFaultOutput.PageFaultVA)));
			AppendAllocationMessages(m_messages, pageFaultOutput.pHeadExistingAllocationNode, "existing");
			AppendAllocationMessages(m_messages, pageFaultOutput.pHeadRecentFreedAllocationNode, "recent-freed");
		}
		return;
	}

	ComPtr<ID3D12DeviceRemovedExtendedData> dred;
	if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(dred.ReleaseAndGetAddressOf()))))
	{
		D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT autoBreadcrumbs{};
		if (SUCCEEDED(dred->GetAutoBreadcrumbsOutput(&autoBreadcrumbs)) && autoBreadcrumbs.pHeadAutoBreadcrumbNode != nullptr)
		{
			const D3D12_AUTO_BREADCRUMB_NODE* currentNode = autoBreadcrumbs.pHeadAutoBreadcrumbNode;
			std::size_t reportedNodes = 0;
			for (; currentNode != nullptr && reportedNodes < kMaxBreadcrumbNodes; currentNode = currentNode->pNext, ++reportedNodes)
			{
				const UINT completedOperations = currentNode->pLastBreadcrumbValue != nullptr ? *currentNode->pLastBreadcrumbValue : 0u;
				const UINT historyIndex = completedOperations > 0 && currentNode->BreadcrumbCount > 0
				                              ? (std::min) (completedOperations, currentNode->BreadcrumbCount) - 1
				                              : 0u;
				const D3D12_AUTO_BREADCRUMB_OP lastOperation = currentNode->pCommandHistory != nullptr && currentNode->BreadcrumbCount > 0
				                                                   ? currentNode->pCommandHistory[historyIndex]
				                                                   : D3D12_AUTO_BREADCRUMB_OP_BEGINSUBMISSION;
				AppendDiagnosticMessage(
				    m_messages,
				    ERhiDiagnosticMessageSeverity::Error,
				    ERhiDiagnosticMessageCategory::Driver,
				    std::format(
				        "DRED breadcrumb queue='{}' commandList='{}' completedOps={}/{} lastOp='{}'",
				        ResolveDebugName(currentNode->pCommandQueueDebugNameA, currentNode->pCommandQueueDebugNameW, "UnnamedCommandQueue"),
				        ResolveDebugName(currentNode->pCommandListDebugNameA, currentNode->pCommandListDebugNameW, "UnnamedCommandList"),
				        completedOperations,
				        currentNode->BreadcrumbCount,
				        GetBreadcrumbOperationName(lastOperation)));
			}
		}

		D3D12_DRED_PAGE_FAULT_OUTPUT pageFaultOutput{};
		if (SUCCEEDED(dred->GetPageFaultAllocationOutput(&pageFaultOutput)) && pageFaultOutput.PageFaultVA != 0)
		{
			AppendDiagnosticMessage(
			    m_messages,
			    ERhiDiagnosticMessageSeverity::Fatal,
			    ERhiDiagnosticMessageCategory::Driver,
			    std::format("DRED page fault at GPU VA {}", Formatting::FormatPrefixedHexUInt64(pageFaultOutput.PageFaultVA)));
			AppendAllocationMessages(m_messages, pageFaultOutput.pHeadExistingAllocationNode, "existing");
			AppendAllocationMessages(m_messages, pageFaultOutput.pHeadRecentFreedAllocationNode, "recent-freed");
		}
	}
}

void D3D12DebugLayer::InitDredSettings() noexcept
{
	ComPtr<ID3D12DeviceRemovedExtendedDataSettings> dredSettings;
	const HRESULT dredHr = D3D12GetDebugInterface(IID_PPV_ARGS(dredSettings.ReleaseAndGetAddressOf()));
	if (FAILED(dredHr))
	{
		m_supportsCrashDiagnostics = false;
		SPDLOG_LOGGER_WARN(
		    g_d3d12DiagnosticsLogger,
		    "D3D12 crash diagnostics unavailable: DRED settings interface query failed with HRESULT {}.",
		    FormatHRESULT(dredHr));
		return;
	}

	dredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
	dredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
	dredSettings->SetWatsonDumpEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);

	ComPtr<ID3D12DeviceRemovedExtendedDataSettings1> dredSettings1;
	if (SUCCEEDED(dredSettings->QueryInterface(IID_PPV_ARGS(dredSettings1.ReleaseAndGetAddressOf()))))
	{
		dredSettings1->SetBreadcrumbContextEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
	}

	m_supportsCrashDiagnostics = true;
	SPDLOG_LOGGER_INFO(g_d3d12DiagnosticsLogger, "D3D12 crash diagnostics enabled through DRED.");
}

void D3D12DebugLayer::InitD3D12Debug()
{
	CHECK(D3D12GetDebugInterface(IID_PPV_ARGS(m_d3d12Debug.ReleaseAndGetAddressOf())));
	m_d3d12Debug->EnableDebugLayer();
	SPDLOG_LOGGER_INFO(g_d3d12DiagnosticsLogger, "D3D12 debug layer enabled.");
}

void D3D12DebugLayer::InitDXGIDebug()
{
	CHECK(DXGIGetDebugInterface1(0, IID_PPV_ARGS(m_dxgiDebug.ReleaseAndGetAddressOf())));
	m_dxgiDebug->EnableLeakTrackingForThread();
	SPDLOG_LOGGER_INFO(g_d3d12DiagnosticsLogger, "DXGI debug interface enabled for live object reporting.");
}

void D3D12DebugLayer::ConfigureInfoQueue(ID3D12Device* device)
{
	static_cast<void>(device);

	if (m_infoQueue != nullptr)
	{
		const BOOL debuggerAttached = IsDebuggerPresent();
		m_infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, debuggerAttached);
		m_infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, debuggerAttached);
		m_infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, debuggerAttached);

		if (!debuggerAttached)
		{
			SPDLOG_LOGGER_INFO(
			    g_d3d12DiagnosticsLogger,
			    "D3D12 info-queue breakpoints disabled because no debugger is attached; diagnostics will be reported through log "
			    "collection instead.");
		}
	}
}

void D3D12DebugLayer::ApplyInfoQueueFilters(ID3D12Device* device)
{
	if (m_infoQueue != nullptr)
	{
		D3D12_MESSAGE_ID disabledMessages[] = {static_cast<D3D12_MESSAGE_ID>(1424)};
		D3D12_INFO_QUEUE_FILTER filter = {};
		filter.DenyList.NumIDs = static_cast<UINT>(std::size(disabledMessages));
		filter.DenyList.pIDList = disabledMessages;
		m_infoQueue->AddStorageFilterEntries(&filter);
	}
}

void D3D12DebugLayer::DrainStoredMessages() noexcept
{
	if (m_infoQueue == nullptr)
	{
		return;
	}

	const UINT64 numMessages = m_infoQueue->GetNumStoredMessagesAllowedByRetrievalFilter();
	for (UINT64 messageIndex = 0; messageIndex < numMessages; ++messageIndex)
	{
		SIZE_T messageLength = 0;
		if (FAILED(m_infoQueue->GetMessage(messageIndex, nullptr, &messageLength)) || messageLength == 0)
		{
			continue;
		}

		std::vector<char> messageData(messageLength);
		auto* message = reinterpret_cast<D3D12_MESSAGE*>(messageData.data());
		if (SUCCEEDED(m_infoQueue->GetMessage(messageIndex, message, &messageLength)))
		{
			m_messages.push_back(
			    RhiDiagnosticMessage{
			        .Severity = ToDiagnosticSeverity(message->Severity),
			        .Category = ToDiagnosticCategory(message->Category),
			        .Text = message->pDescription != nullptr ? message->pDescription : ""});
		}
	}

	m_infoQueue->ClearStoredMessages();
}

void D3D12DebugLayer::ReportLiveDeviceObjects(ID3D12Device* device)
{
	#if ENGINE_REPORT_LIVE_OBJECTS
	ComPtr<ID3D12DebugDevice> debugDevice;
	if (device && SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(debugDevice.ReleaseAndGetAddressOf()))))
	{
		OutputDebugStringW(L"D3D12 Live Device Objects (detail + summary):\n");
		debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_SUMMARY);
	}
	#endif
}

void D3D12DebugLayer::ReportLiveDXGIObjects()
{
	#if ENGINE_REPORT_LIVE_OBJECTS
	if (m_dxgiDebug)
	{
		OutputDebugStringW(L"DXGI Live Objects (all flags):\n");
		m_dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_ALL));
	}
	#endif
}
#endif
