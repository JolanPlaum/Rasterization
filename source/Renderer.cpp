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
	m_Camera.Initialize(m_Width / (float)m_Height, 60.f, { .0f,5.f,-30.f });

	//Initialize Texture
	m_pTexture = Texture::LoadFromFile("Resources/tuktuk.png");
	Utils::ParseOBJ("Resources/tuktuk.obj", m_Mesh.vertices, m_Mesh.indices);
}

Renderer::~Renderer()
{
	delete m_pTexture;
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

	//Define Mesh
	//std::vector<Mesh> meshes_world
	//{
	//	{
	//		std::vector<Vertex>{
	//			{ {	-3,	3,	-2 }, colors::White, { 0.f,	0.f } },
	//			{ {	0,	3,	-2 }, colors::White, { .5f,	0.f } },
	//			{ { 3,	3,	-2 }, colors::White, { 1.f,	0.f } },
	//			{ {	-3,	0,	-2 }, colors::White, { 0.f,	.5f } },
	//			{ { 0,	0,	-2 }, colors::White, { .5f,	.5f } },
	//			{ { 3,	0,	-2 }, colors::White, { 1.f,	.5f } },
	//			{ {	-3,	-3,	-2 }, colors::White, { 0.f,	1.f } },
	//			{ { 0,	-3,	-2 }, colors::White, { .5f,	1.f } },
	//			{ { 3,	-3,	-2 }, colors::White, { 1.f,	1.f } }
	//		},
	//		std::vector<uint32_t>{
	//			/*
	//			3, 0, 1,	1, 4, 3,	4, 1, 2,
	//			2, 5, 4,	6, 3, 4,	4, 7, 6,
	//			7, 4, 5,	5, 8, 7
	//			*/
	//			3, 0, 4, 1, 5, 2,
	//			2, 6,
	//			6, 3, 7, 4, 8, 5
	//		},
	//		PrimitiveTopology::TriangleStrip
	//	}
	//};

	VertexTransformationFunction({m_Mesh});

	//RENDER LOGIC
	RenderMeshes({m_Mesh});

	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}

void Renderer::RenderMeshes(const std::vector<Mesh>& meshes)
{
	for (const Mesh& m : meshes)
	{
		switch (m.primitiveTopology)
		{
		case PrimitiveTopology::TriangeList:
			for (int i{}; i < m.indices.size() - 2; i += 3)
			{
				RenderTriangle(
					m.vertices_out[m.indices[i]],
					m.vertices_out[m.indices[i + 1]],
					m.vertices_out[m.indices[i + 2]]
				);
			}
			break;

		case PrimitiveTopology::TriangleStrip:
			for (int i{}; i < m.indices.size() - 2; ++i)
			{
				if (i & 1)
				{
					RenderTriangle(
						m.vertices_out[m.indices[i]],
						m.vertices_out[m.indices[i + 2]],
						m.vertices_out[m.indices[i + 1]]
					);
				}
				else
				{
					RenderTriangle(
						m.vertices_out[m.indices[i]],
						m.vertices_out[m.indices[i + 1]],
						m.vertices_out[m.indices[i + 2]]
					);
				}
			}
			break;

		default:
			break;
		}
	}
}

