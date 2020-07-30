#pragma once
#include <cstdint>

enum class TextureGLType
{
	VertexDataXYZ, TextureBufferX, TextureBufferXY, TextureBufferXYZ, TextureBufferXYZW
};

enum class TextureGLDataFormat 
{
	FLOAT, INTEGER
};

class TextureGL 
{
public:
	TextureGL(int width, int height, TextureGLType datatype, const void* data);
	TextureGL(TextureGLType datatype, int sizeofData, const void* data, TextureGLDataFormat dataFormat = TextureGLDataFormat::FLOAT);
	TextureGL(TextureGL&& other);
	int getWidth();
	int getHeight();
	void bind();
	~TextureGL();
	friend class ShaderProgram;

private:
	int textureElementType;
	int width;
	int height;
	uint32_t textureID;
	uint32_t textureBufferID;

	void VertexDataXYZToTexture(int width, int height, const void* data);
	void TextureBufferToTexture(int width, int height, const void* data);
};