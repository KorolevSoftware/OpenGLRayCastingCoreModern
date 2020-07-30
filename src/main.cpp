#include <assert.h>
#include <gtc/matrix_transform.hpp>
#include <fwd.hpp> //GLM
#include <iostream>
#include <map>
#include "Utils.h"
#include "ModelLoader.h"
#include "glad.h" // Opengl function loader
#include "TextureGL.h"
#include "ShaderProgram.h"
#include "SDLHelper.h"
#include "BVHBuilderSAH.h"
#include "nuklear_sdl_gl3.h"
#include <tuple>


using std::vector;
using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::mat4;
using glm::mat3;


constexpr int WinWidth = 1920;
constexpr int WinHeight = 1080;


std::map<int, bool> buttinInputKeys; //keyboard key
float yaw = 0.0f;          // for cam rotate
float pitch = 0.0f;        // for cam rotate


TextureGL loadGeometry(BVHBuilderSAH& bvh, std::string const& path)
{
	vector<float> vertex;
	vector<float> normal;
	vector<float> uv;

	ModelLoader::Obj(path, vertex, normal, uv);
	vector<float> triangleIntersect = ModelLoader::rayTracingTrianglePreComplite(vertex);

	bvh.build(vertex);
	return TextureGL(TextureGLType::TextureBufferXYZW, triangleIntersect.size() * sizeof(float), triangleIntersect.data());
	
}


// FPS Camera rotate
void updateMatrix(glm::mat3& viewToWorld)
{
	mat4 matPitch = mat4(1.0);
	mat4 matYaw = mat4(1.0);

	matPitch = glm::rotate(matPitch, glm::radians(pitch), vec3(1, 0, 0));
	matYaw = glm::rotate(matYaw, glm::radians(yaw), vec3(0, 1, 0));

	mat4 rotateMatrix = matYaw * matPitch;
	viewToWorld = mat3(rotateMatrix);
}


// FPS Camera move
void cameraMove(vec3& location, glm::mat3 const& viewToWorld)
{
	float speed = 1;
	if (buttinInputKeys[SDLK_w])
		location += viewToWorld * vec3(0, 0, 1) * speed;

	if (buttinInputKeys[SDLK_s])
		location += viewToWorld * vec3(0, 0, -1) * speed;

	if (buttinInputKeys[SDLK_a])
		location += viewToWorld * vec3(-1, 0, 0) * speed;

	if (buttinInputKeys[SDLK_d])
		location += viewToWorld * vec3(1, 0, 0) * speed;

	if (buttinInputKeys[SDLK_q])
		location += viewToWorld * vec3(0, 1, 0) * speed;

	if (buttinInputKeys[SDLK_e])
		location += viewToWorld * vec3(0, -1, 0) * speed;
}


