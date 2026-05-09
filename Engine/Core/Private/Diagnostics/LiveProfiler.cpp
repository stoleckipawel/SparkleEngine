#include "PCH.h"

#include "Core/Public/Diagnostics/LiveProfiler.h"

#include <algorithm>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

namespace Diagnostics
{
	namespace
	{
		constexpr double kEmaAlpha = 0.1;

		std::uint32_t GetCurrentThreadIdValue() noexcept
		{
			static thread_local const std::uint32_t threadId = []
			{
				const std::hash<std::thread::id> hasher;
				return static_cast<std::uint32_t>(hasher(std::this_thread::get_id()));
			}();
			return threadId;
		}

		// Internal mutable node: children stored by std::unique_ptr so pointers stay
		// stable across insertions (the producer holds a raw pointer in its stack).
		struct LiveNode final
		{
			std::string Name;
			LiveNode* Parent = nullptr;
			double LastDurationMicroseconds = 0.0;
			double AverageDurationMicroseconds = 0.0;
			double MaxDurationMicroseconds = 0.0;
			std::uint64_t TotalCallCount = 0;
			std::uint64_t DrawCallCount = 0;
			std::uint64_t IndexedDrawCount = 0;
			std::uint64_t TotalVertexCount = 0;
			std::uint64_t TotalInstanceCount = 0;
			std::uint64_t DispatchCount = 0;
			std::uint64_t TotalThreadGroupCount = 0;
			std::vector<std::unique_ptr<LiveNode>> Children;

			LiveNode* FindOrCreateChild(std::string_view name) noexcept
			{
				for (const std::unique_ptr<LiveNode>& child : Children)
				{
					if (child->Name == name)
					{
						return child.get();
					}
				}

				auto created = std::make_unique<LiveNode>();
				created->Name.assign(name);
				created->Parent = this;
				LiveNode* result = created.get();
				Children.push_back(std::move(created));
				return result;
			}

			void RecordSample(std::uint64_t durationMicroseconds) noexcept
			{
				const double sample = static_cast<double>(durationMicroseconds);
				LastDurationMicroseconds = sample;
				MaxDurationMicroseconds = std::max(MaxDurationMicroseconds, sample);
				if (TotalCallCount == 0)
				{
					AverageDurationMicroseconds = sample;
				}
				else
				{
					AverageDurationMicroseconds = (AverageDurationMicroseconds * (1.0 - kEmaAlpha)) + (sample * kEmaAlpha);
				}
				++TotalCallCount;
			}
		};

		void CopyToSnapshot(const LiveNode& source, ProfilerSnapshotNode& destination)
		{
			destination.Name = source.Name;
			destination.LastDurationMicroseconds = source.LastDurationMicroseconds;
			destination.AverageDurationMicroseconds = source.AverageDurationMicroseconds;
			destination.MaxDurationMicroseconds = source.MaxDurationMicroseconds;
			destination.TotalCallCount = source.TotalCallCount;
			destination.DrawCallCount = source.DrawCallCount;
			destination.IndexedDrawCount = source.IndexedDrawCount;
			destination.TotalVertexCount = source.TotalVertexCount;
			destination.TotalInstanceCount = source.TotalInstanceCount;
			destination.DispatchCount = source.DispatchCount;
			destination.TotalThreadGroupCount = source.TotalThreadGroupCount;
			destination.Children.reserve(source.Children.size());
			for (const std::unique_ptr<LiveNode>& child : source.Children)
			{
				ProfilerSnapshotNode childSnapshot;
				CopyToSnapshot(*child, childSnapshot);
				destination.Children.push_back(std::move(childSnapshot));
			}
		}

		// Per-thread CPU profile. Producer owns the stack pointer; the snapshot
		// reader takes the mutex briefly to copy the tree.
		struct CpuThreadProfile final
		{
			mutable std::mutex Mutex;
			std::unique_ptr<LiveNode> Root = std::make_unique<LiveNode>();
			LiveNode* CurrentNode = Root.get();
		};
	}

	struct LiveProfiler::State final
	{
		mutable std::mutex CpuRegistryMutex;
		std::unordered_map<std::uint32_t, std::unique_ptr<CpuThreadProfile>> CpuThreadProfiles;

		mutable std::mutex GpuMutex;
		std::unique_ptr<LiveNode> GpuRoot = std::make_unique<LiveNode>();

