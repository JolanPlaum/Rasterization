#pragma once

#include <cstdint>
#include <vector>

#include "Camera.h"
#include "DataTypes.h"

struct SDL_Window;
struct SDL_Surface;

namespace dae
{
	class Texture;
	class Timer;
	class Scene;

	class Renderer final
	{
	public:
		Renderer(SDL_Window* pWindow);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void Update(Timer* pTimer);
		void Render();

		void ToggleFinalColor();
		void ToggleRotation();
		void ToggleNormalMap();
		void CycleLightingMode();
		bool SaveBufferToImage() const;

	private:
		SDL_Window* m_pWindow{};
		Texture* m_pTexDiffuse{ nullptr };
		Texture* m_pTexNormal{ nullptr };
		Texture* m_pTexGloss{ nullptr };
		Texture* m_pTexSpecular{ nullptr };
		Mesh m_Mesh{};
		float m_Rotation{};

		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		uint32_t* m_pBackBufferPixels{};
		Uint32 m_ClearColor{};
		bool m_ShowFinalColor{ true };

		float* m_pDepthBufferPixels{};

		Camera m_Camera{};

		int m_Width{};
		int m_Height{};
		float m_AspectRatio{};

		enum class LightingMode
		{
			ObservedArea, //Lambert Cosine Law
			Diffuse, //Including observed area
			Specular, //Including observed area
			Combined, //ObservedArea * (Diffuse + Specular)

			End
		};

		LightingMode m_LightingMode{ LightingMode::Combined };
		bool m_IsRotating{ true };
		bool m_IsNormalMap{ true };

		//Render helper functions
		void RenderMeshes(const std::vector<Mesh>& meshes);
		bool FrustumCulling(const Vector4& v);
		Vertex_Out NDCToRaster(const Vertex_Out& v);
		bool Remap(float& value, float min, float max);

		//Renders a single triangle
		void RenderTriangle(const Vertex_Out& v0, const Vertex_Out& v1, const Vertex_Out& v2);

		//Shades a single pixel
		ColorRGB PixelShading(const Vertex_Out& v);

		//Function that transforms the vertices from the mesh from World space to Screen space
		void VertexTransformationFunction(Mesh& mesh) const;
		void VertexTransformationFunction(std::vector<Mesh>& meshes) const;
		void VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex_Out>& vertices_out, const Matrix& worldMatrix) const;
	};

	//TODO: add seperate files for material/BRDF functions
	static ColorRGB Lambert(float kd, const ColorRGB& cd)
	{
		return (cd * kd) / PI;
	}
	static ColorRGB Phong(ColorRGB ks, float exp, const Vector3& l, const Vector3& v, const Vector3& n)
	{
		Vector3 r = l - (n * (2.f * (n * l)));
		float dot = r * v;
		if (dot < 0.f) return {};
		return ks * powf(dot, exp);
	}
}
