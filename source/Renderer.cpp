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
	m_Camera.Initialize(m_Width / (float)m_Height, 45.f, { 0.f,0.f,0.f });

	//Initialize Texture
	m_pTexDiffuse =  Texture::LoadFromFile("Resources/vehicle_diffuse.png");
	m_pTexNormal =	 Texture::LoadFromFile("Resources/vehicle_normal.png");
	m_pTexGloss =	 Texture::LoadFromFile("Resources/vehicle_gloss.png");
	m_pTexSpecular = Texture::LoadFromFile("Resources/vehicle_specular.png");
	Utils::ParseOBJ("Resources/vehicle.obj", m_Mesh.vertices, m_Mesh.indices);
	m_Mesh.primitiveTopology = PrimitiveTopology::TriangeList;
}

Renderer::~Renderer()
{
	delete m_pTexDiffuse;
	delete m_pTexNormal;
	delete m_pTexGloss;
	delete m_pTexSpecular;
	delete[] m_pDepthBufferPixels;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);

	m_Rotation += pTimer->GetElapsed();
	if (m_Rotation > PI * 2.f)
		m_Rotation -= PI * 2.f;
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

	//Rotate mesh
	m_Mesh.worldMatrix = Matrix::CreateRotationY(m_Rotation) * Matrix::CreateTranslation(0.f, 0.f, 50.f);

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

bool Renderer::FrustumCulling(const Vector4& v)
{
	if (v.x < -1.f || v.x > 1.f) return true;
	if (v.y < -1.f || v.y > 1.f) return true;
	if (v.z < 0.f || v.z > 1.f) return true;

	return false;
}

Vertex_Out Renderer::NDCToRaster(const Vertex_Out& v)
{
	Vertex_Out temp{ v };
	temp.position.x = ((1.f + v.position.x) / 2.f) * m_Width;
	temp.position.y = ((1.f - v.position.y) / 2.f) * m_Height;
	return temp;
}

bool Renderer::Remap(float& value, float min, float max)
{
	value = (value - min) / (max - min);

	if (value < 0.f || value > 1.f) return false;
	return true;
}

void Renderer::RenderTriangle(const Vertex_Out& _v0, const Vertex_Out& _v1, const Vertex_Out& _v2)
{
	if (FrustumCulling(_v0.position) || FrustumCulling(_v1.position) || FrustumCulling(_v2.position)) return;

	Vertex_Out v0{ NDCToRaster(_v0) };
	Vertex_Out v1{ NDCToRaster(_v1) };
	Vertex_Out v2{ NDCToRaster(_v2) };

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
				Vertex_Out temp{};
				temp.position.x = (float)px;
				temp.position.y = (float)py;
				temp.color = (w0 * v0.color + w1 * v1.color + w2 * v2.color) * depth;
				temp.uv = (w0 * v0.uv + w1 * v1.uv + w2 * v2.uv) * depth;
				temp.normal = ((w0 * v0.normal + w1 * v1.normal + w2 * v2.normal) * depth).Normalized();
				temp.tangent = ((w0 * v0.tangent + w1 * v1.tangent + w2 * v2.tangent) * depth).Normalized();

				PixelShading(temp);
			}
		}
	}
}

void Renderer::PixelShading(const Vertex_Out& v)
{
	Vector3 lightDirection{ .577f, -.577f, .577f };
	float lightIntensity{ 7.f };
	float shininess{ 25.f };
	ColorRGB finalColor{ 0.025f, 0.025f, 0.025f };

	//calculate view direction
	const int px = (int)v.position.x;
	const int py = (int)v.position.y;

	float rx = px + 0.5f;
	float ry = py + 0.5f;

	float cx = (2 * (rx / float(m_Width)) - 1) * m_AspectRatio * m_Camera.fov;
	float cy = (1 - (2 * (ry / float(m_Height)))) * m_Camera.fov;

	Vector3 viewDirection = (cx * m_Camera.right + cy * m_Camera.up + m_Camera.forward).Normalized();

	//Normal map
	Vector3 binormal = Vector3::Cross(v.normal, v.tangent);
	Matrix tangentSpaceAxis{ v.tangent, binormal, v.normal, Vector3::Zero };

	ColorRGB sampledColor = m_pTexNormal->Sample(v.uv);
	sampledColor = (2.f * sampledColor) - ColorRGB{ 1.f, 1.f, 1.f };

	Vector3 sampledNormal = tangentSpaceAxis.TransformVector(sampledColor.r, sampledColor.g, sampledColor.b);

	//Observed area (lambert cosine law)
	float dotProduct = sampledNormal * -lightDirection;
	if (dotProduct >= 0.f)
	{
		finalColor += Lambert(lightIntensity, m_pTexDiffuse->Sample(v.uv)) * dotProduct;
		finalColor += Phong(m_pTexSpecular->Sample(v.uv), shininess * m_pTexGloss->Sample(v.uv).r, -lightDirection, viewDirection, sampledNormal) * dotProduct;
	}



	//Update Color in Buffer
	finalColor.MaxToOne();

	m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
		static_cast<uint8_t>(finalColor.r * 255),
		static_cast<uint8_t>(finalColor.g * 255),
		static_cast<uint8_t>(finalColor.b * 255));
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

		//Set other variables
		v.color = vertices_in[i].color;
		v.uv = vertices_in[i].uv;
		v.normal = worldMatrix.TransformVector(vertices_in[i].normal);
		v.tangent = worldMatrix.TransformVector(vertices_in[i].tangent);

		//Add the new temporary variable to the list
		vertices_out.emplace_back(v);
	}
}

void Renderer::ToggleFinalColor()
{
	m_ShowFinalColor = !m_ShowFinalColor;
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}
