#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL_opengl.h>
#include <SDL_image.h>

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

#include "ShaderProgram.h"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#define STB_IMAGE_IMPLEMENTATION 
#include "stb_image.h"
#include <fstream>
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#define FIXED_TIMESTEP 0.0166666f
#define MAX_TIMESTEP 2
using namespace std;
SDL_Window* displayWindow;


/* function declaration */
GLuint LoadTexture(const char *filePath);
void Setup();
void ProcessEvents();
void Update(float elapsed);
void Render();
void Cleanup();
void DrawSpriteSheet(ShaderProgram &program, GLuint texture, int index, int spriteCountX, int spriteCountY);
void DrawText(ShaderProgram &program, int fontTexture, string text, float row, float col, float size, float spacing);
float lerp(float v0, float v1, float t);
//setup variables
SDL_Event event;
bool done = false;
ShaderProgram program;
glm::mat4 viewMatrix = glm::mat4(1.0f);
glm::mat4 projectionMatrix = glm::mat4(1.0f);


class Entity {
public:
	GLuint texture;
	glm::vec2 position; //vec2 for 2D
	glm::vec2 size;	//for 2D
	glm::vec2 velocity;
	glm::vec2 acceleration;
	Entity(float x, float y, float sizeX, float sizeY, float vX, float vY, float aX, float aY) {
		position.x = x;
		position.y = y;
		size.x = sizeX;
		size.y = sizeY;
		velocity.x = vX;
		velocity.y = vY;
		acceleration.x = aX;
		acceleration.y = aY;
	}
	void drawUniform(ShaderProgram &program, int index, int spriteCountX, int spriteCountY) {
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(position.x, position.y, 1.0f));
		//height and width are here
		modelMatrix = glm::scale(modelMatrix, glm::vec3(size.x, size.y, 1.0f));
		program.SetModelMatrix(modelMatrix);
		program.SetProjectionMatrix(projectionMatrix);
		program.SetViewMatrix(viewMatrix);
		DrawSpriteSheet(program, texture, index, spriteCountX, spriteCountY);
	}

};


/* global variables */
GLuint bettyTexture;
GLuint fontTex;

//betty move index
const int bettyDown[] = { 0, 4, 8, 12 };
const int bettyLeft[] = { 1, 5, 9, 13 };
const int bettyUp[] = { 2, 6, 10, 14 };
const int bettyRight[] = { 3, 7, 11, 15 };

// time and speed
float accumulator = 0.0f;
float lastFrameTicks = 0.0f;
float friction = 0.5f;
Entity betty(0.0f, 0.0f, 0.2f, 0.2f, 0.0f, 0.0f, 0.0f, 0.0f);

GLuint LoadTexture(const char *filePath) {
	int w, h, comp;
	unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);
	if (image == NULL) {
		cout << "Unable to load image. Make sure the path is correct\n";
		assert(false);
	}
	GLuint retTexture;
	glGenTextures(1, &retTexture);
	glBindTexture(GL_TEXTURE_2D, retTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	stbi_image_free(image);
	return retTexture;
}

/*-----------------Draw Uniform Sprite--------------------*/

void DrawSpriteSheet(ShaderProgram &program, GLuint texture, int index, int spriteCountX, int spriteCountY) {
	float u = (float)(((int)index) % spriteCountX) / (float)spriteCountX;
	float v = (float)(((int)index) / spriteCountY) / (float)spriteCountY;
	float spriteWidth = 1.0 / (float)spriteCountX;
	float spriteHeight = 1.0 / (float)spriteCountY;
	
	glBindTexture(GL_TEXTURE_2D, texture);
	float verCoords[] = { 
		-0.5f, -0.5f, 
		0.5f, 0.5f, 
		-0.5f, 0.5f, 
		0.5f, 0.5f,  
		-0.5f, -0.5f, 
		0.5f, -0.5f };
	glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, verCoords);
	glEnableVertexAttribArray(program.positionAttribute);

	float texCoords[] = { u, v + spriteHeight, u + spriteWidth, v, u, v, u + spriteWidth, v, u, v + spriteHeight, u + spriteWidth, v + spriteHeight };
	glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(program.texCoordAttribute);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisableVertexAttribArray(program.positionAttribute);
	glDisableVertexAttribArray(program.texCoordAttribute);
}