		CpuThreadProfile& GetOrCreateCpuProfile(std::uint32_t threadId)
		{
			{
				std::lock_guard<std::mutex> lock(CpuRegistryMutex);
				auto it = CpuThreadProfiles.find(threadId);
				if (it != CpuThreadProfiles.end())
				{
					return *it->second;
				}
				auto created = std::make_unique<CpuThreadProfile>();
				CpuThreadProfile* result = created.get();
				CpuThreadProfiles.emplace(threadId, std::move(created));
				return *result;
			}
		}
	};

	LiveProfiler::LiveProfiler() noexcept : m_state(std::make_unique<State>()) {}

	LiveProfiler::~LiveProfiler() noexcept = default;

	LiveProfiler& LiveProfiler::Get() noexcept
	{
		// Leaked on purpose so producers running during shutdown remain safe.
		static LiveProfiler* instance = new LiveProfiler();
		return *instance;
	}

	void LiveProfiler::BeginCpuScope(std::string_view name) noexcept
	{
		if (!IsEnabled() || name.empty() || m_state == nullptr)
		{
			return;
		}

		const std::uint32_t threadId = GetCurrentThreadIdValue();
		CpuThreadProfile& profile = m_state->GetOrCreateCpuProfile(threadId);

		std::lock_guard<std::mutex> lock(profile.Mutex);
		LiveNode* parent = profile.CurrentNode != nullptr ? profile.CurrentNode : profile.Root.get();
		profile.CurrentNode = parent->FindOrCreateChild(name);
	}

	void LiveProfiler::EndCpuScope(std::uint64_t durationMicroseconds) noexcept
	{
		if (!IsEnabled() || m_state == nullptr)
		{
			return;
		}

		const std::uint32_t threadId = GetCurrentThreadIdValue();
		CpuThreadProfile* profile = nullptr;
		{
			std::lock_guard<std::mutex> registryLock(m_state->CpuRegistryMutex);
			auto it = m_state->CpuThreadProfiles.find(threadId);
			if (it == m_state->CpuThreadProfiles.end())
			{
				return;
			}
			profile = it->second.get();
		}

		std::lock_guard<std::mutex> lock(profile->Mutex);
		if (profile->CurrentNode == nullptr || profile->CurrentNode == profile->Root.get())
		{
			return;
		}

		profile->CurrentNode->RecordSample(durationMicroseconds);
		profile->CurrentNode = profile->CurrentNode->Parent != nullptr ? profile->CurrentNode->Parent : profile->Root.get();
	}

	void LiveProfiler::AccumulateDrawCall(std::uint32_t vertexCountPerInstance, std::uint32_t instanceCount, bool indexed) noexcept
	{
		if (!IsEnabled() || m_state == nullptr)
		{
			return;
		}

		const std::uint32_t threadId = GetCurrentThreadIdValue();
		CpuThreadProfile* profile = nullptr;
		{
			std::lock_guard<std::mutex> registryLock(m_state->CpuRegistryMutex);
			auto it = m_state->CpuThreadProfiles.find(threadId);
			if (it == m_state->CpuThreadProfiles.end())
			{
				return;
			}
			profile = it->second.get();
		}

		std::lock_guard<std::mutex> lock(profile->Mutex);
		if (profile->CurrentNode == nullptr || profile->CurrentNode == profile->Root.get())
		{
			return;
		}

		LiveNode* node = profile->CurrentNode;
		++node->DrawCallCount;
		if (indexed)
		{
			++node->IndexedDrawCount;
		}
		node->TotalVertexCount += static_cast<std::uint64_t>(vertexCountPerInstance) * instanceCount;
		node->TotalInstanceCount += instanceCount;
	}

	void LiveProfiler::AccumulateDispatch(std::uint32_t groupCountX, std::uint32_t groupCountY, std::uint32_t groupCountZ) noexcept
	{
		if (!IsEnabled() || m_state == nullptr)
		{
			return;
		}

		const std::uint32_t threadId = GetCurrentThreadIdValue();
		CpuThreadProfile* profile = nullptr;
		{
			std::lock_guard<std::mutex> registryLock(m_state->CpuRegistryMutex);
			auto it = m_state->CpuThreadProfiles.find(threadId);
			if (it == m_state->CpuThreadProfiles.end())
			{
				return;
			}
			profile = it->second.get();
		}

		std::lock_guard<std::mutex> lock(profile->Mutex);
		if (profile->CurrentNode == nullptr || profile->CurrentNode == profile->Root.get())
		{
			return;
		}

		LiveNode* node = profile->CurrentNode;
		++node->DispatchCount;
		node->TotalThreadGroupCount +=
		    static_cast<std::uint64_t>(groupCountX) * static_cast<std::uint64_t>(groupCountY) * static_cast<std::uint64_t>(groupCountZ);
	}

