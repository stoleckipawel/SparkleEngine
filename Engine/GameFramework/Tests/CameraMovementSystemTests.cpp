#include "World/Systems/CameraMovementSystem.h"

#include "World/ECS/Components/RenderingComponents.h"
#include "World/ECS/Components/TransformComponents.h"

#include <DirectXMath.h>

#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace CameraMovementSystemTests
{
	void Require(bool condition, std::string_view message)
	{
		if (!condition)
		{
			throw std::runtime_error(std::string(message));
		}
	}

	float ApplyVerticalLook(float lookDeltaY, bool invertY)
	{
		CameraInputIntent intent;
		intent.LookDeltaY = lookDeltaY;

		ECS::Camera camera;
		ECS::CameraMovement movement;
		movement.MouseSensitivity = 0.01f;
		movement.InvertY = invertY;
		ECS::LocalTransform transform;

		Require(
		    ECS::CameraMovementSystem::Apply(EntityId::Invalid(), EntityId::Invalid(), intent, 0.0f, camera, movement, transform),
		    "Camera look input was not applied.");

		const DirectX::XMVECTOR forward =
		    DirectX::XMVector3Rotate(DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), DirectX::XMLoadFloat4(&transform.Rotation));
		DirectX::XMFLOAT3 direction;
		DirectX::XMStoreFloat3(&direction, forward);
		return direction.y;
	}

	void DefaultVerticalLookFollowsMouseMotion()
	{
		Require(ApplyVerticalLook(-10.0f, false) > 0.0f, "Moving the mouse up did not make the camera look up.");
		Require(ApplyVerticalLook(10.0f, false) < 0.0f, "Moving the mouse down did not make the camera look down.");
	}

	void InvertedVerticalLookReversesMouseMotion()
	{
		Require(ApplyVerticalLook(-10.0f, true) < 0.0f, "Invert Y did not reverse upward mouse motion.");
		Require(ApplyVerticalLook(10.0f, true) > 0.0f, "Invert Y did not reverse downward mouse motion.");
	}

	using TestFunction = void (*)();

	int Run(std::string_view name, TestFunction test)
	{
		try
		{
			test();
			std::cout << "[PASS] " << name << '\n';
			return 0;
		}
		catch (const std::exception& error)
		{
			std::cerr << "[FAIL] " << name << ": " << error.what() << '\n';
			return 1;
		}
	}
}

int main()
{
	using namespace CameraMovementSystemTests;
	int failureCount = 0;
	failureCount += Run("default vertical look direction", DefaultVerticalLookFollowsMouseMotion);
	failureCount += Run("inverted vertical look direction", InvertedVerticalLookReversesMouseMotion);
	return failureCount == 0 ? 0 : 1;
}
