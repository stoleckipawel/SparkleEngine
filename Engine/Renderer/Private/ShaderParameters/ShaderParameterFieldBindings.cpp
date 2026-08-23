#include "PCH.h"

#include "Renderer/Public/ShaderParameters/ShaderParameterFields.h"

ShaderTexture2DSRV::ShaderTexture2DSRV() = default;
ShaderAccelerationStructure::ShaderAccelerationStructure() = default;
ShaderSamplerSet::ShaderSamplerSet() = default;

bool BindParameterField(PassParameterSet& parameterSet, const char* name, const ShaderTexture2DSRV& field)
{
	return parameterSet.SetShaderResourceView(name, field.GetDescriptorTable());
}

bool BindParameterField(PassParameterSet& parameterSet, const char* name, const ShaderAccelerationStructure& field)
{
	return field.IsBound() && parameterSet.SetAccelerationStructure(name, field.GetHandle());
}

bool BindParameterField(PassParameterSet& parameterSet, const char* name, const ShaderRenderTarget& field)
{
	return field.IsBound() && parameterSet.SetAttachment(name, field.GetBinding());
}

bool BindParameterField(PassParameterSet& parameterSet, const char* name, const ShaderDepthTarget& field)
{
	return field.IsBound() && parameterSet.SetAttachment(name, field.GetBinding());
}

bool BindParameterField(PassParameterSet& parameterSet, const char* name, const ShaderSamplerSet& field)
{
	if (!field.IsBound())
	{
		return false;
	}

	return parameterSet.SetSampler(name, field.GetSampler());
}