	void LiveProfiler::SubmitGpuFrame(const GpuTimingEntry* entries, std::size_t count) noexcept
	{
		if (!IsEnabled() || entries == nullptr || count == 0 || m_state == nullptr)
		{
			return;
		}

		std::lock_guard<std::mutex> lock(m_state->GpuMutex);

		// Walk a stack of LiveNode*; index by depth. Index 0 is the synthetic root.
		std::vector<LiveNode*> stack;
		stack.reserve(8);
		stack.push_back(m_state->GpuRoot.get());

		for (std::size_t i = 0; i < count; ++i)
		{
			const GpuTimingEntry& entry = entries[i];
			if (entry.Label.empty())
			{
				continue;
			}

			const std::size_t targetParentIndex = static_cast<std::size_t>(entry.Depth);
			if (targetParentIndex >= stack.size())
			{
				// Malformed depth (would skip a level); attach under deepest known.
				LiveNode* parent = stack.back();
				LiveNode* node = parent->FindOrCreateChild(entry.Label);
				node->RecordSample(entry.DurationMicroseconds);
				stack.push_back(node);
				continue;
			}

			stack.resize(targetParentIndex + 1);
			LiveNode* parent = stack.back();
			LiveNode* node = parent->FindOrCreateChild(entry.Label);
			node->RecordSample(entry.DurationMicroseconds);
			stack.push_back(node);
		}
	}

	ProfilerSnapshot LiveProfiler::CaptureSnapshot() const
	{
		ProfilerSnapshot snapshot;
		if (m_state == nullptr)
		{
			return snapshot;
		}

		// CPU.
		std::vector<std::pair<std::uint32_t, CpuThreadProfile*>> profiles;
		{
			std::lock_guard<std::mutex> lock(m_state->CpuRegistryMutex);
			profiles.reserve(m_state->CpuThreadProfiles.size());
			for (auto& kv : m_state->CpuThreadProfiles)
			{
				profiles.emplace_back(kv.first, kv.second.get());
			}
		}

		snapshot.CpuThreads.reserve(profiles.size());
		for (auto& kv : profiles)
		{
			ProfilerThreadSnapshot threadSnapshot;
			threadSnapshot.ThreadId = kv.first;
			{
				std::lock_guard<std::mutex> lock(kv.second->Mutex);
				threadSnapshot.Roots.reserve(kv.second->Root->Children.size());
				for (const std::unique_ptr<LiveNode>& root : kv.second->Root->Children)
				{
					ProfilerSnapshotNode rootSnapshot;
					CopyToSnapshot(*root, rootSnapshot);
					threadSnapshot.Roots.push_back(std::move(rootSnapshot));
				}
			}
			snapshot.CpuThreads.push_back(std::move(threadSnapshot));
		}

		std::sort(
		    snapshot.CpuThreads.begin(),
		    snapshot.CpuThreads.end(),
		    [](const ProfilerThreadSnapshot& a, const ProfilerThreadSnapshot& b)
		    {
			    return a.ThreadId < b.ThreadId;
		    });

		// GPU.
		{
			std::lock_guard<std::mutex> lock(m_state->GpuMutex);
			snapshot.GpuRoots.reserve(m_state->GpuRoot->Children.size());
			for (const std::unique_ptr<LiveNode>& root : m_state->GpuRoot->Children)
			{
				ProfilerSnapshotNode rootSnapshot;
				CopyToSnapshot(*root, rootSnapshot);
				snapshot.GpuRoots.push_back(std::move(rootSnapshot));
			}
		}

		return snapshot;
	}

	void LiveProfiler::Reset() noexcept
	{
		if (m_state == nullptr)
		{
			return;
		}
		{
			std::lock_guard<std::mutex> lock(m_state->CpuRegistryMutex);
			m_state->CpuThreadProfiles.clear();
		}
		{
			std::lock_guard<std::mutex> lock(m_state->GpuMutex);
			m_state->GpuRoot = std::make_unique<LiveNode>();
		}
	}
}
