#pragma once
#include <stb_image.h>
#include <string>

#include "Texture.h"

class TextureLoader
{
public:
	static Texture* texture_from_file(std::string const& path);
};