void Renderer::RenderTriangle(const Vertex_Out& v0, const Vertex_Out& v1, const Vertex_Out& v2)
{
	//Frustum Culling
	if (v0.position.x < 0 || v1.position.x < 0 || v2.position.x < 0 ||
		v0.position.x > m_Width || v1.position.x > m_Width || v2.position.x > m_Width) return;
	if (v0.position.y < 0 || v1.position.y < 0 || v2.position.y < 0 ||
		v0.position.y > m_Height || v1.position.y > m_Height || v2.position.y > m_Height) return;

	Vector2 edge0 = v2.position.GetXY() - v1.position.GetXY();
	Vector2 edge1 = v0.position.GetXY() - v2.position.GetXY();
	Vector2 edge2 = v1.position.GetXY() - v0.position.GetXY();

	float area{ Vector2::Cross(edge0, edge1) };
	if (area < 0.001f) return;

	int left{ (int)std::min(v0.position.x, std::min(v1.position.x, v2.position.x)) };
	int top{  (int)std::min(v0.position.y, std::min(v1.position.y, v2.position.y)) };
	int right{  (int)ceilf(std::max(v0.position.x, std::max(v1.position.x, v2.position.x))) };
	int bottom{ (int)ceilf(std::max(v0.position.y, std::max(v1.position.y, v2.position.y))) };

	if (left < 0) left = 0;
	if (top < 0) top = 0;
	if (right >= m_Width) right = m_Width - 1;
	if (bottom >= m_Height) bottom = m_Height - 1;

	for (int px{ left }; px < right; ++px)
	{
		for (int py{ top }; py < bottom; ++py)
		{
			float w0, w1, w2;
			Vector2 pixel{ (float)px, (float)py };

			//Check if pixel is inside triangle
			Vector2 pixelToSide = pixel - v0.position.GetXY();
			if ((w2 = Vector2::Cross(edge2, pixelToSide) / area) < 0.f) continue;

			pixelToSide = pixel - v1.position.GetXY();
			if ((w0 = Vector2::Cross(edge0, pixelToSide) / area) < 0.f) continue;

			pixelToSide = pixel - v2.position.GetXY();
			if ((w1 = Vector2::Cross(edge1, pixelToSide) / area) < 0.f) continue;

			//Calculate depth buffer
			float depthBuffer = 1.f / (w0 / v0.position.z + w1 / v1.position.z + w2 / v2.position.z);

			if (depthBuffer < 0 || depthBuffer > 1) continue;

			//Depth Test
			if (depthBuffer < m_pDepthBufferPixels[px + (py * m_Width)])
			{
				//Depth Write
				m_pDepthBufferPixels[px + (py * m_Width)] = depthBuffer;

				//Depth correction
				w0 /= v0.position.w;
				w1 /= v1.position.w;
				w2 /= v2.position.w;

				//Calculate depth
				float depth = 1.f / (w0 + w1 + w2);

				//Update Color in Buffer
				ColorRGB finalColor{};

				if (m_pTexture != nullptr) finalColor = m_pTexture->Sample((w0 * v0.uv + w1 * v1.uv + w2 * v2.uv) * depth);
				else finalColor = (w0 * v0.color + w1 * v1.color + w2 * v2.color) * depth;

				finalColor.MaxToOne();

				m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
					static_cast<uint8_t>(finalColor.r * 255),
					static_cast<uint8_t>(finalColor.g * 255),
					static_cast<uint8_t>(finalColor.b * 255));
			}
		}
	}
}

void Renderer::VertexTransformationFunction(Mesh& mesh) const
{
	VertexTransformationFunction(mesh.vertices, mesh.vertices_out, mesh.worldMatrix);
}

void Renderer::VertexTransformationFunction(std::vector<Mesh>& meshes) const
{
	for (int i{}; i < meshes.size(); ++i)
	{
		VertexTransformationFunction(meshes[i].vertices, meshes[i].vertices_out, meshes[i].worldMatrix);
	}
}

void Renderer::VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex_Out>& vertices_out, const Matrix& worldMatrix) const
{
	Matrix worldViewProjectionMatrix{ worldMatrix * m_Camera.viewMatrix * m_Camera.projectionMatrix };
	vertices_out.clear();
	vertices_out.reserve(vertices_in.size());

	for (int i{}; i < vertices_in.size(); ++i)
	{
		//Create temporary variable
		Vertex_Out v{};

		//Position calculations
		v.position = worldViewProjectionMatrix.TransformPoint({ vertices_in[i].position, 1.f });

		v.position.x /= v.position.w;
		v.position.y /= v.position.w;
		v.position.z /= v.position.w;

		v.position.x = ((1.f + v.position.x) / 2.f) * m_Width;
		v.position.y = ((1.f - v.position.y) / 2.f) * m_Height;

		//Set other variables
		v.color = vertices_in[i].color;
		v.uv = vertices_in[i].uv;
		v.normal = vertices_in[i].normal;
		v.tangent = vertices_in[i].tangent;

		//Add the new temporary variable to the list
		vertices_out.push_back(v);
	}
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}
