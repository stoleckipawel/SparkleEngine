#pragma once

#include "ShaderParameterStructBuilder.h"

#include <string>
#include <vector>

template <typename TParameters> class TypedPassParameterInstance final : public TParameters
{
  public:
	using Parameters = TParameters;
	using Metadata = ShaderParameterStructMetadata<TParameters>;

	explicit TypedPassParameterInstance(const Metadata& metadata) : m_metadata(&metadata), m_parameterSet(metadata.GetLayout()) {}

	TParameters& GetFields() noexcept
	{
		m_isDirty = true;
		return static_cast<TParameters&>(*this);
	}

	const TParameters& GetFields() const noexcept { return static_cast<const TParameters&>(*this); }

	TParameters* operator->() noexcept
	{
		m_isDirty = true;
		return &static_cast<TParameters&>(*this);
	}

	const TParameters* operator->() const noexcept { return &static_cast<const TParameters&>(*this); }

	TParameters& operator*() noexcept
	{
		m_isDirty = true;
		return static_cast<TParameters&>(*this);
	}

	const TParameters& operator*() const noexcept { return static_cast<const TParameters&>(*this); }

	bool Sync() const { return SyncIfNeeded(); }

	bool HasAllRequiredBindings() const { return SyncIfNeeded(); }

	const std::vector<std::string>& GetMissingBindings() const
	{
		SyncIfNeeded();
		return m_missingBindings;
	}

	const PassParameterSet& GetPassParameterSet() const
	{
		SyncIfNeeded();
		return m_parameterSet;
	}

  private:
	bool SyncIfNeeded() const
	{
		if (!m_isDirty)
		{
			return m_missingBindings.empty() && m_parameterSet.HasAllRequiredBindings();
		}

		m_isDirty = false;
		return m_metadata->Commit(static_cast<const TParameters&>(*this), m_parameterSet, &m_missingBindings);
	}

	const Metadata* m_metadata = nullptr;
	mutable bool m_isDirty = true;
	mutable std::vector<std::string> m_missingBindings;
	mutable PassParameterSet m_parameterSet;
};