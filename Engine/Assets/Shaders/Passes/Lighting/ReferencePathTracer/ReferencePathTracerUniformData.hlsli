#pragma once

static const uint ReferencePathTracerWorkFlag_Trace = 1u << 0u;
static const uint ReferencePathTracerWorkFlag_ClearDisplay = 1u << 1u;
static const uint ReferencePathTracerWorkFlag_CommitPrefix = 1u << 2u;

cbuffer ReferencePathTracerConstants
{
	uint SessionSeed;
	uint ReplicateId;
	uint SampleOrdinal;
	uint FinitePathDiagnosticSurfaceVertices;
	uint FirstRow;
	uint RowCount;
	uint PriorSampleCount;
	uint WorkFlags;
};
