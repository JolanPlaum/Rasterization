#pragma once
#include <cassert>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>

#include "Math.h"
#include "Timer.h"

namespace dae
{
	struct Camera
	{
		Camera() = default;

		Camera(const Vector3& _origin, float _fovAngle):
			origin{_origin},
			fovAngle{_fovAngle}
		{
		}


		Vector3 origin{};
		float fovAngle{90.f};
		float fov{ tanf((fovAngle * TO_RADIANS) / 2.f) };

		Vector3 forward{Vector3::UnitZ};
		Vector3 up{Vector3::UnitY};
		Vector3 right{Vector3::UnitX};

		float totalPitch{};
		float totalYaw{};

		Matrix invViewMatrix{};
		Matrix viewMatrix{};

		const float movementSpeed{ 10.f };
		const float rotationSpeed{ 1.f };


		void Initialize(float _fovAngle = 90.f, Vector3 _origin = {0.f,0.f,0.f})
		{
			fovAngle = _fovAngle;
			fov = tanf((fovAngle * TO_RADIANS) / 2.f);

			origin = _origin;
		}

		void CalculateViewMatrix()
		{
			forward.Normalize();
			right = Vector3::Cross(Vector3::UnitY, forward).Normalized();
			up = Vector3::Cross(forward, right).Normalized();

			invViewMatrix = Matrix{
				right,
				up,
				forward,
				origin
			};

			viewMatrix = invViewMatrix.Inverse();
		}

		void CalculateProjectionMatrix()
		{
			//TODO W2

			//ProjectionMatrix => Matrix::CreatePerspectiveFovLH(...) [not implemented yet]
			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixperspectivefovlh
		}

		void Update(Timer* pTimer)
		{
			const float deltaTime = pTimer->GetElapsed();

			//Keyboard Input
			const uint8_t* pKeyboardState = SDL_GetKeyboardState(nullptr);

			//Mouse Input
			int mouseX{}, mouseY{};
			const uint32_t mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);

			//Check if any valid input is received
			if (mouseState & (SDL_BUTTON_LMASK ^ SDL_BUTTON_RMASK)
				|| pKeyboardState[SDL_SCANCODE_W]
				|| pKeyboardState[SDL_SCANCODE_S]
				|| pKeyboardState[SDL_SCANCODE_D]
				|| pKeyboardState[SDL_SCANCODE_A])
			{
				//Camera Movement/Rotation
				float moveSpeed{ movementSpeed * deltaTime * (pKeyboardState[SDL_SCANCODE_LSHIFT] * 3 + 1) };
				float rotSpeed{ rotationSpeed * deltaTime };

				origin += pKeyboardState[SDL_SCANCODE_W] * forward * moveSpeed
					- pKeyboardState[SDL_SCANCODE_S] * forward * moveSpeed
					+ pKeyboardState[SDL_SCANCODE_D] * right * moveSpeed
					- pKeyboardState[SDL_SCANCODE_A] * right * moveSpeed;

				bool lmb = mouseState == SDL_BUTTON_LMASK;
				bool rmb = mouseState == SDL_BUTTON_RMASK;
				bool lrmb = mouseState == (SDL_BUTTON_LMASK ^ SDL_BUTTON_RMASK);
				origin -= lmb * forward * moveSpeed * (float)mouseY;
				origin -= lrmb * up * moveSpeed * (float)mouseY;

				totalPitch -= rmb * rotSpeed * (float)mouseY;
				totalYaw += lmb * rotSpeed * (float)mouseX;
				totalYaw += rmb * rotSpeed * (float)mouseX;

				Matrix finalRotation{ Matrix::CreateRotationX(totalPitch) * Matrix::CreateRotationY(totalYaw) };
				forward = finalRotation.TransformVector(Vector3::UnitZ);

				//Update Matrices
				CalculateViewMatrix();
				CalculateProjectionMatrix(); //Try to optimize this - should only be called once or when fov/aspectRatio changes
			}
		}
	};
}