int main(int ArgCount, char** Args)
{
	nk_context* ctx;
	// Set Opengl Specification
	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	//SDL_SetRelativeMouseMode(SDL_TRUE);

	// SDL window and Opengl
	SDLWindowPtr window(SDL_CreateWindow("OpenGL Ray Casting Core", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WinWidth, WinHeight, SDL_WINDOW_OPENGL));
	if (!window) {
		std::cerr << "Failed create window" << std::endl;
		return -1;
	}

	SDLGLContextPtr context(SDL_GL_CreateContext(window));
	if (!context) {
		std::cerr << "Failed to create OpenGL context" << std::endl;
		return -1;
	}

	if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
		std::cerr << "Failed to initialize OpenGL context" << std::endl;
		return -1;
	}

	ctx = nk_sdl_init(window.get());
	{
		nk_font_atlas* atlas;
		nk_sdl_font_stash_begin(&atlas);
		nk_font* roboto = nk_font_atlas_add_from_file(atlas, "e:/Project/Develop/Nuklear/extra_font/Roboto-Regular.ttf", 16, 0);
		nk_sdl_font_stash_end();
		nk_style_set_font(ctx, &roboto->handle);
	}

	std::cout << "OpenGL version " << glGetString(GL_VERSION) << std::endl;
	std::cout << "GLSL version " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

	// GL 3.3, you must have though 1 VAO for correct work vertex shader
	uint32_t VAO;
	glGenVertexArrays(1, &VAO);

	// Load geometry, build BVH && and load data to texture
	ShaderProgram shaderProgram("shaders/vertex.vert", "shaders/raytracing.frag");

	BVHBuilderSAH* bvh = new BVHBuilderSAH(); // Big object
	TextureGL texPreInData = loadGeometry(*bvh, "models/BoxT.obj");
	TextureGL texNode = bvh->bvhToTexture();
	TextureGL texIndex = bvh->indexToTexture();

	// Variable for camera  
	vec3 location = vec3(0, 0.1, -20);
	mat3 viewToWorld = mat3(1.0f);

	// Add value  in map
	buttinInputKeys[SDLK_w] = false;
	buttinInputKeys[SDLK_s] = false;
	buttinInputKeys[SDLK_a] = false;
	buttinInputKeys[SDLK_d] = false;
	buttinInputKeys[SDLK_q] = false;
	buttinInputKeys[SDLK_e] = false;

	// Event loop
	SDL_Event Event;
	auto keyIsInside = [&Event] {return buttinInputKeys.count(Event.key.keysym.sym); }; // check key inside in buttinInputKeys

	while (true)
	{
		nk_input_begin(ctx);
		while (SDL_PollEvent(&Event))
		{
			nk_sdl_handle_event(&Event);
			if (Event.type == SDL_QUIT)
				return 0;

			if (Event.type == SDL_MOUSEMOTION) {
				pitch += Event.motion.yrel * 0.03;
				yaw += Event.motion.xrel * 0.03;
			}

			if (Event.type == SDL_KEYDOWN && keyIsInside())
				buttinInputKeys[Event.key.keysym.sym] = true;

			if (Event.type == SDL_KEYUP && keyIsInside())
				buttinInputKeys[Event.key.keysym.sym] = false;

			if (Event.type == SDL_KEYDOWN && Event.key.keysym.sym == SDLK_ESCAPE)
				return 0;
		}
		nk_input_end(ctx);
		cameraMove(location, viewToWorld);
		updateMatrix(viewToWorld);

		// Render/Draw
		// Clear the colorbuffer
		glViewport(900, 100, WinWidth - 800, WinHeight - 200);
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		shaderProgram.bind();
		glBindVertexArray(VAO);
		shaderProgram.setTextureAI("texNode", texNode);
		shaderProgram.setTextureAI("texIndex", texIndex);
		shaderProgram.setTextureAI("texPreInData", texPreInData);
		shaderProgram.setMatrix3x3("viewToWorld", viewToWorld);
		shaderProgram.setFloat("aspectRation", (WinWidth - 800.0f)/ (WinHeight - 200.0f));
		shaderProgram.setVec3("location", location);
		// Draw
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);



		if (nk_begin(ctx, "Привет", nk_rect(20, 20, 230, 250), 0))
		{
			enum { EASY, HARD };
			static int op = EASY;
			static int property = 20;
			nk_layout_row_static(ctx, 30, 80, 1);
			 
			if (nk_button_label(ctx, u8"Привет"))
				printf("button pressed!\n");

			if (nk_button_label(ctx, u8"Приsвет"))
				printf("button pressed!\n");
			nk_layout_row_dynamic(ctx, 30, 2);
			if (nk_option_label(ctx, "easy", op == EASY)) op = EASY;
			if (nk_option_label(ctx, "hard", op == HARD)) op = HARD;
			nk_layout_row_dynamic(ctx, 22, 1);
			nk_property_int(ctx, "Compression:", 0, &property, 100, 10, 1);

			nk_layout_row_dynamic(ctx, 20, 1);
			nk_label(ctx, "background:", NK_TEXT_LEFT);
			nk_layout_row_dynamic(ctx, 25, 1);
			
		}
		nk_end(ctx);

		#define MAX_VERTEX_MEMORY 512 * 1024
		#define MAX_ELEMENT_MEMORY 128 * 1024
		nk_sdl_render(NK_ANTI_ALIASING_ON, MAX_VERTEX_MEMORY, MAX_ELEMENT_MEMORY);
		SDL_GL_SwapWindow(window);
	}
}