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

	//Define Triangle - Vertices in NDC space
	std::vector<Vector3> vertices_ndc
	{
		{ 0.f, 0.5f, 1.f},
		{ 0.5f, -0.5f, 1.f},
		{ -0.5f, -0.5f, 1.f}
	};

	for (Vector3& vertex : vertices_ndc)
	{
		vertex.x = ((1.f + vertex.x) / 2.f) * m_Width;
		vertex.y = ((1.f - vertex.y) / 2.f) * m_Height;
	}

	//RENDER LOGIC
	for (int px{}; px < m_Width; ++px)
	{
		for (int py{}; py < m_Height; ++py)
		{
			ColorRGB finalColor{ colors::Black };

			if (TrianglePixelHitTest(vertices_ndc, { (float)px, (float)py }))
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
}

bool dae::Renderer::TrianglePixelHitTest(const std::vector<Vector3>& triangle, const Vector2& pixel)
{
	//Make sure the passed vector is a triangle
	if (triangle.size() != 3) return false;

	//Check if pixel is inside triangle
	Vector2 edge = triangle[1].GetXY() - triangle[0].GetXY();
	Vector2 pixelToSide = pixel - triangle[0].GetXY();
	if (Vector2::Cross(edge, pixelToSide) < 0.f) return false;

	edge = triangle[2].GetXY() - triangle[1].GetXY();
	pixelToSide = pixel - triangle[1].GetXY();
	if (Vector2::Cross(edge, pixelToSide) < 0.f) return false;

	edge = triangle[0].GetXY() - triangle[2].GetXY();
	pixelToSide = pixel - triangle[2].GetXY();
	if (Vector2::Cross(edge, pixelToSide) < 0.f) return false;

	return true;
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}
