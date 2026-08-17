#include "RHI/Public/RayTracing/RhiRayTracingTransformPacking.h"

#include <DirectXMath.h>

#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace RayTracingTransformContractTests
{
	void Require(bool condition, std::string_view message)
	{
		if (!condition)
		{
			throw std::runtime_error(message.data());
		}
	}

	bool NearlyEqual(float left, float right) noexcept
	{
		return std::abs(left - right) <= 1.0e-5f;
	}

	void NativePackingPreservesCanonicalPointTransform()
	{
		const DirectX::XMMATRIX world = DirectX::XMMatrixScaling(2.0f, 3.0f, 4.0f)
		    * DirectX::XMMatrixRotationRollPitchYaw(0.2f, -0.4f, 0.1f) * DirectX::XMMatrixTranslation(11.0f, 13.0f, 17.0f);
		DirectX::XMFLOAT4X4 storedWorld{};
		DirectX::XMStoreFloat4x4(&storedWorld, world);

		const std::array<float, 12> packed = RhiRayTracingTransformPacking::PackCanonicalObjectToWorld(storedWorld);
		const DirectX::XMFLOAT3 point{1.5f, -2.0f, 0.25f};
		DirectX::XMFLOAT3 expected{};
		DirectX::XMStoreFloat3(&expected, DirectX::XMVector3TransformCoord(DirectX::XMLoadFloat3(&point), world));

		const DirectX::XMFLOAT3 actual{
		    packed[0] * point.x + packed[1] * point.y + packed[2] * point.z + packed[3],
		    packed[4] * point.x + packed[5] * point.y + packed[6] * point.z + packed[7],
		    packed[8] * point.x + packed[9] * point.y + packed[10] * point.z + packed[11]};

		Require(
		    NearlyEqual(actual.x, expected.x) && NearlyEqual(actual.y, expected.y) && NearlyEqual(actual.z, expected.z),
		    "Ray-tracing 3x4 packing changed the canonical object-to-world transform.");
		Require(
		    NearlyEqual(packed[3], storedWorld._41) && NearlyEqual(packed[7], storedWorld._42) && NearlyEqual(packed[11], storedWorld._43),
		    "Ray-tracing 3x4 packing did not preserve row-vector translation.");
	}
}

int main()
{
	try
	{
		RayTracingTransformContractTests::NativePackingPreservesCanonicalPointTransform();
		std::cout << "[PASS] ray-tracing instance transform packing\n";
		return 0;
	}
	catch (const std::exception& error)
	{
		std::cerr << "[FAIL] ray-tracing instance transform packing: " << error.what() << '\n';
		return 1;
	}
}
