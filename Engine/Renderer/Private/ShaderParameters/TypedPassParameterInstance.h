#pragma once

#include "ShaderParameterStructBuilder.h"

#include <string>
#include <vector>

template <typename TParameters> class TypedPassParameterInstance final
{
public:
	using Parameters = TParameters;
	using Metadata = ShaderParameterStructMetadata<TParameters>;

	explicit TypedPassParameterInstance(const Metadata& metadata) :
	    m_metadata(&metadata),
	    m_parameterSet(metadata.GetLayout(), metadata.GetGraphResourceParameters())
	{
	}

	TypedPassParameterInstance(const TypedPassParameterInstance&) = delete;
	TypedPassParameterInstance& operator=(const TypedPassParameterInstance&) = delete;
	TypedPassParameterInstance(TypedPassParameterInstance&&) = delete;
	TypedPassParameterInstance& operator=(TypedPassParameterInstance&&) = delete;

	TParameters& GetFields() noexcept
	{
		m_isDirty = true;
		return m_fields;
	}

	const TParameters& GetFields() const noexcept { return m_fields; }

	TParameters* operator->() noexcept
	{
		m_isDirty = true;
		return &m_fields;
	}

	const TParameters* operator->() const noexcept { return &m_fields; }

	TParameters& operator*() noexcept
	{
		m_isDirty = true;
		return m_fields;
	}

	const TParameters& operator*() const noexcept { return m_fields; }

	bool Sync() const
	{
		if (!m_isDirty)
		{
			return m_missingBindings.empty() && m_parameterSet.HasAllRequiredBindings();
		}

		m_isDirty = false;
		return m_metadata->Commit(m_fields, m_parameterSet, m_bindingStorage, &m_missingBindings);
	}

	const std::vector<std::string>& GetMissingBindings() const
	{
		Sync();
		return m_missingBindings;
	}

	const PassParameterSet& GetPassParameterSet() const
	{
		Sync();
		return m_parameterSet;
	}

private:
	const Metadata* m_metadata = nullptr;
	TParameters m_fields = {};
	mutable bool m_isDirty = true;
	mutable std::vector<std::string> m_missingBindings;
	mutable PassParameterSet m_parameterSet;
	mutable std::vector<std::vector<std::byte>> m_bindingStorage;
};
