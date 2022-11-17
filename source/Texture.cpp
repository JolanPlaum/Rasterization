#include "Texture.h"
#include "Vector2.h"
#include <SDL_image.h>
#include <cassert>

namespace dae
{
	Texture::Texture(SDL_Surface* pSurface) :
		m_pSurface{ pSurface },
		m_pSurfacePixels{ (uint32_t*)pSurface->pixels }
	{
	}

	Texture::~Texture()
	{
		if (m_pSurface)
		{
			SDL_FreeSurface(m_pSurface);
			m_pSurface = nullptr;
		}
	}

	Texture* Texture::LoadFromFile(const std::string& path)
	{
		//Load SDL_Surface using IMG_LOAD
		SDL_Surface* pSurface = IMG_Load(path.c_str());
		assert(pSurface && "Image failed to load!");

		//Create & Return a new Texture Object (using SDL_Surface)
		return new Texture(pSurface);
	}

	ColorRGB Texture::Sample(const Vector2& uv) const
	{
		int width = m_pSurface->w;
		int height = m_pSurface->h;
		
		int px = int(uv.x * width);
		int py = int(uv.y * height);

		Uint8 r, g, b;
		SDL_GetRGB(m_pSurfacePixels[px + (py * width)], m_pSurface->format, &r, &g, &b);

		return { r / 255.f, g / 255.f, b / 255.f };
	}
}