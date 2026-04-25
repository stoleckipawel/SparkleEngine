#include "PCH.h"

#include "D3D12/D3D12RenderHardwareInterface.h"

#include "D3D12/D3D12Rhi.h"
#include "D3D12/D3D12SwapChain.h"
#include "D3D12/D3D12TypeConversions.h"
#include "D3D12/Descriptors/D3D12DescriptorHeap.h"
#include "D3D12/Descriptors/D3D12DescriptorHeapManager.h"
#include "D3D12/Pipeline/D3D12BindingLayout.h"
#include "D3D12/Pipeline/D3D12PipelineState.h"
#include "D3D12/Resources/D3D12ConstantBufferManager.h"
#include "Resources/Texture.h"
#include "D3D12/Textures/TextureFactory.h"
#include "D3D12/Textures/TextureLoader.h"

#include <d3d12.h>
#include <cstring>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{
	struct D3D12PixEventRuntime final
	{
		using BeginEventOnCommandListFn = void(WINAPI*)(ID3D12GraphicsCommandList* commandList, UINT64 color, PCSTR formatString);
		using EndEventOnCommandListFn = void(WINAPI*)(ID3D12GraphicsCommandList* commandList);
		using SetMarkerOnCommandListFn = void(WINAPI*)(ID3D12GraphicsCommandList* commandList, UINT64 color, PCSTR formatString);

		D3D12PixEventRuntime() noexcept
		{
			module = LoadLibraryW(L"WinPixEventRuntime.dll");
			if (module == nullptr)
			{
				return;
			}

			beginEventOnCommandList = reinterpret_cast<BeginEventOnCommandListFn>(GetProcAddress(module, "PIXBeginEventOnCommandList"));
			endEventOnCommandList = reinterpret_cast<EndEventOnCommandListFn>(GetProcAddress(module, "PIXEndEventOnCommandList"));
			setMarkerOnCommandList = reinterpret_cast<SetMarkerOnCommandListFn>(GetProcAddress(module, "PIXSetMarkerOnCommandList"));
		}

		bool IsAvailable() const noexcept
		{
			return module != nullptr && beginEventOnCommandList != nullptr && endEventOnCommandList != nullptr &&
			       setMarkerOnCommandList != nullptr;
		}

		void BeginEvent(ID3D12GraphicsCommandList* commandList, UINT64 color, const char* label) const noexcept
		{
			if (IsAvailable() && commandList != nullptr && label != nullptr)
			{
				beginEventOnCommandList(commandList, color, label);
			}
		}

		void EndEvent(ID3D12GraphicsCommandList* commandList) const noexcept
		{
			if (IsAvailable() && commandList != nullptr)
			{
				endEventOnCommandList(commandList);
			}
		}

		void SetMarker(ID3D12GraphicsCommandList* commandList, UINT64 color, const char* label) const noexcept
		{
			if (IsAvailable() && commandList != nullptr && label != nullptr)
			{
				setMarkerOnCommandList(commandList, color, label);
			}
		}

		HMODULE module = nullptr;
		BeginEventOnCommandListFn beginEventOnCommandList = nullptr;
		EndEventOnCommandListFn endEventOnCommandList = nullptr;
		SetMarkerOnCommandListFn setMarkerOnCommandList = nullptr;
	};

	const D3D12PixEventRuntime& GetPixEventRuntime() noexcept
	{
		static const D3D12PixEventRuntime runtime;
		return runtime;
	}

	UINT64 ToPixEventColor(RhiDiagnosticLabelColor color) noexcept
	{
		return (static_cast<UINT64>(0xFFu) << 24u) | (static_cast<UINT64>(color.Red) << 16u) |
		       (static_cast<UINT64>(color.Green) << 8u) | static_cast<UINT64>(color.Blue);
	}

	struct OwnedHeapState
	{
		Microsoft::WRL::ComPtr<ID3D12Heap> Heap;
	};

	struct OwnedResourceState
	{
		Microsoft::WRL::ComPtr<ID3D12Resource> Resource;
	};

	ID3D12GraphicsCommandList* ToD3D12GraphicsCommandList(NativeGraphicsCommandListHandle handle) noexcept
	{
		return static_cast<ID3D12GraphicsCommandList*>(handle.Value);
	}

	ID3D12Resource* ToD3D12Resource(NativeResourceHandle handle) noexcept
	{
		return static_cast<ID3D12Resource*>(handle.Value);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE ToD3D12CpuDescriptor(RhiCpuDescriptorHandle handle) noexcept
	{
		return D3D12_CPU_DESCRIPTOR_HANDLE{handle.Value};
	}

	D3D12_GPU_DESCRIPTOR_HANDLE ToD3D12GpuDescriptor(RhiGpuDescriptorHandle handle) noexcept
	{
		return D3D12_GPU_DESCRIPTOR_HANDLE{handle.Value};
	}

	D3D12_DESCRIPTOR_HEAP_TYPE ToD3D12DescriptorHeapType(ERhiDescriptorHeapType heapType) noexcept
	{
		switch (heapType)
		{
			case ERhiDescriptorHeapType::RenderTarget:
				return D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			case ERhiDescriptorHeapType::DepthStencil:
				return D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
			case ERhiDescriptorHeapType::Sampler:
				return D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
			case ERhiDescriptorHeapType::ShaderResource:
			default:
				return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		}
	}

	D3D12_RESOURCE_STATES ToD3D12ResourceState(ResourceState state) noexcept
	{
		switch (state)
		{
			case ResourceState::Common:
				return D3D12_RESOURCE_STATE_COMMON;
			case ResourceState::RenderTarget:
				return D3D12_RESOURCE_STATE_RENDER_TARGET;
			case ResourceState::DepthWrite:
				return D3D12_RESOURCE_STATE_DEPTH_WRITE;
			case ResourceState::DepthRead:
				return D3D12_RESOURCE_STATE_DEPTH_READ;
			case ResourceState::ShaderResource:
				return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
			case ResourceState::UnorderedAccess:
				return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			case ResourceState::CopySource:
				return D3D12_RESOURCE_STATE_COPY_SOURCE;
			case ResourceState::CopyDest:
				return D3D12_RESOURCE_STATE_COPY_DEST;
			case ResourceState::Present:
				return D3D12_RESOURCE_STATE_PRESENT;
			default:
				return D3D12_RESOURCE_STATE_COMMON;
		}
	}

	D3D12_PRIMITIVE_TOPOLOGY ToD3D12PrimitiveTopology(RhiPrimitiveTopology topology) noexcept
	{
		switch (topology)
		{
			case RhiPrimitiveTopology::TriangleList:
			default:
				return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		}
	}

	DXGI_FORMAT ToD3D12IndexFormat(RhiIndexFormat format) noexcept
	{
		switch (format)
		{
			case RhiIndexFormat::UInt16:
				return DXGI_FORMAT_R16_UINT;
			case RhiIndexFormat::UInt32:
			default:
				return DXGI_FORMAT_R32_UINT;
		}
	}

	D3D12_RESOURCE_DESC BuildTextureResourceDesc(const RhiTextureResourceDesc& desc) noexcept
	{
		D3D12_RESOURCE_DESC resourceDesc{};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		resourceDesc.Width = static_cast<UINT64>(desc.Width);
		resourceDesc.Height = desc.Height;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = desc.MipLevels;
		resourceDesc.Format = D3D12TypeConversions::ToDxgiFormat(desc.Format);
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.SampleDesc.Quality = 0;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		if (desc.AllowRenderTarget)
		{
			resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		}
		if (desc.AllowDepthStencil)
		{
			resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		}
		if (desc.AllowUnorderedAccess)
		{
			resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}
		return resourceDesc;
	}

	D3D12_RESOURCE_DESC BuildBufferResourceDesc(const RhiBufferResourceDesc& desc) noexcept
	{
		D3D12_RESOURCE_DESC resourceDesc{};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		resourceDesc.Width = desc.SizeInBytes;
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.SampleDesc.Quality = 0;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resourceDesc.Flags = desc.AllowUnorderedAccess ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;
		return resourceDesc;
	}

	D3D12_HEAP_FLAGS ToHeapFlags(RhiTransientAllocationPool pool) noexcept
	{
		switch (pool)
		{
			case RhiTransientAllocationPool::Buffer:
				return D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
			case RhiTransientAllocationPool::Color:
			case RhiTransientAllocationPool::Depth:
			default:
				return D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES;
		}
	}

	D3D12_CLEAR_VALUE BuildClearValue(const RhiOptimizedClearValue& clearValue) noexcept
	{
		D3D12_CLEAR_VALUE nativeClearValue{};
		nativeClearValue.Format = D3D12TypeConversions::ToDxgiFormat(clearValue.Format);
		if (clearValue.ValueType == RhiOptimizedClearValue::Type::DepthStencil)
		{
			nativeClearValue.DepthStencil.Depth = clearValue.Depth;
			nativeClearValue.DepthStencil.Stencil = clearValue.Stencil;
		}
		else
		{
			for (std::size_t index = 0; index < clearValue.Color.size(); ++index)
			{
				nativeClearValue.Color[index] = clearValue.Color[index];
			}
		}
		return nativeClearValue;
	}

	D3D12_HEAP_PROPERTIES BuildUploadHeapProperties() noexcept
	{
		D3D12_HEAP_PROPERTIES properties{};
		properties.Type = D3D12_HEAP_TYPE_UPLOAD;
		properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		properties.CreationNodeMask = 1;
		properties.VisibleNodeMask = 1;
		return properties;
	}

	D3D12_HEAP_PROPERTIES BuildReadbackHeapProperties() noexcept
	{
		D3D12_HEAP_PROPERTIES properties{};
		properties.Type = D3D12_HEAP_TYPE_READBACK;
		properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		properties.CreationNodeMask = 1;
		properties.VisibleNodeMask = 1;
		return properties;
	}

	std::wstring CopyDebugName(std::wstring_view debugName, std::wstring_view fallbackName) noexcept
	{
		return debugName.empty() ? std::wstring(fallbackName) : std::wstring(debugName);
	}

	OwnedHeapState* ToOwnedHeapState(RhiOwnedHeapHandle handle) noexcept
	{
		return static_cast<OwnedHeapState*>(handle.Value);
	}

	const OwnedResourceState* ToConstOwnedResourceState(RhiOwnedResourceHandle handle) noexcept
	{
		return static_cast<const OwnedResourceState*>(handle.Value);
	}

	OwnedResourceState* ToOwnedResourceState(RhiOwnedResourceHandle handle) noexcept
	{
		return static_cast<OwnedResourceState*>(handle.Value);
	}

	bool ResourceSupportsUnorderedAccess(ID3D12Resource* resource) noexcept
	{
		return resource != nullptr && (resource->GetDesc().Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0;
	}

	void SetD3D12ObjectDebugName(ID3D12Object* object, std::wstring_view debugName) noexcept
	{
		if (object == nullptr || debugName.empty())
		{
			return;
		}

		std::wstring ownedName(debugName);
		object->SetName(ownedName.c_str());
	}
	class D3D12RenderObjectDiagnostics final : public RenderObjectDiagnostics
	{
	  public:
		explicit D3D12RenderObjectDiagnostics(D3D12Rhi&) noexcept {}

		bool SupportsObjectNames() const noexcept override { return true; }

		void SetDebugName(NativeGraphicsDeviceHandle device, std::wstring_view debugName) noexcept override
		{
			SetD3D12ObjectDebugName(static_cast<ID3D12Object*>(device.Value), debugName);
		}

		void SetDebugName(NativeGraphicsQueueHandle queue, std::wstring_view debugName) noexcept override
		{
			SetD3D12ObjectDebugName(static_cast<ID3D12Object*>(queue.Value), debugName);
		}

		void SetDebugName(NativeGraphicsCommandListHandle commandList, std::wstring_view debugName) noexcept override
		{
			SetD3D12ObjectDebugName(static_cast<ID3D12Object*>(commandList.Value), debugName);
		}

		void SetDebugName(NativeResourceHandle resource, std::wstring_view debugName) noexcept override
		{
			SetD3D12ObjectDebugName(static_cast<ID3D12Object*>(resource.Value), debugName);
		}

		void SetDebugName(RhiOwnedHeapHandle heap, std::wstring_view debugName) noexcept override
		{
			OwnedHeapState* ownedHeap = ToOwnedHeapState(heap);
			SetD3D12ObjectDebugName(ownedHeap != nullptr ? ownedHeap->Heap.Get() : nullptr, debugName);
		}

		void SetDebugName(RhiOwnedResourceHandle resource, std::wstring_view debugName) noexcept override
		{
			OwnedResourceState* ownedResource = ToOwnedResourceState(resource);
			SetD3D12ObjectDebugName(ownedResource != nullptr ? ownedResource->Resource.Get() : nullptr, debugName);
		}

	};

	class D3D12RenderTimingDiagnostics final : public RenderTimingDiagnostics
	{
	  public:
		explicit D3D12RenderTimingDiagnostics(D3D12Rhi& rhi) noexcept : m_rhi(&rhi)
		{
			Initialize();
		}

		bool SupportsTimestampQueries() const noexcept override { return m_supportsTimestampQueries; }

		RhiTimestampQueryHandle AllocateTimestampQuery() override
		{
			if (!m_supportsTimestampQueries || m_rhi == nullptr || m_queryLocations.size() >= std::numeric_limits<std::uint32_t>::max() - 1)
			{
				return {};
			}

			const std::uint32_t frameIndex = m_rhi->GetCurrentFrameIndex();
			if (frameIndex >= m_frameStates.size())
			{
				return {};
			}

			FrameTimingState& frameState = m_frameStates[frameIndex];
			if (frameState.FreeQueryIndices.empty() || frameState.MappedReadback == nullptr)
			{
				return {};
			}

			const std::uint32_t queryIndex = frameState.FreeQueryIndices.back();
			frameState.FreeQueryIndices.pop_back();
			frameState.MappedReadback[queryIndex] = 0;

			std::uint32_t handleValue = m_nextHandleValue++;
			while (handleValue == 0 || m_queryLocations.find(handleValue) != m_queryLocations.end())
			{
				handleValue = m_nextHandleValue++;
			}

			m_queryLocations.emplace(handleValue, QueryLocation{.FrameIndex = frameIndex, .QueryIndex = queryIndex});
			return RhiTimestampQueryHandle{.Value = handleValue};
		}

		void ReleaseTimestampQuery(RhiTimestampQueryHandle query) noexcept override
		{
			const auto locationIt = m_queryLocations.find(query.Value);
			if (locationIt == m_queryLocations.end())
			{
				return;
			}

			const QueryLocation location = locationIt->second;
			if (location.FrameIndex < m_frameStates.size())
			{
				FrameTimingState& frameState = m_frameStates[location.FrameIndex];
				if (location.QueryIndex < frameState.QueryCount)
				{
					if (frameState.MappedReadback != nullptr)
					{
						frameState.MappedReadback[location.QueryIndex] = 0;
					}
					frameState.FreeQueryIndices.push_back(location.QueryIndex);
				}
			}

			m_queryLocations.erase(locationIt);
		}

		bool WriteTimestamp(RenderCommandList& commandList, RhiTimestampQueryHandle query) noexcept override
		{
			if (!m_supportsTimestampQueries)
			{
				return false;
			}

			const auto locationIt = m_queryLocations.find(query.Value);
			if (locationIt == m_queryLocations.end())
			{
				return false;
			}

			const QueryLocation location = locationIt->second;
			if (location.FrameIndex >= m_frameStates.size())
			{
				return false;
			}

			FrameTimingState& frameState = m_frameStates[location.FrameIndex];
			ID3D12GraphicsCommandList* const nativeCommandList = ToD3D12GraphicsCommandList(commandList.GetNativeHandle());
			if (nativeCommandList == nullptr || frameState.QueryHeap == nullptr || frameState.ReadbackBuffer == nullptr)
			{
				return false;
			}

			nativeCommandList->EndQuery(frameState.QueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, location.QueryIndex);
			nativeCommandList->ResolveQueryData(
			    frameState.QueryHeap.Get(),
			    D3D12_QUERY_TYPE_TIMESTAMP,
			    location.QueryIndex,
			    1,
			    frameState.ReadbackBuffer.Get(),
			    static_cast<UINT64>(location.QueryIndex) * sizeof(std::uint64_t));
			return true;
		}

		bool TryResolveTimestamp(RhiTimestampQueryHandle query, std::uint64_t& outTicks) const noexcept override
		{
			outTicks = 0;
			const auto locationIt = m_queryLocations.find(query.Value);
			if (!m_supportsTimestampQueries || locationIt == m_queryLocations.end())
			{
				return false;
			}

			const QueryLocation location = locationIt->second;
			if (location.FrameIndex >= m_frameStates.size())
			{
				return false;
			}

			const FrameTimingState& frameState = m_frameStates[location.FrameIndex];
			if (frameState.MappedReadback == nullptr || location.QueryIndex >= frameState.QueryCount)
			{
				return false;
			}

			outTicks = frameState.MappedReadback[location.QueryIndex];
			return true;
		}

		std::uint64_t GetTimestampFrequencyHz() const noexcept override { return m_timestampFrequencyHz; }

	  private:
		static constexpr std::uint32_t kQueriesPerFrame = 4096;

		struct QueryLocation
		{
			std::uint32_t FrameIndex = 0;
			std::uint32_t QueryIndex = 0;
		};

		struct FrameTimingState
		{
			Microsoft::WRL::ComPtr<ID3D12QueryHeap> QueryHeap;
			Microsoft::WRL::ComPtr<ID3D12Resource> ReadbackBuffer;
			std::uint64_t* MappedReadback = nullptr;
			std::vector<std::uint32_t> FreeQueryIndices;
			std::uint32_t QueryCount = 0;
		};

		void Initialize() noexcept
		{
			if (m_rhi == nullptr || m_rhi->GetDevice() == nullptr || m_rhi->GetCommandQueue() == nullptr)
			{
				return;
			}

			UINT64 timestampFrequency = 0;
			if (FAILED(m_rhi->GetCommandQueue()->GetTimestampFrequency(&timestampFrequency)) || timestampFrequency == 0)
			{
				return;
			}

			for (std::uint32_t frameIndex = 0; frameIndex < m_frameStates.size(); ++frameIndex)
			{
				if (!InitializeFrameState(frameIndex))
				{
					return;
				}
			}

			m_timestampFrequencyHz = timestampFrequency;
			m_supportsTimestampQueries = true;
		}

		bool InitializeFrameState(std::uint32_t frameIndex) noexcept
		{
			FrameTimingState& frameState = m_frameStates[frameIndex];
			const D3D12_QUERY_HEAP_DESC queryHeapDesc{
			    .Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP,
			    .Count = kQueriesPerFrame,
			    .NodeMask = 0};
			if (FAILED(m_rhi->GetDevice()->CreateQueryHeap(
			        &queryHeapDesc,
			        IID_PPV_ARGS(frameState.QueryHeap.ReleaseAndGetAddressOf()))))
			{
				return false;
			}

			const RhiBufferResourceDesc readbackBufferDesc{
			    .SizeInBytes = static_cast<std::uint64_t>(kQueriesPerFrame) * sizeof(std::uint64_t),
			    .StrideInBytes = sizeof(std::uint64_t),
			    .AllowUnorderedAccess = false};
			const D3D12_RESOURCE_DESC nativeReadbackDesc = BuildBufferResourceDesc(readbackBufferDesc);
			const D3D12_HEAP_PROPERTIES readbackHeapProperties = BuildReadbackHeapProperties();
			if (FAILED(m_rhi->GetDevice()->CreateCommittedResource(
			        &readbackHeapProperties,
			        D3D12_HEAP_FLAG_NONE,
			        &nativeReadbackDesc,
			        D3D12_RESOURCE_STATE_COPY_DEST,
			        nullptr,
			        IID_PPV_ARGS(frameState.ReadbackBuffer.ReleaseAndGetAddressOf()))))
			{
				return false;
			}

			if (FAILED(frameState.ReadbackBuffer->Map(0, nullptr, reinterpret_cast<void**>(&frameState.MappedReadback))))
			{
				return false;
			}

			std::memset(frameState.MappedReadback, 0, static_cast<std::size_t>(readbackBufferDesc.SizeInBytes));
			frameState.QueryCount = kQueriesPerFrame;
			frameState.FreeQueryIndices.reserve(kQueriesPerFrame);
			for (std::uint32_t queryIndex = kQueriesPerFrame; queryIndex > 0; --queryIndex)
			{
				frameState.FreeQueryIndices.push_back(queryIndex - 1);
			}

			std::wstring queryHeapName = std::wstring(L"D3D12TimestampQueryHeap_Frame") + std::to_wstring(frameIndex);
			frameState.QueryHeap->SetName(queryHeapName.c_str());
			std::wstring readbackName = std::wstring(L"D3D12TimestampReadback_Frame") + std::to_wstring(frameIndex);
			frameState.ReadbackBuffer->SetName(readbackName.c_str());
			return true;
		}

		D3D12Rhi* m_rhi = nullptr;
		std::array<FrameTimingState, RenderConfig::FramesInFlight> m_frameStates;
		std::unordered_map<std::uint32_t, QueryLocation> m_queryLocations;
		std::uint32_t m_nextHandleValue = 1;
		std::uint64_t m_timestampFrequencyHz = 0;
		bool m_supportsTimestampQueries = false;
	};

	class D3D12RenderMessageDiagnostics final : public RenderMessageDiagnostics
	{
	  public:
		explicit D3D12RenderMessageDiagnostics(D3D12Rhi& rhi) noexcept : m_rhi(rhi) {}

		bool SupportsDebugMessages() const noexcept override { return m_rhi.SupportsDebugMessages(); }

		bool TryPopMessage(RhiDiagnosticMessage& outMessage) noexcept override { return m_rhi.TryPopDebugMessage(outMessage); }

		void ClearMessages() noexcept override { m_rhi.ClearDebugMessages(); }

	  private:
		D3D12Rhi& m_rhi;
	};

	class D3D12RenderFailureDiagnostics final : public RenderFailureDiagnostics
	{
	  public:
		explicit D3D12RenderFailureDiagnostics(D3D12Rhi& rhi) noexcept : m_rhi(rhi) {}

		bool SupportsLiveObjectReports() const noexcept override { return m_rhi.SupportsLiveObjectReports(); }

		bool SupportsCrashDiagnostics() const noexcept override { return m_rhi.SupportsCrashDiagnostics(); }

		void ReportLiveObjects() noexcept override { m_rhi.ReportLiveObjects(); }

		void CollectCrashDiagnostics() noexcept override { m_rhi.CollectCrashDiagnostics(); }

	  private:
		D3D12Rhi& m_rhi;
	};

	class D3D12RenderDiagnostics final : public RenderDiagnostics
	{
	  public:
		explicit D3D12RenderDiagnostics(D3D12Rhi& rhi) noexcept :
			m_objectDiagnostics(rhi), m_timingDiagnostics(rhi), m_messageDiagnostics(rhi), m_failureDiagnostics(rhi)
		{
		}

		RhiDiagnosticsCapabilities GetCapabilities() const noexcept override
		{
			return RhiDiagnosticsCapabilities{
			    .SupportsObjectNames = m_objectDiagnostics.SupportsObjectNames(),
			    .SupportsGpuEvents = GetPixEventRuntime().IsAvailable(),
			    .SupportsTimestampQueries = m_timingDiagnostics.SupportsTimestampQueries(),
			    .SupportsDebugMessages = m_messageDiagnostics.SupportsDebugMessages(),
			    .SupportsLiveObjectReports = m_failureDiagnostics.SupportsLiveObjectReports(),
			    .SupportsCrashDiagnostics = m_failureDiagnostics.SupportsCrashDiagnostics()};
		}

		RenderObjectDiagnostics& GetObjectDiagnostics() noexcept override { return m_objectDiagnostics; }

		const RenderObjectDiagnostics& GetObjectDiagnostics() const noexcept override { return m_objectDiagnostics; }

		RenderTimingDiagnostics* GetTimingDiagnostics() noexcept override
		{
			return m_timingDiagnostics.SupportsTimestampQueries() ? &m_timingDiagnostics : nullptr;
		}

		const RenderTimingDiagnostics* GetTimingDiagnostics() const noexcept override
		{
			return m_timingDiagnostics.SupportsTimestampQueries() ? &m_timingDiagnostics : nullptr;
		}

		RenderMessageDiagnostics* GetMessageDiagnostics() noexcept override
		{
			return m_messageDiagnostics.SupportsDebugMessages() ? &m_messageDiagnostics : nullptr;
		}

		const RenderMessageDiagnostics* GetMessageDiagnostics() const noexcept override
		{
			return m_messageDiagnostics.SupportsDebugMessages() ? &m_messageDiagnostics : nullptr;
		}

		RenderFailureDiagnostics* GetFailureDiagnostics() noexcept override
		{
			return (m_failureDiagnostics.SupportsLiveObjectReports() || m_failureDiagnostics.SupportsCrashDiagnostics())
			           ? &m_failureDiagnostics
			           : nullptr;
		}

		const RenderFailureDiagnostics* GetFailureDiagnostics() const noexcept override
		{
			return (m_failureDiagnostics.SupportsLiveObjectReports() || m_failureDiagnostics.SupportsCrashDiagnostics())
			           ? &m_failureDiagnostics
			           : nullptr;
		}

	  private:
		D3D12RenderObjectDiagnostics m_objectDiagnostics;
		D3D12RenderTimingDiagnostics m_timingDiagnostics;
		D3D12RenderMessageDiagnostics m_messageDiagnostics;
		D3D12RenderFailureDiagnostics m_failureDiagnostics;
	};
}

class D3D12RenderHardwareInterface::D3D12RenderCommandList final : public RenderCommandList
{
  public:
	D3D12RenderCommandList(D3D12RenderHardwareInterface& owner, ID3D12GraphicsCommandList* commandList) noexcept :
	    m_owner(&owner), m_commandList(commandList)
	{
	}

	ERhiBackendApi GetBackendApi() const noexcept override { return ERhiBackendApi::D3D12; }
	NativeGraphicsCommandListHandle GetNativeHandle() const noexcept override { return NativeGraphicsCommandListHandle{m_commandList}; }
	bool SupportsDiagnosticScopes() const noexcept override { return m_commandList != nullptr && GetPixEventRuntime().IsAvailable(); }

	void BeginDiagnosticScope(std::string_view label, RhiDiagnosticLabelColor color) noexcept override
	{
		if (!SupportsDiagnosticScopes() || label.empty())
		{
			return;
		}

		const std::string ownedLabel(label);
		GetPixEventRuntime().BeginEvent(m_commandList, ToPixEventColor(color), ownedLabel.c_str());
	}

	void EndDiagnosticScope() noexcept override
	{
		if (SupportsDiagnosticScopes())
		{
			GetPixEventRuntime().EndEvent(m_commandList);
		}
	}

	void InsertDiagnosticMarker(std::string_view label, RhiDiagnosticLabelColor color) noexcept override
	{
		if (!SupportsDiagnosticScopes() || label.empty())
		{
			return;
		}

		const std::string ownedLabel(label);
		GetPixEventRuntime().SetMarker(m_commandList, ToPixEventColor(color), ownedLabel.c_str());
	}

	void SetDescriptorHeaps(std::uint32_t heapCount, const NativeDescriptorHeapHandle* heaps) noexcept override
	{
		if (m_commandList == nullptr)
		{
			return;
		}

		std::array<ID3D12DescriptorHeap*, 2> nativeHeaps{};
		for (std::uint32_t index = 0; index < heapCount && index < nativeHeaps.size(); ++index)
		{
			nativeHeaps[index] = static_cast<ID3D12DescriptorHeap*>(heaps[index].Value);
		}

		m_commandList->SetDescriptorHeaps(heapCount, nativeHeaps.data());
	}

	void SetPipelineState(const RenderPipelineState& pipelineState) noexcept override
	{
		if (m_commandList == nullptr)
		{
			return;
		}

		const auto& nativePipelineState = static_cast<const D3D12PipelineState&>(pipelineState);
		m_commandList->SetPipelineState(nativePipelineState.Get().Get());
	}

	void SetGraphicsBindingLayout(const RenderBindingLayout& bindingLayout) noexcept override
	{
		if (m_commandList == nullptr)
		{
			return;
		}

		const auto& nativeBindingLayout = static_cast<const D3D12BindingLayout&>(bindingLayout);
		m_commandList->SetGraphicsRootSignature(nativeBindingLayout.GetRootSignature().GetRaw());
	}

	void SetComputeBindingLayout(const RenderBindingLayout& bindingLayout) noexcept override
	{
		if (m_commandList == nullptr)
		{
			return;
		}

		const auto& nativeBindingLayout = static_cast<const D3D12BindingLayout&>(bindingLayout);
		m_commandList->SetComputeRootSignature(nativeBindingLayout.GetRootSignature().GetRaw());
	}

	void BindGraphicsConstantBuffer(std::uint32_t rootParameterIndex, RhiGpuVirtualAddress gpuAddress) noexcept override
	{
		if (m_commandList != nullptr)
		{
			m_commandList->SetGraphicsRootConstantBufferView(rootParameterIndex, gpuAddress);
		}
	}

	void BindGraphicsShaderResource(std::uint32_t rootParameterIndex, RhiGpuVirtualAddress gpuAddress) noexcept override
	{
		if (m_commandList != nullptr)
		{
			m_commandList->SetGraphicsRootShaderResourceView(rootParameterIndex, gpuAddress);
		}
	}

	void BindGraphicsUnorderedAccess(std::uint32_t rootParameterIndex, RhiGpuVirtualAddress gpuAddress) noexcept override
	{
		if (m_commandList != nullptr)
		{
			m_commandList->SetGraphicsRootUnorderedAccessView(rootParameterIndex, gpuAddress);
		}
	}

	void BindGraphicsDescriptorTable(std::uint32_t rootParameterIndex, RhiDescriptorTableHandle tableHandle) noexcept override
	{
		if (m_commandList == nullptr || m_owner == nullptr || !tableHandle)
		{
			return;
		}

		m_commandList->SetGraphicsRootDescriptorTable(rootParameterIndex, m_owner->ResolveDescriptorTableGpuHandle(tableHandle));
	}

	void BindGraphicsDescriptorTable(std::uint32_t rootParameterIndex, RhiGpuDescriptorHandle baseDescriptor) noexcept override
	{
		if (m_commandList != nullptr)
		{
			m_commandList->SetGraphicsRootDescriptorTable(rootParameterIndex, ToD3D12GpuDescriptor(baseDescriptor));
		}
	}

	void SetGraphicsRootConstants(
	    std::uint32_t rootParameterIndex,
	    std::uint32_t num32BitValues,
	    const void* data,
	    std::uint32_t destOffsetIn32BitValues) noexcept override
	{
		if (m_commandList != nullptr)
		{
			m_commandList->SetGraphicsRoot32BitConstants(rootParameterIndex, num32BitValues, data, destOffsetIn32BitValues);
		}
	}

	void BindComputeConstantBuffer(std::uint32_t rootParameterIndex, RhiGpuVirtualAddress gpuAddress) noexcept override
	{
		if (m_commandList != nullptr)
		{
			m_commandList->SetComputeRootConstantBufferView(rootParameterIndex, gpuAddress);
		}
	}

	void BindComputeShaderResource(std::uint32_t rootParameterIndex, RhiGpuVirtualAddress gpuAddress) noexcept override
	{
		if (m_commandList != nullptr)
		{
			m_commandList->SetComputeRootShaderResourceView(rootParameterIndex, gpuAddress);
		}
	}

	void BindComputeUnorderedAccess(std::uint32_t rootParameterIndex, RhiGpuVirtualAddress gpuAddress) noexcept override
	{
		if (m_commandList != nullptr)
		{
			m_commandList->SetComputeRootUnorderedAccessView(rootParameterIndex, gpuAddress);
		}
	}

	void BindComputeDescriptorTable(std::uint32_t rootParameterIndex, RhiDescriptorTableHandle tableHandle) noexcept override
	{
		if (m_commandList == nullptr || m_owner == nullptr || !tableHandle)
		{
			return;
		}

		m_commandList->SetComputeRootDescriptorTable(rootParameterIndex, m_owner->ResolveDescriptorTableGpuHandle(tableHandle));
	}

	void BindComputeDescriptorTable(std::uint32_t rootParameterIndex, RhiGpuDescriptorHandle baseDescriptor) noexcept override
	{
		if (m_commandList != nullptr)
		{
			m_commandList->SetComputeRootDescriptorTable(rootParameterIndex, ToD3D12GpuDescriptor(baseDescriptor));
		}
	}

	void SetComputeRootConstants(
	    std::uint32_t rootParameterIndex,
	    std::uint32_t num32BitValues,
	    const void* data,
	    std::uint32_t destOffsetIn32BitValues) noexcept override
	{
		if (m_commandList != nullptr)
		{
			m_commandList->SetComputeRoot32BitConstants(rootParameterIndex, num32BitValues, data, destOffsetIn32BitValues);
		}
	}

	void SetPrimitiveTopology(RhiPrimitiveTopology topology) noexcept override
	{
		if (m_commandList != nullptr)
		{
			m_commandList->IASetPrimitiveTopology(ToD3D12PrimitiveTopology(topology));
		}
	}

	void BindVertexBuffer(const RhiVertexBufferView& view) noexcept override
	{
		if (m_commandList == nullptr)
		{
			return;
		}

		const D3D12_VERTEX_BUFFER_VIEW nativeView{
		    .BufferLocation = view.BufferLocation,
		    .SizeInBytes = view.SizeInBytes,
		    .StrideInBytes = view.StrideInBytes};
		m_commandList->IASetVertexBuffers(0, 1, &nativeView);
	}

	void BindIndexBuffer(const RhiIndexBufferView& view) noexcept override
	{
		if (m_commandList == nullptr)
		{
			return;
		}

		const D3D12_INDEX_BUFFER_VIEW nativeView{
		    .BufferLocation = view.BufferLocation,
		    .SizeInBytes = view.SizeInBytes,
		    .Format = ToD3D12IndexFormat(view.Format)};
		m_commandList->IASetIndexBuffer(&nativeView);
	}

	void SetRenderTarget(RhiCpuDescriptorHandle rtv, const RhiCpuDescriptorHandle* dsv) noexcept override
	{
		if (m_commandList == nullptr)
		{
			return;
		}

		const D3D12_CPU_DESCRIPTOR_HANDLE nativeRtv = ToD3D12CpuDescriptor(rtv);
		const D3D12_CPU_DESCRIPTOR_HANDLE nativeDsv = dsv != nullptr ? ToD3D12CpuDescriptor(*dsv) : D3D12_CPU_DESCRIPTOR_HANDLE{};
		m_commandList->OMSetRenderTargets(1, &nativeRtv, FALSE, dsv != nullptr ? &nativeDsv : nullptr);
	}

	void SetRenderTargets(std::uint32_t numRTVs, const RhiCpuDescriptorHandle* rtvs, const RhiCpuDescriptorHandle* dsv) noexcept override
	{
		if (m_commandList == nullptr)
		{
			return;
		}

		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> nativeRtvs(numRTVs);
		for (std::uint32_t index = 0; index < numRTVs; ++index)
		{
			nativeRtvs[index] = ToD3D12CpuDescriptor(rtvs[index]);
		}

		const D3D12_CPU_DESCRIPTOR_HANDLE nativeDsv = dsv != nullptr ? ToD3D12CpuDescriptor(*dsv) : D3D12_CPU_DESCRIPTOR_HANDLE{};
		m_commandList->OMSetRenderTargets(numRTVs, nativeRtvs.data(), FALSE, dsv != nullptr ? &nativeDsv : nullptr);
	}

	void ClearRenderTarget(RhiCpuDescriptorHandle rtv, const float color[4]) noexcept override
	{
		if (m_commandList != nullptr)
		{
			m_commandList->ClearRenderTargetView(ToD3D12CpuDescriptor(rtv), color, 0, nullptr);
		}
	}

	void ClearDepthStencil(RhiCpuDescriptorHandle dsv, float depth, std::uint8_t stencil) noexcept override
	{
		if (m_commandList != nullptr)
		{
			m_commandList->ClearDepthStencilView(
			    ToD3D12CpuDescriptor(dsv),
			    D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
			    depth,
			    stencil,
			    0,
			    nullptr);
		}
	}

	void SetViewport(const RhiViewport& viewport) noexcept override
	{
		if (m_commandList == nullptr)
		{
			return;
		}

		const D3D12_VIEWPORT nativeViewport{
		    .TopLeftX = viewport.X,
		    .TopLeftY = viewport.Y,
		    .Width = viewport.Width,
		    .Height = viewport.Height,
		    .MinDepth = viewport.MinDepth,
		    .MaxDepth = viewport.MaxDepth};
		m_commandList->RSSetViewports(1, &nativeViewport);
	}

	void SetScissorRect(const RhiRect& rect) noexcept override
	{
		if (m_commandList == nullptr)
		{
			return;
		}

		const D3D12_RECT nativeRect{.left = rect.Left, .top = rect.Top, .right = rect.Right, .bottom = rect.Bottom};
		m_commandList->RSSetScissorRects(1, &nativeRect);
	}

	void DrawIndexedInstanced(
	    std::uint32_t indexCountPerInstance,
	    std::uint32_t instanceCount,
	    std::uint32_t startIndexLocation,
	    std::int32_t baseVertexLocation,
	    std::uint32_t startInstanceLocation) noexcept override
	{
		if (m_commandList != nullptr)
		{
			m_commandList
			    ->DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
		}
	}

	void DrawInstanced(
	    std::uint32_t vertexCountPerInstance,
	    std::uint32_t instanceCount,
	    std::uint32_t startVertexLocation,
	    std::uint32_t startInstanceLocation) noexcept override
	{
		if (m_commandList != nullptr)
		{
			m_commandList->DrawInstanced(vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);
		}
	}

	void Dispatch(std::uint32_t groupCountX, std::uint32_t groupCountY, std::uint32_t groupCountZ) noexcept override
	{
		if (m_commandList != nullptr)
		{
			m_commandList->Dispatch(groupCountX, groupCountY, groupCountZ);
		}
	}

	void CopyResource(NativeResourceHandle destinationResource, NativeResourceHandle sourceResource) noexcept override
	{
		if (m_commandList != nullptr)
		{
			m_commandList->CopyResource(ToD3D12Resource(destinationResource), ToD3D12Resource(sourceResource));
		}
	}

	void AliasResource(NativeResourceHandle beforeResource, NativeResourceHandle afterResource) noexcept override
	{
		if (m_commandList == nullptr)
		{
			return;
		}

		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Aliasing.pResourceBefore = ToD3D12Resource(beforeResource);
		barrier.Aliasing.pResourceAfter = ToD3D12Resource(afterResource);
		m_commandList->ResourceBarrier(1, &barrier);
	}

	void TransitionResource(NativeResourceHandle resource, ResourceState before, ResourceState after) noexcept override
	{
		if (m_commandList == nullptr)
		{
			return;
		}

		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = ToD3D12Resource(resource);
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = ToD3D12ResourceState(before);
		barrier.Transition.StateAfter = ToD3D12ResourceState(after);
		m_commandList->ResourceBarrier(1, &barrier);
	}

	void UnorderedAccessBarrier(NativeResourceHandle resource) noexcept override
	{
		if (m_commandList == nullptr)
		{
			return;
		}

		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.UAV.pResource = ToD3D12Resource(resource);
		m_commandList->ResourceBarrier(1, &barrier);
	}

  private:
	D3D12RenderHardwareInterface* m_owner = nullptr;
	ID3D12GraphicsCommandList* m_commandList = nullptr;
};

D3D12RenderHardwareInterface::D3D12RenderHardwareInterface(
    D3D12Rhi& rhi,
    D3D12DescriptorHeapManager& descriptorHeapManager,
    D3D12SwapChain& swapChain,
    D3D12ConstantBufferManager& constantBufferManager) noexcept :
    m_rhi(&rhi), m_descriptorHeapManager(&descriptorHeapManager), m_swapChain(&swapChain), m_constantBufferManager(&constantBufferManager)
{
	for (std::uint32_t frameIndex = 0; frameIndex < RenderConfig::FramesInFlight; ++frameIndex)
	{
		m_commandLists[frameIndex] = std::make_unique<D3D12RenderCommandList>(*this, rhi.GetCommandList(frameIndex).Get());
	}

	m_diagnostics = std::make_unique<D3D12RenderDiagnostics>(rhi);
}

ERhiBackendApi D3D12RenderHardwareInterface::GetBackendApi() const noexcept
{
	return ERhiBackendApi::D3D12;
}

std::uint32_t D3D12RenderHardwareInterface::GetCurrentFrameIndex() const noexcept
{
	return m_rhi != nullptr ? m_rhi->GetCurrentFrameIndex() : 0u;
}

void D3D12RenderHardwareInterface::WaitForIdle() noexcept
{
	if (m_rhi != nullptr)
	{
		m_rhi->Flush();
	}
}

NativeGraphicsDeviceHandle D3D12RenderHardwareInterface::GetDeviceHandle() const noexcept
{
	return NativeGraphicsDeviceHandle{m_rhi != nullptr ? m_rhi->GetDevice().Get() : nullptr};
}

NativeGraphicsQueueHandle D3D12RenderHardwareInterface::GetGraphicsQueueHandle() const noexcept
{
	return NativeGraphicsQueueHandle{m_rhi != nullptr ? m_rhi->GetCommandQueue().Get() : nullptr};
}

RenderCommandList& D3D12RenderHardwareInterface::GetGraphicsCommandList(std::uint32_t frameIndex) noexcept
{
	return *m_commandLists[frameIndex];
}

NativeGraphicsCommandListHandle D3D12RenderHardwareInterface::GetGraphicsCommandListHandle(std::uint32_t frameIndex) const noexcept
{
	return NativeGraphicsCommandListHandle{m_rhi != nullptr ? m_rhi->GetCommandList(frameIndex).Get() : nullptr};
}

RenderDiagnostics& D3D12RenderHardwareInterface::GetDiagnostics() noexcept
{
	return *m_diagnostics;
}

const RenderDiagnostics& D3D12RenderHardwareInterface::GetDiagnostics() const noexcept
{
	return *m_diagnostics;
}

std::unique_ptr<RenderBindingLayout> D3D12RenderHardwareInterface::CreateBindingLayout(const RenderBindingLayoutCompileDesc& desc)
{
	if (m_rhi == nullptr || desc.ParameterLayout == nullptr || desc.ShaderPackage == nullptr)
	{
		return {};
	}

	return D3D12BindingLayoutCompiler::Compile(*m_rhi, desc);
}

std::unique_ptr<RenderPipelineState> D3D12RenderHardwareInterface::CreateGraphicsPipelineState(const GraphicsPipelineStateDesc& desc)
{
	if (m_rhi == nullptr || desc.BindingLayout == nullptr || !desc.VertexShader.IsValid())
	{
		return {};
	}

	return std::make_unique<D3D12PipelineState>(*m_rhi, desc);
}

std::unique_ptr<RenderPipelineState> D3D12RenderHardwareInterface::CreateComputePipelineState(const ComputePipelineStateDesc& desc)
{
	if (m_rhi == nullptr || desc.BindingLayout == nullptr || !desc.ComputeShader.IsValid())
	{
		return {};
	}

	return std::make_unique<D3D12PipelineState>(*m_rhi, desc);
}

void D3D12RenderHardwareInterface::SetShaderVisibleDescriptorHeaps(RenderCommandList& commandList) const noexcept
{
	if (m_descriptorHeapManager != nullptr)
	{
		m_descriptorHeapManager->SetShaderVisibleHeaps(commandList);
	}
}

NativeDescriptorHeapHandle D3D12RenderHardwareInterface::GetShaderResourceHeapHandle() const noexcept
{
	if (m_descriptorHeapManager == nullptr)
	{
		return {};
	}

	D3D12DescriptorHeap* heap = m_descriptorHeapManager->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	return NativeDescriptorHeapHandle{heap != nullptr ? heap->GetRaw() : nullptr};
}

RhiDescriptorAllocation D3D12RenderHardwareInterface::AllocateDescriptor(ERhiDescriptorHeapType heapType)
{
	RhiDescriptorAllocation allocation{};
	if (m_descriptorHeapManager == nullptr)
	{
		return allocation;
	}

	const D3D12_DESCRIPTOR_HEAP_TYPE nativeType = ToNativeDescriptorHeapType(heapType);
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{};
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{};
	m_descriptorHeapManager->AllocateHandle(nativeType, cpuHandle, gpuHandle);
	allocation.CpuHandle = RhiCpuDescriptorHandle{cpuHandle.ptr};
	allocation.GpuHandle = RhiGpuDescriptorHandle{gpuHandle.ptr};
	return allocation;
}

void D3D12RenderHardwareInterface::ReleaseDescriptor(ERhiDescriptorHeapType heapType, const RhiDescriptorAllocation& allocation) noexcept
{
	if (m_descriptorHeapManager == nullptr || !allocation.CpuHandle)
	{
		return;
	}

	m_descriptorHeapManager->FreeHandle(
	    ToNativeDescriptorHeapType(heapType),
	    D3D12_CPU_DESCRIPTOR_HANDLE{allocation.CpuHandle.Value},
	    D3D12_GPU_DESCRIPTOR_HANDLE{allocation.GpuHandle.Value});
}

RhiDescriptorTableHandle D3D12RenderHardwareInterface::AllocateDescriptorTable(
    ERhiDescriptorHeapType heapType,
    std::uint32_t descriptorCount)
{
	if (m_descriptorHeapManager == nullptr || descriptorCount == 0)
	{
		return {};
	}

	const D3D12DescriptorHandle nativeHandle =
	    m_descriptorHeapManager->AllocateContiguous(ToD3D12DescriptorHeapType(heapType), descriptorCount);
	if (!nativeHandle.IsValid())
	{
		return {};
	}

	DescriptorTableRecord record{};
	record.heapType = heapType;
	record.descriptorCount = descriptorCount;
	record.nativeHandle = nativeHandle;

	if (!m_freeDescriptorTableIndices.empty())
	{
		const std::uint32_t recordIndex = m_freeDescriptorTableIndices.back();
		m_freeDescriptorTableIndices.pop_back();
		m_descriptorTableRecords[recordIndex] = record;
		return RhiDescriptorTableHandle{recordIndex + 1u};
	}

	m_descriptorTableRecords.push_back(record);
	return RhiDescriptorTableHandle{static_cast<std::uint32_t>(m_descriptorTableRecords.size())};
}

RhiCpuDescriptorHandle D3D12RenderHardwareInterface::GetDescriptorTableCpuHandle(
    RhiDescriptorTableHandle tableHandle,
    std::uint32_t descriptorIndex) const noexcept
{
	return RhiCpuDescriptorHandle{ResolveDescriptorTableCpuHandle(tableHandle, descriptorIndex).ptr};
}

void D3D12RenderHardwareInterface::ReleaseDescriptorTable(RhiDescriptorTableHandle tableHandle) noexcept
{
	DescriptorTableRecord* const record = FindDescriptorTableRecord(tableHandle);
	if (record == nullptr || m_descriptorHeapManager == nullptr || !record->IsAllocated())
	{
		return;
	}

	m_descriptorHeapManager->FreeContiguous(ToD3D12DescriptorHeapType(record->heapType), record->nativeHandle, record->descriptorCount);
	*record = DescriptorTableRecord{};
	m_freeDescriptorTableIndices.push_back(tableHandle.Value - 1u);
}

void D3D12RenderHardwareInterface::AllocateShaderResourceDescriptor(
    RhiCpuDescriptorHandle& outCpuHandle,
    RhiGpuDescriptorHandle& outGpuHandle)
{
	outCpuHandle = {};
	outGpuHandle = {};
	if (m_descriptorHeapManager == nullptr)
	{
		return;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{};
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{};
	m_descriptorHeapManager->AllocateHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, cpuHandle, gpuHandle);
	outCpuHandle.Value = cpuHandle.ptr;
	outGpuHandle.Value = gpuHandle.ptr;
}

void D3D12RenderHardwareInterface::ReleaseShaderResourceDescriptor(
    RhiCpuDescriptorHandle cpuHandle,
    RhiGpuDescriptorHandle gpuHandle) noexcept
{
	if (m_descriptorHeapManager == nullptr || !cpuHandle)
	{
		return;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE nativeCpuHandle{};
	nativeCpuHandle.ptr = cpuHandle.Value;
	D3D12_GPU_DESCRIPTOR_HANDLE nativeGpuHandle{};
	nativeGpuHandle.ptr = gpuHandle.Value;
	m_descriptorHeapManager->FreeHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, nativeCpuHandle, nativeGpuHandle);
}

const PerFrameConstantBufferData& D3D12RenderHardwareInterface::GetPerFrameConstantData() const noexcept
{
	static const PerFrameConstantBufferData emptyData{};
	return m_constantBufferManager != nullptr ? m_constantBufferManager->GetPerFrameData() : emptyData;
}

RhiGpuVirtualAddress D3D12RenderHardwareInterface::GetPerFrameConstantGpuAddress() const noexcept
{
	return m_constantBufferManager != nullptr ? m_constantBufferManager->GetPerFrameGpuAddress() : 0;
}

RhiGpuVirtualAddress D3D12RenderHardwareInterface::AllocatePerViewConstantBuffer(const PerViewConstantBufferData& data)
{
	return m_constantBufferManager != nullptr ? m_constantBufferManager->AllocatePerView(data) : 0;
}

RhiGpuVirtualAddress D3D12RenderHardwareInterface::AllocatePerObjectVertexConstants(const PerObjectVSConstantBufferData& data)
{
	return m_constantBufferManager != nullptr ? m_constantBufferManager->UpdatePerObjectVS(data) : 0;
}

RhiGpuVirtualAddress D3D12RenderHardwareInterface::AllocatePerObjectPixelConstants(const PerObjectPSConstantBufferData& data)
{
	return m_constantBufferManager != nullptr ? m_constantBufferManager->UpdatePerObjectPS(data) : 0;
}

RhiDescriptorTableHandle D3D12RenderHardwareInterface::GetSamplerTableHandle() const noexcept
{
	return m_samplerTableHandle;
}

RhiViewport D3D12RenderHardwareInterface::GetBackBufferViewport() const noexcept
{
	return m_swapChain != nullptr ? m_swapChain->GetDefaultViewport() : RhiViewport{};
}

RhiRect D3D12RenderHardwareInterface::GetBackBufferScissorRect() const noexcept
{
	return m_swapChain != nullptr ? m_swapChain->GetDefaultScissorRect() : RhiRect{};
}

RhiCpuDescriptorHandle D3D12RenderHardwareInterface::GetBackBufferRenderTargetView() const noexcept
{
	return m_swapChain != nullptr ? RhiCpuDescriptorHandle{m_swapChain->GetCPUHandle().ptr} : RhiCpuDescriptorHandle{};
}

NativeResourceHandle D3D12RenderHardwareInterface::GetBackBufferResource() const noexcept
{
	return NativeResourceHandle{m_swapChain != nullptr ? m_swapChain->GetCurrentResource() : nullptr};
}

std::unique_ptr<Texture> D3D12RenderHardwareInterface::CreateTextureFromPath(const std::filesystem::path& texturePath) const
{
	if (m_rhi == nullptr || m_descriptorHeapManager == nullptr)
	{
		return {};
	}

	TextureLoadResult loadResult = TextureLoader::Load(texturePath);
	if (!loadResult.IsValid())
	{
		return {};
	}

	std::unique_ptr<TextureFactory> textureFactory = TextureFactory::Create(*m_rhi, *m_descriptorHeapManager);
	return textureFactory != nullptr ? textureFactory->CreateTexture(std::move(loadResult)) : std::unique_ptr<Texture>{};
}

bool D3D12RenderHardwareInterface::CreateVertexBuffer(
    const void* data,
    std::size_t sizeInBytes,
    std::uint32_t strideInBytes,
    std::wstring_view debugName,
    RhiOwnedResourceHandle& outResource,
    RhiVertexBufferView& outView)
{
	outResource = {};
	outView = {};
	if (m_rhi == nullptr || data == nullptr || sizeInBytes == 0 || strideInBytes == 0)
	{
		return false;
	}

	D3D12_RESOURCE_DESC resourceDesc = BuildBufferResourceDesc(RhiBufferResourceDesc{.SizeInBytes = sizeInBytes});
	const D3D12_HEAP_PROPERTIES heapProperties = BuildUploadHeapProperties();
	auto ownedResource = std::make_unique<OwnedResourceState>();
	if (FAILED(m_rhi->GetDevice()->CreateCommittedResource(
	        &heapProperties,
	        D3D12_HEAP_FLAG_NONE,
	        &resourceDesc,
	        D3D12_RESOURCE_STATE_GENERIC_READ,
	        nullptr,
	        IID_PPV_ARGS(ownedResource->Resource.ReleaseAndGetAddressOf()))))
	{
		return false;
	}

	ownedResource->Resource->SetName(CopyDebugName(debugName, L"VertexBuffer").c_str());
	void* mappedData = nullptr;
	const D3D12_RANGE readRange{0, 0};
	if (FAILED(ownedResource->Resource->Map(0, &readRange, &mappedData)))
	{
		return false;
	}

	std::memcpy(mappedData, data, sizeInBytes);
	ownedResource->Resource->Unmap(0, nullptr);

	outView = RhiVertexBufferView{
	    .BufferLocation = ownedResource->Resource->GetGPUVirtualAddress(),
	    .SizeInBytes = static_cast<std::uint32_t>(sizeInBytes),
	    .StrideInBytes = strideInBytes};
	outResource = RhiOwnedResourceHandle{ownedResource.release()};
	return true;
}

bool D3D12RenderHardwareInterface::CreateIndexBuffer(
    const void* data,
    std::size_t sizeInBytes,
    RhiIndexFormat format,
    std::wstring_view debugName,
    RhiOwnedResourceHandle& outResource,
    RhiIndexBufferView& outView)
{
	outResource = {};
	outView = {};
	if (m_rhi == nullptr || data == nullptr || sizeInBytes == 0)
	{
		return false;
	}

	D3D12_RESOURCE_DESC resourceDesc = BuildBufferResourceDesc(RhiBufferResourceDesc{.SizeInBytes = sizeInBytes});
	const D3D12_HEAP_PROPERTIES heapProperties = BuildUploadHeapProperties();
	auto ownedResource = std::make_unique<OwnedResourceState>();
	if (FAILED(m_rhi->GetDevice()->CreateCommittedResource(
	        &heapProperties,
	        D3D12_HEAP_FLAG_NONE,
	        &resourceDesc,
	        D3D12_RESOURCE_STATE_GENERIC_READ,
	        nullptr,
	        IID_PPV_ARGS(ownedResource->Resource.ReleaseAndGetAddressOf()))))
	{
		return false;
	}

	ownedResource->Resource->SetName(CopyDebugName(debugName, L"IndexBuffer").c_str());
	void* mappedData = nullptr;
	const D3D12_RANGE readRange{0, 0};
	if (FAILED(ownedResource->Resource->Map(0, &readRange, &mappedData)))
	{
		return false;
	}

	std::memcpy(mappedData, data, sizeInBytes);
	ownedResource->Resource->Unmap(0, nullptr);

	outView = RhiIndexBufferView{
	    .BufferLocation = ownedResource->Resource->GetGPUVirtualAddress(),
	    .SizeInBytes = static_cast<std::uint32_t>(sizeInBytes),
	    .Format = format};
	outResource = RhiOwnedResourceHandle{ownedResource.release()};
	return true;
}

void D3D12RenderHardwareInterface::ReleaseOwnedResource(RhiOwnedResourceHandle resource) noexcept
{
	delete ToOwnedResourceState(resource);
}

NativeResourceHandle D3D12RenderHardwareInterface::GetNativeResource(RhiOwnedResourceHandle resource) const noexcept
{
	const OwnedResourceState* ownedResource = ToConstOwnedResourceState(resource);
	return NativeResourceHandle{ownedResource != nullptr ? ownedResource->Resource.Get() : nullptr};
}

RhiResourceAllocationInfo D3D12RenderHardwareInterface::GetTextureAllocationInfo(const RhiTextureResourceDesc& desc) const noexcept
{
	if (m_rhi == nullptr)
	{
		return {};
	}

	const D3D12_RESOURCE_DESC resourceDesc = BuildTextureResourceDesc(desc);
	const D3D12_RESOURCE_ALLOCATION_INFO allocationInfo = m_rhi->GetDevice()->GetResourceAllocationInfo(0, 1, &resourceDesc);
	return RhiResourceAllocationInfo{.SizeInBytes = allocationInfo.SizeInBytes, .Alignment = allocationInfo.Alignment};
}

RhiResourceAllocationInfo D3D12RenderHardwareInterface::GetBufferAllocationInfo(const RhiBufferResourceDesc& desc) const noexcept
{
	if (m_rhi == nullptr)
	{
		return {};
	}

	const D3D12_RESOURCE_DESC resourceDesc = BuildBufferResourceDesc(desc);
	const D3D12_RESOURCE_ALLOCATION_INFO allocationInfo = m_rhi->GetDevice()->GetResourceAllocationInfo(0, 1, &resourceDesc);
	return RhiResourceAllocationInfo{.SizeInBytes = allocationInfo.SizeInBytes, .Alignment = allocationInfo.Alignment};
}

RhiOwnedHeapHandle D3D12RenderHardwareInterface::CreateOwnedHeap(
    RhiTransientAllocationPool pool,
    std::uint64_t sizeInBytes,
    std::uint64_t alignment,
    std::wstring_view debugName)
{
	if (m_rhi == nullptr || sizeInBytes == 0)
	{
		return {};
	}

	auto ownedHeap = std::make_unique<OwnedHeapState>();
	D3D12_HEAP_DESC heapDesc{};
	heapDesc.SizeInBytes = sizeInBytes;
	heapDesc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapDesc.Properties.CreationNodeMask = 0;
	heapDesc.Properties.VisibleNodeMask = 0;
	heapDesc.Alignment = alignment;
	heapDesc.Flags = ToHeapFlags(pool);
	if (FAILED(m_rhi->GetDevice()->CreateHeap(&heapDesc, IID_PPV_ARGS(ownedHeap->Heap.ReleaseAndGetAddressOf()))))
	{
		return {};
	}

	ownedHeap->Heap->SetName(CopyDebugName(debugName, L"TransientHeap").c_str());
	return RhiOwnedHeapHandle{ownedHeap.release()};
}

void D3D12RenderHardwareInterface::ReleaseOwnedHeap(RhiOwnedHeapHandle heap) noexcept
{
	delete ToOwnedHeapState(heap);
}

RhiOwnedResourceHandle D3D12RenderHardwareInterface::CreatePlacedTextureResource(
    RhiOwnedHeapHandle heap,
    std::uint64_t heapOffset,
    const RhiTransientTextureAllocationDesc& desc,
    std::wstring_view debugName)
{
	OwnedHeapState* ownedHeap = ToOwnedHeapState(heap);
	if (m_rhi == nullptr || ownedHeap == nullptr || ownedHeap->Heap == nullptr)
	{
		return {};
	}

	auto ownedResource = std::make_unique<OwnedResourceState>();
	const D3D12_RESOURCE_DESC resourceDesc = BuildTextureResourceDesc(desc.ResourceDesc);
	const D3D12_CLEAR_VALUE clearValue = BuildClearValue(desc.ClearValue);
	const D3D12_CLEAR_VALUE* clearValuePtr = desc.ClearValue.ValueType == RhiOptimizedClearValue::Type::None ? nullptr : &clearValue;
	if (FAILED(m_rhi->GetDevice()->CreatePlacedResource(
	        ownedHeap->Heap.Get(),
	        heapOffset,
	        &resourceDesc,
	        ToD3D12ResourceState(desc.InitialState),
	        clearValuePtr,
	        IID_PPV_ARGS(ownedResource->Resource.ReleaseAndGetAddressOf()))))
	{
		return {};
	}

	ownedResource->Resource->SetName(CopyDebugName(debugName, L"PlacedTexture").c_str());
	return RhiOwnedResourceHandle{ownedResource.release()};
}

RhiOwnedResourceHandle D3D12RenderHardwareInterface::CreatePlacedBufferResource(
    RhiOwnedHeapHandle heap,
    std::uint64_t heapOffset,
    const RhiTransientBufferAllocationDesc& desc,
    std::wstring_view debugName)
{
	OwnedHeapState* ownedHeap = ToOwnedHeapState(heap);
	if (m_rhi == nullptr || ownedHeap == nullptr || ownedHeap->Heap == nullptr)
	{
		return {};
	}

	auto ownedResource = std::make_unique<OwnedResourceState>();
	const D3D12_RESOURCE_DESC resourceDesc = BuildBufferResourceDesc(desc.ResourceDesc);
	if (FAILED(m_rhi->GetDevice()->CreatePlacedResource(
	        ownedHeap->Heap.Get(),
	        heapOffset,
	        &resourceDesc,
	        ToD3D12ResourceState(desc.InitialState),
	        nullptr,
	        IID_PPV_ARGS(ownedResource->Resource.ReleaseAndGetAddressOf()))))
	{
		return {};
	}

	ownedResource->Resource->SetName(CopyDebugName(debugName, L"PlacedBuffer").c_str());
	return RhiOwnedResourceHandle{ownedResource.release()};
}

void D3D12RenderHardwareInterface::CreateRenderTargetView(
    NativeResourceHandle resource,
    PixelFormat format,
    RhiCpuDescriptorHandle destination)
{
	if (m_rhi == nullptr || !resource || !destination)
	{
		return;
	}

	D3D12_RENDER_TARGET_VIEW_DESC viewDesc{};
	viewDesc.Format = D3D12TypeConversions::ToDxgiFormat(format);
	viewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	m_rhi->GetDevice()->CreateRenderTargetView(ToD3D12Resource(resource), &viewDesc, ToD3D12CpuDescriptor(destination));
}

void D3D12RenderHardwareInterface::CreateDepthStencilView(
    NativeResourceHandle resource,
    PixelFormat format,
    RhiCpuDescriptorHandle destination)
{
	if (m_rhi == nullptr || !resource || !destination)
	{
		return;
	}

	D3D12_DEPTH_STENCIL_VIEW_DESC viewDesc{};
	viewDesc.Format = D3D12TypeConversions::ToDxgiFormat(format);
	viewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	viewDesc.Flags = D3D12_DSV_FLAG_NONE;
	m_rhi->GetDevice()->CreateDepthStencilView(ToD3D12Resource(resource), &viewDesc, ToD3D12CpuDescriptor(destination));
}

void D3D12RenderHardwareInterface::CreateTextureShaderResourceView(
    NativeResourceHandle resource,
    PixelFormat format,
    RhiCpuDescriptorHandle destination)
{
	if (m_rhi == nullptr || !resource || !destination)
	{
		return;
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc{};
	viewDesc.Format = D3D12TypeConversions::ToDxgiFormat(format);
	viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	viewDesc.Texture2D.MostDetailedMip = 0;
	viewDesc.Texture2D.MipLevels = 1;
	m_rhi->GetDevice()->CreateShaderResourceView(ToD3D12Resource(resource), &viewDesc, ToD3D12CpuDescriptor(destination));
}

void D3D12RenderHardwareInterface::CreateTextureUnorderedAccessView(
    NativeResourceHandle resource,
    PixelFormat format,
    RhiCpuDescriptorHandle destination)
{
	if (m_rhi == nullptr || !resource || !destination)
	{
		return;
	}

	D3D12_UNORDERED_ACCESS_VIEW_DESC viewDesc{};
	viewDesc.Format = D3D12TypeConversions::ToDxgiFormat(format);
	viewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	viewDesc.Texture2D.MipSlice = 0;
	viewDesc.Texture2D.PlaneSlice = 0;
	m_rhi->GetDevice()->CreateUnorderedAccessView(ToD3D12Resource(resource), nullptr, &viewDesc, ToD3D12CpuDescriptor(destination));
}

void D3D12RenderHardwareInterface::CreateBufferShaderResourceView(
    NativeResourceHandle resource,
    std::uint64_t sizeInBytes,
    std::uint32_t strideInBytes,
    RhiCpuDescriptorHandle destination)
{
	if (m_rhi == nullptr || !resource || !destination || sizeInBytes == 0)
	{
		return;
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc{};
	viewDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	if (strideInBytes > 0)
	{
		viewDesc.Format = DXGI_FORMAT_UNKNOWN;
		viewDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		viewDesc.Buffer.StructureByteStride = strideInBytes;
		viewDesc.Buffer.NumElements = static_cast<UINT>(sizeInBytes / strideInBytes);
	}
	else
	{
		viewDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		viewDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
		viewDesc.Buffer.StructureByteStride = 0;
		viewDesc.Buffer.NumElements = static_cast<UINT>(sizeInBytes / sizeof(std::uint32_t));
	}
	m_rhi->GetDevice()->CreateShaderResourceView(ToD3D12Resource(resource), &viewDesc, ToD3D12CpuDescriptor(destination));
}

void D3D12RenderHardwareInterface::CreateBufferUnorderedAccessView(
    NativeResourceHandle resource,
    std::uint64_t sizeInBytes,
    std::uint32_t strideInBytes,
    RhiCpuDescriptorHandle destination)
{
	if (m_rhi == nullptr || !resource || !destination || sizeInBytes == 0)
	{
		return;
	}

	D3D12_UNORDERED_ACCESS_VIEW_DESC viewDesc{};
	viewDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	if (strideInBytes > 0)
	{
		viewDesc.Format = DXGI_FORMAT_UNKNOWN;
		viewDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
		viewDesc.Buffer.StructureByteStride = strideInBytes;
		viewDesc.Buffer.NumElements = static_cast<UINT>(sizeInBytes / strideInBytes);
	}
	else
	{
		viewDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		viewDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
		viewDesc.Buffer.StructureByteStride = 0;
		viewDesc.Buffer.NumElements = static_cast<UINT>(sizeInBytes / sizeof(std::uint32_t));
	}
	m_rhi->GetDevice()->CreateUnorderedAccessView(ToD3D12Resource(resource), nullptr, &viewDesc, ToD3D12CpuDescriptor(destination));
}

bool D3D12RenderHardwareInterface::SupportsUnorderedAccess(NativeResourceHandle resource) const noexcept
{
	return ResourceSupportsUnorderedAccess(ToD3D12Resource(resource));
}

void D3D12RenderHardwareInterface::TransitionResource(
    NativeGraphicsCommandListHandle commandList,
    NativeResourceHandle resource,
    ResourceState before,
    ResourceState after) const noexcept
{
	ID3D12GraphicsCommandList* const nativeCommandList = ToD3D12GraphicsCommandList(commandList);
	ID3D12Resource* const nativeResource = ToD3D12Resource(resource);
	if (nativeCommandList == nullptr || nativeResource == nullptr)
	{
		return;
	}

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = nativeResource;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = ToD3D12ResourceState(before);
	barrier.Transition.StateAfter = ToD3D12ResourceState(after);
	nativeCommandList->ResourceBarrier(1, &barrier);
}

void D3D12RenderHardwareInterface::BeginPresentRenderPass(NativeGraphicsCommandListHandle commandList, const float clearColor[4])
    const noexcept
{
	if (m_swapChain == nullptr || !commandList)
	{
		return;
	}

	auto* nativeCommandList = ToD3D12GraphicsCommandList(commandList);
	ID3D12Resource* presentTexture = m_swapChain->GetCurrentResource();
	if (nativeCommandList == nullptr || presentTexture == nullptr)
	{
		return;
	}

	D3D12_RESOURCE_BARRIER transitionToRenderTarget{};
	transitionToRenderTarget.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	transitionToRenderTarget.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	transitionToRenderTarget.Transition.pResource = presentTexture;
	transitionToRenderTarget.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	transitionToRenderTarget.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	transitionToRenderTarget.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	nativeCommandList->ResourceBarrier(1, &transitionToRenderTarget);

	BindPresentDescriptorHeaps(*nativeCommandList);

	const D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView = m_swapChain->GetCPUHandle();
	nativeCommandList->OMSetRenderTargets(1, &renderTargetView, FALSE, nullptr);

	static constexpr float defaultClearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
	nativeCommandList->ClearRenderTargetView(renderTargetView, clearColor != nullptr ? clearColor : defaultClearColor, 0, nullptr);
}

void D3D12RenderHardwareInterface::BeginPresentOverlayPass(NativeGraphicsCommandListHandle commandList) const noexcept
{
	if (m_swapChain == nullptr || !commandList)
	{
		return;
	}

	auto* nativeCommandList = ToD3D12GraphicsCommandList(commandList);
	ID3D12Resource* presentTexture = m_swapChain->GetCurrentResource();
	if (nativeCommandList == nullptr || presentTexture == nullptr)
	{
		return;
	}

	D3D12_RESOURCE_BARRIER transitionToRenderTarget{};
	transitionToRenderTarget.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	transitionToRenderTarget.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	transitionToRenderTarget.Transition.pResource = presentTexture;
	transitionToRenderTarget.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	transitionToRenderTarget.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	transitionToRenderTarget.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	nativeCommandList->ResourceBarrier(1, &transitionToRenderTarget);

	BindPresentDescriptorHeaps(*nativeCommandList);

	const D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView = m_swapChain->GetCPUHandle();
	nativeCommandList->OMSetRenderTargets(1, &renderTargetView, FALSE, nullptr);
}

void D3D12RenderHardwareInterface::EndPresentRenderPass(NativeGraphicsCommandListHandle commandList) const noexcept
{
	if (m_swapChain == nullptr || !commandList)
	{
		return;
	}

	auto* nativeCommandList = ToD3D12GraphicsCommandList(commandList);
	ID3D12Resource* presentTexture = m_swapChain->GetCurrentResource();
	if (nativeCommandList == nullptr || presentTexture == nullptr)
	{
		return;
	}

	D3D12_RESOURCE_BARRIER transitionToPresent{};
	transitionToPresent.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	transitionToPresent.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	transitionToPresent.Transition.pResource = presentTexture;
	transitionToPresent.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	transitionToPresent.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	transitionToPresent.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	nativeCommandList->ResourceBarrier(1, &transitionToPresent);
}

void D3D12RenderHardwareInterface::BindPresentDescriptorHeaps(ID3D12GraphicsCommandList& commandList) const noexcept
{
	ID3D12DescriptorHeap* heaps[2] = {};
	UINT heapCount = 0;
	if (m_descriptorHeapManager != nullptr)
	{
		if (D3D12DescriptorHeap* srvHeap = m_descriptorHeapManager->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV))
		{
			heaps[heapCount++] = srvHeap->GetRaw();
		}

		if (D3D12DescriptorHeap* samplerHeap = m_descriptorHeapManager->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER))
		{
			heaps[heapCount++] = samplerHeap->GetRaw();
		}
	}

	if (heapCount > 0)
	{
		commandList.SetDescriptorHeaps(heapCount, heaps);
	}
}

PixelFormat D3D12RenderHardwareInterface::GetPresentColorFormat() const noexcept
{
	return m_swapChain != nullptr ? m_swapChain->GetBackBufferFormat() : PixelFormat::Unknown;
}

void D3D12RenderHardwareInterface::SetSamplerTableHandle(RhiDescriptorTableHandle samplerTableHandle) noexcept
{
	m_samplerTableHandle = samplerTableHandle;
}

D3D12RenderHardwareInterface::DescriptorTableRecord* D3D12RenderHardwareInterface::FindDescriptorTableRecord(
    RhiDescriptorTableHandle tableHandle) noexcept
{
	if (!tableHandle || tableHandle.Value == 0 || tableHandle.Value > m_descriptorTableRecords.size())
	{
		return nullptr;
	}

	DescriptorTableRecord& record = m_descriptorTableRecords[tableHandle.Value - 1u];
	return record.IsAllocated() ? &record : nullptr;
}

const D3D12RenderHardwareInterface::DescriptorTableRecord* D3D12RenderHardwareInterface::FindDescriptorTableRecord(
    RhiDescriptorTableHandle tableHandle) const noexcept
{
	if (!tableHandle || tableHandle.Value == 0 || tableHandle.Value > m_descriptorTableRecords.size())
	{
		return nullptr;
	}

	const DescriptorTableRecord& record = m_descriptorTableRecords[tableHandle.Value - 1u];
	return record.IsAllocated() ? &record : nullptr;
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12RenderHardwareInterface::ResolveDescriptorTableCpuHandle(
    RhiDescriptorTableHandle tableHandle,
    std::uint32_t descriptorIndex) const noexcept
{
	const DescriptorTableRecord* const record = FindDescriptorTableRecord(tableHandle);
	if (record == nullptr || descriptorIndex >= record->descriptorCount)
	{
		return D3D12_CPU_DESCRIPTOR_HANDLE{};
	}

	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = record->nativeHandle.GetCPU();
	cpuHandle.ptr += static_cast<SIZE_T>(descriptorIndex) * record->nativeHandle.GetIncrementSize();
	return cpuHandle;
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12RenderHardwareInterface::ResolveDescriptorTableGpuHandle(
    RhiDescriptorTableHandle tableHandle) const noexcept
{
	const DescriptorTableRecord* const record = FindDescriptorTableRecord(tableHandle);
	return record != nullptr ? record->nativeHandle.GetGPU() : D3D12_GPU_DESCRIPTOR_HANDLE{};
}

D3D12_DESCRIPTOR_HEAP_TYPE D3D12RenderHardwareInterface::ToNativeDescriptorHeapType(ERhiDescriptorHeapType heapType) noexcept
{
	return ToD3D12DescriptorHeapType(heapType);
}