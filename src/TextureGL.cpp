#include "TextureGL.h"
#include "glad.h" // Opengl function loader
#include <iostream>

TextureGL::TextureGL(int width, int height, TextureGLType datatype, const void* data):width(width), height(height)
{
	glGenTextures(1, &textureID);
	if (datatype == TextureGLType::VertexDataXYZ)
		VertexDataXYZToTexture(width, height, data);
}

TextureGL::TextureGL(TextureGLType datatype, int sizeofData, const void* data, TextureGLDataFormat dataFormat) :
	width(width), height(height), textureElementType(textureElementType)
{
	glGenTextures(1, &textureID);
	glGenBuffers(1, &textureBufferID);
	glBindBuffer(GL_TEXTURE_BUFFER, textureBufferID);
	glBufferData(GL_TEXTURE_BUFFER, sizeofData, data, GL_STATIC_DRAW);
	glBindBuffer(GL_TEXTURE_BUFFER, 0);

	if (dataFormat == TextureGLDataFormat::FLOAT && datatype == TextureGLType::TextureBufferX)
		textureElementType = GL_R32F;

	if (dataFormat == TextureGLDataFormat::FLOAT && datatype == TextureGLType::TextureBufferXY)
		textureElementType = GL_RG32F;

	if (dataFormat == TextureGLDataFormat::FLOAT && datatype == TextureGLType::TextureBufferXYZ)
		textureElementType = GL_RGB32F;

	if (dataFormat == TextureGLDataFormat::FLOAT && datatype == TextureGLType::TextureBufferXYZW)
		textureElementType = GL_RGBA32F;

	if (dataFormat == TextureGLDataFormat::INTEGER && datatype == TextureGLType::TextureBufferX)
		textureElementType = GL_R32I;

	if (dataFormat == TextureGLDataFormat::INTEGER && datatype == TextureGLType::TextureBufferXY)
		textureElementType = GL_RG32I;

	if (dataFormat == TextureGLDataFormat::INTEGER && datatype == TextureGLType::TextureBufferXYZ)
		textureElementType = GL_RGB32I;

	if (dataFormat == TextureGLDataFormat::INTEGER && datatype == TextureGLType::TextureBufferXYZW)
		textureElementType = GL_RGBA32I;
}

TextureGL::TextureGL(TextureGL&& other): 
	textureID(std::exchange(other.textureID, -1)),
	textureBufferID(std::exchange(other.textureBufferID, -1)),
	width(std::exchange(other.width, -1)),
	height(std::exchange(other.height, -1)),
	textureElementType(std::exchange(other.textureElementType, -1))
{

}

int TextureGL::getWidth()
{
	return width;
}

int TextureGL::getHeight()
{
	return height;
}

void TextureGL::bind()
{
	glBindTexture(GL_TEXTURE_BUFFER, textureID);
	glTexBuffer(GL_TEXTURE_BUFFER, textureElementType, textureBufferID);
}

TextureGL::~TextureGL()
{
	glDeleteTextures(1, &textureID);
}

void TextureGL::VertexDataXYZToTexture(int width, int height, const void* data)
{
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	int error = glGetError();
	if (error)
		std::cerr << error << std::endl;

	glBindTexture(GL_TEXTURE_2D, 0);
}
