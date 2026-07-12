#pragma once

// =============================================================================
// Common Samplers (Copy-Paste Reference)
// =============================================================================
// Point:
//   SamplerPointMipPointWrap       SamplerPointMipPointClamp
//   SamplerPointNoMipWrap          SamplerPointNoMipClamp
//
// Linear (Trilinear):
//   SamplerLinearMipLinearWrap     SamplerLinearMipLinearClamp
//   SamplerLinearNoMipWrap         SamplerLinearNoMipClamp
//
// Bilinear:
//   SamplerLinearMipPointWrap      SamplerLinearMipPointClamp
//
// Anisotropic:
//   SamplerAniso4xWrap             SamplerAniso4xClamp
//   SamplerAniso8xWrap             SamplerAniso8xClamp
//   SamplerAniso16xWrap            SamplerAniso16xClamp

// =============================================================================
// Sampler Declarations
// =============================================================================
// Shared sampler symbols are reflected during cook. Runtime pass code selects
// the shared sampler by typed options; shader authors do not assign registers.
//
// Naming: Sampler<MinMag><Mip><Address>
//   MinMag:  Point, Linear
//   Mip:     MipPoint, MipLinear, NoMip
//   Address: Wrap, Clamp, Mirror
//
// Anisotropic: SamplerAniso<Level>x<Address>
//   Level: 1, 2, 4, 8, 16

// =============================================================================
// Point MinMag (s0-s8)
// =============================================================================

// Point MinMag, Point Mip
SamplerState SamplerPointMipPointWrap;
SamplerState SamplerPointMipPointClamp;
SamplerState SamplerPointMipPointMirror;

// Point MinMag, Linear Mip
SamplerState SamplerPointMipLinearWrap;
SamplerState SamplerPointMipLinearClamp;
SamplerState SamplerPointMipLinearMirror;

// Point MinMag, No Mip
SamplerState SamplerPointNoMipWrap;
SamplerState SamplerPointNoMipClamp;
SamplerState SamplerPointNoMipMirror;

// =============================================================================
// Linear MinMag (s9-s17)
// =============================================================================

// Linear MinMag, Point Mip (Bilinear)
SamplerState SamplerLinearMipPointWrap;
SamplerState SamplerLinearMipPointClamp;
SamplerState SamplerLinearMipPointMirror;

// Linear MinMag, Linear Mip (Trilinear)
SamplerState SamplerLinearMipLinearWrap;
SamplerState SamplerLinearMipLinearClamp;
SamplerState SamplerLinearMipLinearMirror;

// Linear MinMag, No Mip
SamplerState SamplerLinearNoMipWrap;
SamplerState SamplerLinearNoMipClamp;
SamplerState SamplerLinearNoMipMirror;

// =============================================================================
// Anisotropic (s18-s32)
// =============================================================================

// Anisotropic 1x
SamplerState SamplerAniso1xWrap;
SamplerState SamplerAniso1xClamp;
SamplerState SamplerAniso1xMirror;

// Anisotropic 2x
SamplerState SamplerAniso2xWrap;
SamplerState SamplerAniso2xClamp;
SamplerState SamplerAniso2xMirror;

// Anisotropic 4x
SamplerState SamplerAniso4xWrap;
SamplerState SamplerAniso4xClamp;
SamplerState SamplerAniso4xMirror;

// Anisotropic 8x
SamplerState SamplerAniso8xWrap;
SamplerState SamplerAniso8xClamp;
SamplerState SamplerAniso8xMirror;

// Anisotropic 16x
SamplerState SamplerAniso16xWrap;
SamplerState SamplerAniso16xClamp;
SamplerState SamplerAniso16xMirror;

// =============================================================================
// Sampler Count
// =============================================================================
static const uint kSamplerCount = 33;
