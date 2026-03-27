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
// Convenience Macros
// =============================================================================
// Usage:
//   SAMPLER(Point, MipLinear, Wrap)    -> SamplerPointMipLinearWrap
//   SAMPLER(Linear, NoMip, Clamp)      -> SamplerLinearNoMipClamp
//   SAMPLER_ANISO(4, Mirror)           -> SamplerAniso4xMirror

#define SAMPLER(MinMag, Mip, Address) Sampler##MinMag##Mip##Address
#define SAMPLER_ANISO(Level, Address) SamplerAniso##Level##x##Address

// =============================================================================
// Sampler Declarations
// =============================================================================
// Contiguous descriptor table bound at runtime.
// Register slots must match D3D12SamplerLibrary::Slot.
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
SamplerState SamplerPointMipPointWrap : register(s0);
SamplerState SamplerPointMipPointClamp : register(s1);
SamplerState SamplerPointMipPointMirror : register(s2);

// Point MinMag, Linear Mip
SamplerState SamplerPointMipLinearWrap : register(s3);
SamplerState SamplerPointMipLinearClamp : register(s4);
SamplerState SamplerPointMipLinearMirror : register(s5);

// Point MinMag, No Mip
SamplerState SamplerPointNoMipWrap : register(s6);
SamplerState SamplerPointNoMipClamp : register(s7);
SamplerState SamplerPointNoMipMirror : register(s8);

// =============================================================================
// Linear MinMag (s9-s17)
// =============================================================================

// Linear MinMag, Point Mip (Bilinear)
SamplerState SamplerLinearMipPointWrap : register(s9);
SamplerState SamplerLinearMipPointClamp : register(s10);
SamplerState SamplerLinearMipPointMirror : register(s11);

// Linear MinMag, Linear Mip (Trilinear)
SamplerState SamplerLinearMipLinearWrap : register(s12);
SamplerState SamplerLinearMipLinearClamp : register(s13);
SamplerState SamplerLinearMipLinearMirror : register(s14);

// Linear MinMag, No Mip
SamplerState SamplerLinearNoMipWrap : register(s15);
SamplerState SamplerLinearNoMipClamp : register(s16);
SamplerState SamplerLinearNoMipMirror : register(s17);

// =============================================================================
// Anisotropic (s18-s32)
// =============================================================================

// Anisotropic 1x
SamplerState SamplerAniso1xWrap : register(s18);
SamplerState SamplerAniso1xClamp : register(s19);
SamplerState SamplerAniso1xMirror : register(s20);

// Anisotropic 2x
SamplerState SamplerAniso2xWrap : register(s21);
SamplerState SamplerAniso2xClamp : register(s22);
SamplerState SamplerAniso2xMirror : register(s23);

// Anisotropic 4x
SamplerState SamplerAniso4xWrap : register(s24);
SamplerState SamplerAniso4xClamp : register(s25);
SamplerState SamplerAniso4xMirror : register(s26);

// Anisotropic 8x
SamplerState SamplerAniso8xWrap : register(s27);
SamplerState SamplerAniso8xClamp : register(s28);
SamplerState SamplerAniso8xMirror : register(s29);

// Anisotropic 16x
SamplerState SamplerAniso16xWrap : register(s30);
SamplerState SamplerAniso16xClamp : register(s31);
SamplerState SamplerAniso16xMirror : register(s32);

// =============================================================================
// Sampler Count
// =============================================================================
static const uint kSamplerCount = 33;
