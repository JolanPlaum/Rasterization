//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include <iostream>
#include "Renderer.h"
#include "Math.h"
#include "Matrix.h"
#include "Texture.h"
#include "Utils.h"

using namespace dae;

Renderer::Renderer(SDL_Window* pWindow) :
	m_pWindow(pWindow)
{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);
	m_AspectRatio = m_Width / (float)m_Height;

	//Create Buffers
	m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
	m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
	m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;
	m_ClearColor = SDL_MapRGB(m_pBackBuffer->format,
		static_cast<uint8_t>(100),
		static_cast<uint8_t>(100),
		static_cast<uint8_t>(100));

	m_pDepthBufferPixels = new float[m_Width * m_Height];

	//Initialize Camera
	m_Camera.Initialize(60.f, { .0f,.0f,-10.f });
}

Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);
}

void Renderer::Render()
{
	//@START
	//Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);
	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);
	SDL_FillRect(m_pBackBuffer, NULL, m_ClearColor);

	//Define Triangle - Vertices in WORLD space
	std::vector<Vertex> vertices_world
	{
		//Triangle 0
		{{ 0.f, 2.f, 0.f }, { 1, 0, 0 }},
		{{ 1.5f, -1.f, 0.f }, { 1, 0, 0 }},
		{{ -1.5f, -1.f, 0.f }, { 1, 0, 0 }},

		//Triangle 1
		{{ 0.f, 4.f, 2.f }, { 1, 0, 0 }},
		{{ 3.f, -2.f, 2.f }, { 0, 1, 0 }},
		{{ -3.f, -2.f, 2.f }, { 0, 0, 1 }}
	};

	std::vector<Vertex> vertices_screen;
	VertexTransformationFunction(vertices_world, vertices_screen);

	//RENDER LOGIC
	size_t nrTriangles{ vertices_screen.size() / 3 };
	for (int index{}; index < nrTriangles; ++index)
	{
		Vertex v0{ vertices_screen[3 * index] };
		Vertex v1{ vertices_screen[3 * index + 1] };
		Vertex v2{ vertices_screen[3 * index + 2] };

		for (int px{}; px < m_Width; ++px)
		{
			for (int py{}; py < m_Height; ++py)
			{
				float w0, w1, w2;
				Vector2 pixel{ (float)px, (float)py };

				//Check if pixel is inside triangle
				Vector2 edge = v1.position.GetXY() - v0.position.GetXY();
				Vector2 pixelToSide = pixel - v0.position.GetXY();
				if ((w2 = Vector2::Cross(edge, pixelToSide)) < 0.f) continue;

				edge = v2.position.GetXY() - v1.position.GetXY();
				pixelToSide = pixel - v1.position.GetXY();
				if ((w0 = Vector2::Cross(edge, pixelToSide)) < 0.f) continue;

				edge = v0.position.GetXY() - v2.position.GetXY();
				pixelToSide = pixel - v2.position.GetXY();
				if ((w1 = Vector2::Cross(edge, pixelToSide)) < 0.f) continue;

				//float total = w0 + w1 + w2;
				//w0 /= total;
				//w1 /= total;
				//w2 /= total;

				float depth = v0.position.z * w0 + v1.position.z * w1 + v2.position.z * w2;
				//depth /= total;
				//TrianglePixelHitTest(triangle, { (float)px, (float)py }, finalColor, depth);

				//Depth Test
				if (depth < m_pDepthBufferPixels[px + (py * m_Width)])
				{
					//Depth Write
					m_pDepthBufferPixels[px + (py * m_Width)] = depth;

					//Update Color in Buffer
					ColorRGB finalColor{ v0.color * w0 + v1.color * w1 + v2.color * w2 };
					finalColor /= w0 + w1 + w2;
					finalColor.MaxToOne();

					m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
						static_cast<uint8_t>(finalColor.r * 255),
						static_cast<uint8_t>(finalColor.g * 255),
						static_cast<uint8_t>(finalColor.b * 255));
				}
			}
		}
	}

	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}

void Renderer::VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const
{
	//Todo > W1 Projection Stage
	vertices_out.reserve(vertices_in.size());

	for (int i{}; i < vertices_in.size(); ++i)
	{
		vertices_out.push_back({ vertices_in[i] });
		vertices_out[i].position = m_Camera.viewMatrix.TransformPoint(vertices_in[i].position);

		vertices_out[i].position.x = vertices_out[i].position.x / (vertices_out[i].position.z * m_AspectRatio * m_Camera.fov);
		vertices_out[i].position.y = vertices_out[i].position.y / (vertices_out[i].position.z * m_Camera.fov);

		vertices_out[i].position.x = ((1.f + vertices_out[i].position.x) / 2.f) * m_Width;
		vertices_out[i].position.y = ((1.f - vertices_out[i].position.y) / 2.f) * m_Height;
	}
}

bool dae::Renderer::TrianglePixelHitTest(const std::vector<Vertex>& triangle, const Vector2& pixel, ColorRGB& color, float& depth)
{
	//Make sure the passed vector is a triangle
	if (triangle.size() != 3) return false;

	float w0, w1, w2;

	//Check if pixel is inside triangle
	Vector2 edge = triangle[1].position.GetXY() - triangle[0].position.GetXY();
	Vector2 pixelToSide = pixel - triangle[0].position.GetXY();
	if ((w2 = Vector2::Cross(edge, pixelToSide)) < 0.f) return false;

	edge = triangle[2].position.GetXY() - triangle[1].position.GetXY();
	pixelToSide = pixel - triangle[1].position.GetXY();
	if ((w0 = Vector2::Cross(edge, pixelToSide)) < 0.f) return false;

	edge = triangle[0].position.GetXY() - triangle[2].position.GetXY();
	pixelToSide = pixel - triangle[2].position.GetXY();
	if ((w1 = Vector2::Cross(edge, pixelToSide)) < 0.f) return false;

	float total = w0 + w1 + w2;
	w0 /= total;
	w1 /= total;
	w2 /= total;

	color = triangle[0].color * w0 + triangle[1].color * w1 + triangle[2].color * w2;
	depth = triangle[0].position.z * w0 + triangle[1].position.z * w1 + triangle[2].position.z * w2;

	return true;
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}