/*-----------------------------------Draw Text---------------------------------------*/
void DrawText(ShaderProgram &program, int fontTexture, string text, float row, float col, float size, float spacing) {
	float character_size = 1.0 / 16.0f;
	std::vector<float> vertexData;
	std::vector<float> texCoordData;
	glm::mat4 fontModel = glm::mat4(1.0f);
	for (size_t i = 0; i < text.size(); i++) {
		float texture_x = (float)(((int)text[i]) % 16) / 16.0f;
		float texture_y = (float)(((int)text[i]) / 16) / 16.0f;
		vertexData.insert(vertexData.end(), {
			((size + spacing) * i) + (-0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
			});
		texCoordData.insert(texCoordData.end(), {
			texture_x, texture_y,
			texture_x, texture_y + character_size,
			texture_x + character_size, texture_y,
			texture_x + character_size, texture_y + character_size,
			texture_x + character_size, texture_y,
			texture_x, texture_y + character_size,
			});
	}
	glUseProgram(program.programID);
	fontModel = glm::translate(fontModel, glm::vec3(row, col, 1.0f));
	fontModel = glm::scale(fontModel, glm::vec3(0.8f, 1.0f, 1.0f));
	program.SetModelMatrix(fontModel);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
	glEnableVertexAttribArray(program.positionAttribute);
	glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glEnableVertexAttribArray(program.texCoordAttribute);
	glBindTexture(GL_TEXTURE_2D, fontTexture);
	glDrawArrays(GL_TRIANGLES, 0, int(vertexData.size() / 2));

	glDisableVertexAttribArray(program.positionAttribute);
	glDisableVertexAttribArray(program.texCoordAttribute);
}

/*-------------------------------Render Main Menu------------------------------------*/
void RenderMainMenu() {
	string text = "Super Betty: Find Star";
	DrawText(program, fontTex, text, -1.3f, 0.6f, 0.15f, 0.003f);
	string text2 = "Press: SPACE to jump";
	string text3 = "LEFT/RIGHT to move";
	string text4 = "Click anywhere to start the game";
	DrawText(program, fontTex, text2, -1.0f, -0.3f, 0.1f, 0.003f);
	DrawText(program, fontTex, text3, -0.42f, -0.5f, 0.1f, 0.003f);
	DrawText(program, fontTex, text4, -1.02f, -0.8f, 0.08f, 0.003f);
}

/*-------------------------------Friction--------------------------------------------*/
float lerp(float v0, float v1, float t) {
	return (1.0 - t) * v0 + t * v1;
}


/*---------------------Functions To be Called in Main()---------------------------*/
void Setup() {
	//setup SDL, OpenGL, projection matrix
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 940, 530, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
	glewInit();
#endif
	//setup
	glViewport(0, 0, 940, 530);
	//for textured polygon
	program.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
	bettyTexture = LoadTexture(RESOURCE_FOLDER"betty.png");
	fontTex = LoadTexture(RESOURCE_FOLDER"font.png");
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	projectionMatrix = glm::ortho(-1.774f, 1.774f, -1.0f, 1.0f, -1.0f, 1.0f);
	glUseProgram(program.programID);
}

void ProcessEvents() {
	//check input events
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
			done = true;
		}
		
	}
}

void Update(float elapsed) {   
	//move stuff and check for collisions
	const Uint8* keys = SDL_GetKeyboardState(NULL);
	betty.acceleration.x = 0.0f;
	betty.acceleration.y = 0.0f;
	if (keys[SDL_SCANCODE_RIGHT]) {
		betty.acceleration.x = 2.0f;
	}
	else if (keys[SDL_SCANCODE_LEFT]) {
		betty.acceleration.x = -2.0f;
	}
	else if (keys[SDL_SCANCODE_UP]) {
		betty.acceleration.y = 2.0f;
	}
	else if (keys[SDL_SCANCODE_DOWN]) {
		betty.acceleration.y = -2.0f;
	}

	bool condLW = (betty.position.x - betty.size.x / 2 < -1.774f); //hit left wall
	bool condRW = (betty.position.x + betty.size.x / 2 > 1.774f); //hit right wall
	bool condDW = (betty.position.y - betty.size.y / 2 < -1.0f); //hit down wall
	bool condUW = (betty.position.y + betty.size.y / 2 > 1.0f); // hit upper wall
	if (condLW) {
		betty.velocity.x = 0;
		betty.position.x += abs((betty.position.x - betty.size.x / 2) -(-1.774f));
	}
	else if(condRW){
		betty.velocity.x = 0;
		betty.position.x -= ((betty.position.x + betty.size.x / 2) - 1.774f);
	}
	else if (condDW) {
		betty.velocity.y = 0;
		betty.position.y += abs((betty.position.y - betty.size.y / 2) - (-1.0f));
	}
	else if (condUW) {
		betty.velocity.y = 0;
		betty.position.y -= ((betty.position.y + betty.size.y / 2) - 1.0f);
	}
	else
	{
		betty.velocity.x = lerp(betty.velocity.x, 0.0f, elapsed*friction);
		betty.velocity.y = lerp(betty.velocity.y, 0.0f, elapsed*friction);
		betty.velocity.x += betty.acceleration.x * elapsed;
		betty.velocity.y += betty.acceleration.y * elapsed;
		betty.position.x += betty.velocity.x * elapsed;
		betty.position.y += betty.velocity.y * elapsed;
	}
	cout << betty.position.x << " " << betty.position.y << endl;
}


void Render() {
	//for all game elements
	glClearColor(0.0f, 0.1f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	//draw betty   
	betty.texture = bettyTexture;

	int index = 4;
	int bcountX = 4;
	int bcountY = 4;
	betty.drawUniform(program, index, bcountX, bcountY);
	RenderMainMenu();
	SDL_GL_SwapWindow(displayWindow);
}

void Cleanup() {
	SDL_Quit();
}

int main(int argc, char *argv[])
{
	Setup();
	while (!done) {
		//update time frame
		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;
		elapsed += accumulator;
		if (elapsed < FIXED_TIMESTEP) {
			accumulator = elapsed;
			continue;
		}
		while (elapsed >= FIXED_TIMESTEP) {
			Update(FIXED_TIMESTEP);
			elapsed -= FIXED_TIMESTEP;
		}
		accumulator = elapsed;
		ProcessEvents();
		Render();
	}
	Cleanup();
	return 0;
}