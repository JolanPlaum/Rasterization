//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
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

	//m_pDepthBufferPixels = new float[m_Width * m_Height];

	//Initialize Camera
	m_Camera.Initialize(60.f, { .0f,.0f,-10.f });
}

Renderer::~Renderer()
{
	//delete[] m_pDepthBufferPixels;
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

	//Define Triangle - Vertices in WORLD space
	std::vector<Vertex> vertices_world
	{
		{{ 0.f, 2.f, 0.f }},
		{{ 1.f, 0.f, 0.f }},
		{{ -1.f, 0.f, 0.f }}
	};

	//Define Triangle - Vertices in WORLD space
	std::vector<Vertex> vertices_screen;
	VertexTransformationFunction(vertices_world, vertices_screen);

	//RENDER LOGIC
	for (int px{}; px < m_Width; ++px)
	{
		for (int py{}; py < m_Height; ++py)
		{
			ColorRGB finalColor{ colors::Black };

			if (TrianglePixelHitTest(vertices_screen, { (float)px, (float)py }))
				finalColor = colors::White;

			//Update Color in Buffer
			finalColor.MaxToOne();

			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
				static_cast<uint8_t>(finalColor.r * 255),
				static_cast<uint8_t>(finalColor.g * 255),
				static_cast<uint8_t>(finalColor.b * 255));
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
		vertices_out.push_back({});
		vertices_out[i].position = m_Camera.viewMatrix.TransformPoint(vertices_in[i].position);

		vertices_out[i].position.x = vertices_out[i].position.x / (vertices_out[i].position.z * m_AspectRatio * m_Camera.fov);
		vertices_out[i].position.y = vertices_out[i].position.y / (vertices_out[i].position.z * m_Camera.fov);

		vertices_out[i].position.x = ((1.f + vertices_out[i].position.x) / 2.f) * m_Width;
		vertices_out[i].position.y = ((1.f - vertices_out[i].position.y) / 2.f) * m_Height;
	}
}

bool dae::Renderer::TrianglePixelHitTest(const std::vector<Vertex>& triangle, const Vector2& pixel)
{
	//Make sure the passed vector is a triangle
	if (triangle.size() != 3) return false;

	//Check if pixel is inside triangle
	Vector2 edge = triangle[1].position.GetXY() - triangle[0].position.GetXY();
	Vector2 pixelToSide = pixel - triangle[0].position.GetXY();
	if (Vector2::Cross(edge, pixelToSide) < 0.f) return false;

	edge = triangle[2].position.GetXY() - triangle[1].position.GetXY();
	pixelToSide = pixel - triangle[1].position.GetXY();
	if (Vector2::Cross(edge, pixelToSide) < 0.f) return false;

	edge = triangle[0].position.GetXY() - triangle[2].position.GetXY();
	pixelToSide = pixel - triangle[2].position.GetXY();
	if (Vector2::Cross(edge, pixelToSide) < 0.f) return false;

	return true;
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}
