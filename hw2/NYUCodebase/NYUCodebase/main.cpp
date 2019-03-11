/*
Author: Ziwei (Ava) Zheng
Date: 03/10/2019
*/
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
using namespace std;
SDL_Window* displayWindow;

//declaration
void Setup();
void ProcessEvents();
void Cleanup();
void Render();
void Updates();

//Player Entity
class Player {
public:
	Player(float posX, float posY): x(posX), y(posY){}
	//original position of player when start the game
	float x;
	float y;

	//update with elapsed time in the loop
	float speed = 2.8f;

	//size of the player
	float width = 0.1f;
	float height = 0.3f;
};

//Ball Entity
class Ball {
public:
	//original position
	float x = 0.0f;
	float y = 0.0f;

	//update with elapsed time in the loop
	float speed = 1.0f;
	
	//size of the player
	float width = 0.1f;
	float height = 0.1f;

	//movement unit of each direction
	float xDir = 1.0f;
	float yDir = 1.0f;
};

//global variables
ShaderProgram program;
bool done = false;
glm::mat4 projectionMatrix = glm::mat4(1.0f);
glm::mat4 viewMatrix = glm::mat4(1.0f);
SDL_Event event;
float texturedVertices[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
float texturedVertices2[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
float texturedVertices3[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
float lastFrameTicks = 0.0f;
float elapsed = 0.0f;
char winner;
Player p1(1.7f, 0.0f);
Player p2(-1.7f, 0.0f);
Ball b;

void Setup() {
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("Pong Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
	glewInit();
#endif
	glViewport(0, 0, 640, 360);
	program.Load(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");
	projectionMatrix = glm::ortho(-1.777f, 1.777f, -1.0f, 1.0f, -1.0f, 1.0f);
	glUseProgram(program.programID);
}

void ProcessEvents() {
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
			done = true;
		}
	}
	float ticks = (float)SDL_GetTicks() / 1000.0f;
	elapsed = ticks - lastFrameTicks;
	lastFrameTicks = ticks;
	const Uint8* keys = SDL_GetKeyboardState(NULL);
	
	//Move the two players according to user's hit keys
	if (keys[SDL_SCANCODE_UP] && (p1.y+p1.height/2) <= 1.0f) {
		p1.y += p1.speed * elapsed;
	}
	else if (keys[SDL_SCANCODE_DOWN] && (p1.y - p1.height / 2) >= -1.0f) {
		p1.y -= p1.speed * elapsed;
	}
	if (keys[SDL_SCANCODE_W] && (p2.y + p2.height / 2) <= 1.0f) {
		p2.y += p2.speed * elapsed;
	}
	else if (keys[SDL_SCANCODE_S] && (p2.y - p2.height / 2) >= -1.0f) {
		p2.y -= p2.speed * elapsed;
	}
}

void Updates() {
	//if ball hit left/right wall, game over
	bool winP1 = b.x - b.width / 2 <= -1.777f;
	bool winP2 = b.x + b.width / 2 >= 1.777f;
	if (winP1) {
		winner = '1'; // player 1 wins will change all player color to light pink
		cout << "Player 1 wins" << endl;
		done = true;
	}
	else if (winP2) {
		winner = '2'; // player 2 wins will change all player color to light blue
		cout << "Player 2 wins" << endl;
		done = true;
	}

	b.x += b.speed * b.xDir * elapsed;
	b.y += (b.speed / 2) * b.yDir * elapsed;

	// ball if hit upper wall
	if (b.y + b.height/2 >= 1.0f) {
		b.yDir = -b.yDir;
		b.y += (b.speed/2) * b.yDir * elapsed;
		b.x += b.speed * b.xDir * elapsed;
	}
	// ball hit lower wall
	else if (b.y - b.height / 2 <= -1.0f) {
		b.yDir = -b.yDir;
		b.y += (b.speed / 2) * b.yDir * elapsed;
		b.x += b.speed * b.xDir * elapsed;
	}

	// ball hit either player
	bool collideP1 = abs(p1.y - b.y) <= p1.height / 2 && abs(p1.x - b.x) <= p1.width / 2;
	bool collideP2 = abs(p2.y - b.y) <= p2.height / 2 && abs(p2.x - b.x) <= p2.width / 2;
	if (collideP1 || collideP2) {
		b.xDir = -b.xDir;
		b.x += b.speed * b.xDir * elapsed;
		b.y += (b.speed/2) * b.yDir * elapsed;
	}
}

void Render() {
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	program.SetProjectionMatrix(projectionMatrix);
	program.SetViewMatrix(viewMatrix);

	switch (winner) {
	case '1':
		program.SetColor(1.0f, 0.8f, 1.0f, 0.0f);
		break;
	case '2':
		program.SetColor(0.0f, 1.0f, 1.0f, 1.0f);
		break;
	default:
		program.SetColor(1.0f, 0.5f, 0.0f, 0.0f);
	}
	//Player1 drawing
	glm::mat4 modelMatrixP1 = glm::mat4(1.0f);
	modelMatrixP1 = glm::mat4(1.0f);
	modelMatrixP1 = glm::translate(modelMatrixP1, glm::vec3(p1.x, p1.y, 0.0f));
	modelMatrixP1 = glm::scale(modelMatrixP1, glm::vec3(p1.width, p1.height, 1.0f));

	program.SetModelMatrix(modelMatrixP1);
	glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, texturedVertices);
	glEnableVertexAttribArray(program.positionAttribute);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisableVertexAttribArray(program.positionAttribute);
	
	//Player2 drawing
	glm::mat4 modelMatrixP2 = glm::mat4(1.0f);
	modelMatrixP2 = glm::mat4(1.0f);
	modelMatrixP2 = glm::translate(modelMatrixP2, glm::vec3(p2.x, p2.y, 0.0f));
	modelMatrixP2 = glm::scale(modelMatrixP2, glm::vec3(p2.width, p2.height, 1.0f));

	program.SetModelMatrix(modelMatrixP2);
	glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, texturedVertices2);
	glEnableVertexAttribArray(program.positionAttribute);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisableVertexAttribArray(program.positionAttribute);

	//Ball Drawing
	glm::mat4 modelMatrixB = glm::mat4(1.0f);
	modelMatrixB = glm::mat4(1.0f);
	modelMatrixB = glm::translate(modelMatrixB, glm::vec3(b.x, b.y, 0.0f));
	modelMatrixB = glm::scale(modelMatrixB, glm::vec3(b.width, b.height, 1.0f));

	program.SetModelMatrix(modelMatrixB);
	glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, texturedVertices3);
	glEnableVertexAttribArray(program.positionAttribute);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisableVertexAttribArray(program.positionAttribute);
	SDL_GL_SwapWindow(displayWindow);
}

void Cleanup() {
	SDL_Quit();
}

int main(int argc, char *argv[])
{
	Setup();
	while (!done) {
		ProcessEvents();
		Updates();
		Render();
	}
	Cleanup();
	return 0;
}