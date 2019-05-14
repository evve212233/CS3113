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

#include "FlareMap.h"
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
#include <algorithm>
#include <SDL_mixer.h>
#define LEVEL_HEIGHT 10.0f
#define LEVEL_WIDTH 40.0f
#define tileSize 0.18f
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
void reset(FlareMap &m);
float friction = 0.8f;

//setup variables
SDL_Event event;
bool done = false;
ShaderProgram program;
ShaderProgram untxtprogram;
glm::mat4 projectionMatrix = glm::mat4(1.0f);

//for entity class
enum EntityType { ENTITY_PLAYER, ENTITY_ENEMY, ENTITY_GOAL };
class Entity;
vector <Entity> entities;
glm::vec2 startPos;
float gravity = 0.0f;
FlareMap map;
FlareMap map2;
FlareMap map3;

//game mode
enum GameMode { STATE_MAIN_MENU, STATE_GAME_LEVEL1, STATE_GAME_LEVEL2, STATE_GAME_LEVEL3, STATE_GAME_OVER };
GameMode mode = STATE_MAIN_MENU;
bool fail = false;
bool win1 = false;
bool win2 = false;
bool win3 = false;

//animation
const int runAnimation[] = { 3, 4, 5, 6, 7, 8, 9};
const int numFrames = 7;
float animationElapsed = 0.0f;
float framesPerSecond = 160.0f;
int currentIndex = 0;
//betty move index
const int playerFrames = 4;
const int bettyDown[] = { 0, 4, 8, 12 };
const int bettyLeft[] = { 1, 5, 9, 13 };
const int bettyUp[] = { 2, 6, 10, 14 };
const int bettyRight[] = { 3, 7, 11, 15 };

const int hDown[] = { 0, 4, 8, 12 };
const int hLeft[] = { 1, 5, 9, 13 };
const int hUp[] = { 2, 6, 10, 14 };
const int hRight[] = { 3, 7, 11, 15 };
int playerInd=0;
int enemyInd=1;

//particle
class Particle {
public:
	Particle(float x, float y) :position(x, y, 0.0f), velocity(-0.2f, -0.2f, 0.0f){}
	glm::vec3 position;
	glm::vec3 velocity;
	glm::vec3 size;
	float lifetime;
};

class ParticleEmitter {
public:
	ParticleEmitter(unsigned int particleCount, float maxLifeTime = 1.0f) : gravity(1.0f), maxLifetime(maxLifeTime)
	{
		for (int i = 0; i < particleCount; i++) {
			Particle p(position.x, position.y);
			p.lifetime = ((float)rand() / (float)RAND_MAX) * maxLifetime;
			p.size.x *= ((float)rand() / (float)RAND_MAX);
			p.size.y *= ((float)rand() / (float)RAND_MAX);
			p.velocity.x *= ((float)rand() / (float)RAND_MAX) * ((i % 2 == 0) ? 1 : -1);
			p.velocity.y *= ((float)rand() / (float)RAND_MAX);
			particles.push_back(p);
		}
	}
	ParticleEmitter() {};
	~ParticleEmitter() {};

	void Update(float elapsed) {
		for (int i = 0; i < particles.size(); i++) {
			particles[i].position.x += particles[i].velocity.x * elapsed;
			particles[i].position.y += particles[i].velocity.y * elapsed;
			particles[i].velocity.y += gravity.y * elapsed;
			particles[i].lifetime += elapsed;
			if (particles[i].lifetime >= maxLifetime) {
				particles[i].lifetime = 0.0f;
				particles[i].position.x = position.x;
				particles[i].position.y = position.y;
				particles[i].velocity.y = 0.2f;
				particles[i].velocity.x = 0.2f;
				particles[i].velocity.x *= ((float)rand() / (float)RAND_MAX) * ((i % 2 == 0) ? 1 : -1);
				particles[i].velocity.y *= ((float)rand() / (float)RAND_MAX);
			}
		}
	}
	void Render(ShaderProgram& untextured) {
		glm::mat4 modelMatrix(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(position.x, position.y, 0.0f));
		untxtprogram.SetModelMatrix(modelMatrix);
		vector<float> vertices;
		for (int i = 0; i < particles.size(); i++) {
			vertices.push_back(particles[i].position.x);
			vertices.push_back(particles[i].position.y);
		}

		glVertexAttribPointer(untxtprogram.positionAttribute, 2, GL_FLOAT, false, 0, vertices.data());
		glEnableVertexAttribArray(untxtprogram.positionAttribute);
		glDrawArrays(GL_POINTS, 0, particles.size());
	}

	glm::vec3 position;
	glm::vec3 gravity;
	float maxLifetime;
	std::vector<Particle> particles;
};

class Entity {
public:
	Entity() {}
	GLuint texture;
	glm::vec2 position; //vec2 for 2D
	glm::vec2 size; //for 2D
	glm::vec2 velocity;
	glm::vec2 acceleration;
	FlareMap map;

	bool collideL = false, collideR = false, collideU = false, collideD = false;
	bool canJump = false;

	EntityType entityType;

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
		DrawSpriteSheet(program, texture, index, spriteCountX, spriteCountY);
	}
	bool checkEntityCollision(const Entity& e) {
		return !(position.x + 0.5f * size.x < e.position.x - 0.5f * e.size.x ||
			position.x - 0.5f * size.x > e.position.x + 0.5f * e.size.x ||
			position.y + 0.5f * size.y < e.position.y - 0.5f * e.size.y ||
			position.y - 0.5f * size.y > e.position.y + 0.5f * e.size.y);
	}


	void update(float elapsed) {
		if (entityType == ENTITY_PLAYER) {
			if (canJump == false) {
				gravity = -2.0f;
			}
			else {
				gravity = 0.0f;
			}
			for (const Entity &e : entities) {
				if (checkEntityCollision(e) && e.entityType == ENTITY_ENEMY) {
					fail = true;
				}
				if (checkEntityCollision(e) && e.entityType == ENTITY_GOAL && mode == STATE_GAME_LEVEL1) {
					win1 = true;
				}
				if (checkEntityCollision(e) && e.entityType == ENTITY_GOAL && mode == STATE_GAME_LEVEL2) {
					win2 = true;
				}
				if (checkEntityCollision(e) && e.entityType == ENTITY_GOAL && mode == STATE_GAME_LEVEL3) {
					win3 = true;
				}
			}
		}

		// friction slows down velocity
		velocity.x = lerp(velocity.x, 0.0f, elapsed*friction);
		velocity.y = lerp(velocity.y, 0.0f, elapsed*friction);
		velocity.x += acceleration.x * elapsed;
		velocity.y += (acceleration.y + gravity) * elapsed;
		position.y += elapsed * velocity.y;
		position.x += elapsed * velocity.x;
		CollideBotMap();
		CollideTopMap();
		CollideLeftMap();
		CollideRightMap();
	}

private:

	void CollideLeftMap() {
		int gridX = (int)(position.x / (tileSize)-size.y / 2);
		int gridY = (int)(position.y / (-tileSize));
		if (gridX < map.mapWidth && abs(gridY) < map.mapHeight) {
			if (map.mapData[gridY][gridX] != -1) {
				float penetration = fabs(((tileSize * gridX) + tileSize) - (position.x - size.x / 2.0f));
				penetration -= 0.04f;
				position.x += penetration;
				collideL = true;
				velocity.x = 0;
			}
			else {
				collideL = false;
			}
		}
	}

	void CollideRightMap() {
		int gridX = (int)(position.x / (tileSize)+size.y / 2);
		int gridY = (int)(position.y / (-tileSize));
		if (gridX < map.mapWidth && abs(gridY) < map.mapHeight) {
			if (map.mapData[gridY][gridX] != -1) {
				float penetration = fabs(((tileSize*gridX) - position.x - (size.x / 2)*tileSize));
				position.x -= penetration;
				collideR = true;
				velocity.x = 0;
			}
			else {
				collideR = false;
			}
		}
	}

	void CollideBotMap() {
		int gridX = (int)(position.x / (tileSize));
		int gridY = (int)(position.y / (-tileSize) + size.y / 2);
		if (gridX < map.mapWidth && abs(gridY) < map.mapHeight) {
			if (map.mapData[gridY][gridX] != -1) {
				float penetration = fabs(((-tileSize * gridY) - (position.y - (size.y / 2)*tileSize)));
				position.y += penetration;
				collideD = true;
				if (!canJump) {
					velocity.y = 0;
				}
				else {
					canJump = false;
				}
			}
			else {
				collideD = false;
			}
		}
	}

	void CollideTopMap() {
		int gridX = (int)(position.x / (tileSize));
		int gridY = (int)(position.y / (-tileSize) - size.y / 2);
		if (gridX < map.mapWidth && abs(gridY) < map.mapHeight) {
			if (map.mapData[gridY][gridX] != -1) {
				float penetration = fabs(((-tileSize * gridY) - (position.y + (size.y / 2)*tileSize))) / 5.0f;
				position.y -= penetration;
				velocity.y = 0;
				collideU = true;
			}
			else {
				collideU = false;
			}
		}
	}

};


ParticleEmitter follower (150, 2.5f);
/* global variables */
GLuint bettyTexture;
GLuint helperTexture;
GLuint fontTex;
GLuint mapTex;
GLuint starTex;
GLuint angelTex;

// time and speed
float accumulator = 0.0f;
float lastFrameTicks = 0.0f;

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

/*-----------------Render Map-------------------------------*/
void renderMap(int sprite_count_x, int sprite_count_y, FlareMap &m) {
	glm::mat4 modelMapMatrix = glm::mat4(1.0f);
	program.SetModelMatrix(modelMapMatrix);

	vector<float> vertexData;
	vector<float> texCoordData;
	for (int y = 0; y < m.mapHeight; y++) {
		for (int x = 0; x < m.mapWidth; x++) {
			if (m.mapData[y][x] != -1) {
				float u = (float)(((int)m.mapData[y][x]) % sprite_count_x) / (float)sprite_count_x;
				float v = (float)(((int)m.mapData[y][x]) / sprite_count_x) / (float)sprite_count_y + 0.0022f;
				float spriteWidth = 1.0f / (float)sprite_count_x - 0.0006f;
				float spriteHeight = 1.0f / (float)sprite_count_y - 0.005f;
				vertexData.insert(vertexData.end(), {
					tileSize * x, -tileSize * y,
					tileSize * x, (-tileSize * y) - tileSize,
					(tileSize * x) + tileSize, (-tileSize * y) - tileSize,
					tileSize * x, -tileSize * y,
					(tileSize * x) + tileSize, (-tileSize * y) - tileSize,
					(tileSize * x) + tileSize, -tileSize * y
					});
				texCoordData.insert(texCoordData.end(), {
					u, v,
					u, v + (spriteHeight),
					u + spriteWidth, v + (spriteHeight),
					u, v,
					u + spriteWidth, v + (spriteHeight),
					u + spriteWidth, v
					});
			}
		}
	}
	int size = vertexData.size() / 2;
	glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
	glEnableVertexAttribArray(program.positionAttribute);
	glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
	glEnableVertexAttribArray(program.texCoordAttribute);
	glBindTexture(GL_TEXTURE_2D, mapTex);
	glDrawArrays(GL_TRIANGLES, 0, size);
	glDisableVertexAttribArray(program.positionAttribute);
	glDisableVertexAttribArray(program.texCoordAttribute);
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


/*-------------------------------Friction--------------------------------------------*/
float lerp(float v0, float v1, float t) {
	return (1.0 - t) * v0 + t * v1;
}

Entity betty;
Entity helper;
Entity enemy;
Entity goal;
glm::vec2 starPos;

void placeEntity(string type, float placeX, float placeY, FlareMap &m) {
	float worldX = placeX * tileSize;
	float worldY = placeY * -tileSize;
	if (type == "player") {
		cout << worldX << " " << worldY << endl;
		startPos.x = worldX + 0.085f;
		startPos.y = worldY + 0.08f;
		betty = Entity(worldX, worldY, 0.18f, 0.18f, 0.0f, 0.0f, 0.0f, 0.0f);
		betty.entityType = ENTITY_PLAYER;
		betty.map = m;
	}
	if (type == "player2") {
		cout << worldX << " " << worldY << endl;
		startPos.x = worldX + 0.085f;
		startPos.y = worldY + 0.08f;
		helper = Entity(worldX, worldY, 0.18f, 0.18f, 0.0f, 0.0f, 0.0f, 0.0f);
		helper.entityType = ENTITY_PLAYER;
		helper.map = m;
	}
	if (type == "enemy") {
		cout << worldX << " " << worldY << endl;
		enemy = Entity(worldX, worldY, 0.16f, 0.18f, 0.0f, 0.0f, 0.0f, 0.0f);
		enemy.entityType = ENTITY_ENEMY;
		entities.push_back(enemy);
		enemy.map = m;
	}
	if (type == "goal") {
		cout << worldX << " " << worldY << endl;
		goal = Entity(worldX + 0.088f, worldY + 0.05f, 0.15f, 0.17f, 0.0f, 0.0f, 0.0f, 0.0f);
		starPos.x = worldX + 0.088f;
		starPos.y = worldY + 0.05f;
		goal.entityType = ENTITY_GOAL;
		entities.push_back(goal);
		goal.map = m;
	}
}

/*-----------------------Translate world to tile coordinates---------------------*/
void worldToTileCoordinates(float worldX, float worldY, int *gridX, int *gridY) {
	*gridX = (int)(worldX / tileSize);
	*gridY = (int)(worldY / tileSize);
}

/*-------------------------------Render Main Menu------------------------------------*/
void RenderMainMenu() {
	string text = "Super Betty: Find Star";
	string text2 = "Press: SPACE to jump";
	string text3 = "LEFT/RIGHT to move";
	string text4 = "Click anywhere to start the game";
	string text5 = "ESC to quit";
	DrawText(program, fontTex, text, -1.35f, 0.4f, 0.15f, 0.003f);
	DrawText(program, fontTex, text2, -1.0f, 0.1f, 0.11f, 0.003f);
	DrawText(program, fontTex, text3, -0.37f, -0.12f, 0.105f, 0.003f);
	DrawText(program, fontTex, text5, -0.37f, -0.34f, 0.105f, 0.003f);
	DrawText(program, fontTex, text4, -1.4f, -0.7f, 0.11f, 0.003f);
}
/*-------------------------------game over menu-----------------------------------*/
void RenderGameOver() {
	if (!fail) {
		string text = "Congrats!";
		string text2 = "You are the brighest star!";
		DrawText(program, fontTex, text, -0.4f, 0.2f, 0.15f, 0.003f);
		DrawText(program, fontTex, text2,-1.5f, -0.2f, 0.15f, 0.003f);
	}
	else {
		string tex = "You are caught!";
		DrawText(program, fontTex, tex, 2.43f, 0.0f, 0.20f, 0.003f);
		string text = "Retry: Press Enter";
		DrawText(program, fontTex, text, 2.65f, -0.8f, 0.11f, 0.003f);
		string text2 = "Exit:  Press ESC";
		DrawText(program, fontTex, text2, 2.65f, -1.0f, 0.11f, 0.003f);
		enemy.drawUniform(program, 11, 3, 4);
	}
}

/*---------------------Functions To be Called in Main()---------------------------*/
void Setup() {
	//setup SDL, OpenGL, projection matrix
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 940, 500, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
	glewInit();
#endif
	//setup
	glViewport(0, 0, 940, 500);
	glClearColor(0.6f, 0.8f, 1.0f, 0.0f);
	glm::mat4 viewMatrix = glm::mat4(1.0f);
	projectionMatrix = glm::ortho(-1.88f, 1.88f, -1.0f, 1.0f, -1.0f, 1.0f);
	//for untextured
	untxtprogram.Load(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");
	untxtprogram.SetViewMatrix(viewMatrix);
	untxtprogram.SetProjectionMatrix(projectionMatrix);
	glUseProgram(untxtprogram.programID);

	//for textured polygon
	program.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
	bettyTexture = LoadTexture(RESOURCE_FOLDER"betty.png");
	helperTexture = LoadTexture(RESOURCE_FOLDER"helper.png");
	fontTex = LoadTexture(RESOURCE_FOLDER"font.png");
	mapTex = LoadTexture(RESOURCE_FOLDER"mapSprite.png");
	starTex = LoadTexture(RESOURCE_FOLDER"star.png");
	angelTex = LoadTexture(RESOURCE_FOLDER"angel.png");
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	program.SetViewMatrix(viewMatrix);
	program.SetProjectionMatrix(projectionMatrix);
	glUseProgram(program.programID);
	map.Load("map.txt");
	map2.Load("map2.txt");
	map3.Load("map3.txt");
	reset(map);
	follower.position.x = betty.position.x;
	follower.position.y = betty.position.y;
	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);
	Mix_Music *music;
	music = Mix_LoadMUS("background.mp3");
	Mix_PlayMusic(music, -1);
}

void ProcessEvents() {
	//check input events
	while (SDL_PollEvent(&event)) {
		switch (mode) {
		case STATE_MAIN_MENU:
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
			else if (event.type == SDL_MOUSEBUTTONDOWN) {
				mode = STATE_GAME_LEVEL1;
			}
			break;
		case STATE_GAME_LEVEL1:
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
			else if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
				done = true;
			}
			break;
		case STATE_GAME_LEVEL2:
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
			else if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
				done = true;
			}
			break;
		case STATE_GAME_LEVEL3:
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
			else if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
				done = true;
			}
			break;
		case STATE_GAME_OVER:
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
			else if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
				done = true;
			}
			else if (event.key.keysym.scancode == SDL_SCANCODE_RETURN) {
				mode = STATE_GAME_LEVEL1;
				reset(map);
			}
			break;
		}
	}
}

//globals for animation character's movement
int hInd = 0;
bool rightClick = false;
bool leftClick = false;
bool rightClickH = false;
bool leftClickH = false;
//global for sound
Mix_Chunk *someSound;


void Update(float elapsed) {
	//move stuff and check for collisions
	const Uint8* keys = SDL_GetKeyboardState(NULL);
	switch (mode)
	{
	case STATE_MAIN_MENU:
		break;
	case STATE_GAME_LEVEL1:
		betty.acceleration.x = 0.0f;
		betty.acceleration.y = 0.0f;
		betty.canJump = false;

		helper.acceleration.x = 0.0f;
		helper.acceleration.y = 0.0f;
		helper.canJump = false;
		
		rightClick = false;
		leftClick = false;
		rightClickH = false;
		leftClickH = false;

		if (keys[SDL_SCANCODE_D] && helper.collideR == false) {
			helper.acceleration.x = 1.0f;
			rightClickH = true;
		}
		if (keys[SDL_SCANCODE_A] && helper.collideL == false) {
			helper.acceleration.x = -1.0f;
			leftClickH = true;
		}
		if (keys[SDL_SCANCODE_W] && helper.collideU == false && helper.collideD) {
			helper.canJump = true;
			helper.velocity.y = 1.5f;
			helper.acceleration.y = 2.0f;
		}


		if (keys[SDL_SCANCODE_RIGHT] && betty.collideR == false) {
			betty.acceleration.x = 1.0f;
			rightClick = true;
		}
		if (keys[SDL_SCANCODE_LEFT] && betty.collideL == false) {
			betty.acceleration.x = -1.0f;
			leftClick = true;
		}
		if (keys[SDL_SCANCODE_SPACE] && betty.collideU == false && betty.collideD) {
			betty.canJump = true;
			betty.velocity.y = 1.5f;
			betty.acceleration.y = 2.0f;
		}

		if (win1) {
			cout << "congrats1!" << endl;
			reset(map2);
			mode = STATE_GAME_LEVEL2;
		}

		else if (fail) {
			cout << "It's ok, please try again!";
			mode = STATE_GAME_OVER;
		}
		else {
			betty.update(elapsed);
			betty.acceleration.x = 0.0f;
			helper.update(elapsed);
			helper.acceleration.x = 0.0;
			follower.position.x = betty.position.x;
			follower.position.y = betty.position.y;
			follower.Update(elapsed);
		}
		enemy.update(elapsed);

		glm::mat4 viewMatrix = glm::mat4(1.0f);
		float xMin, yMin;
		if (betty.position.x < goal.position.x) {
			xMin = xMin = min(-betty.position.x, -helper.position.x);
			xMin = min(xMin, -1.88f);
			yMin = min(-betty.position.y, -helper.position.y);
			yMin = min(yMin, 1.0f);
		}
		else {
			xMin = min(-betty.position.x, -helper.position.x);
			yMin = min(-betty.position.y, -helper.position.y);
			xMin = min(xMin, -goal.position.x);
			yMin = min(yMin, 1.0f);
		}
		viewMatrix = glm::translate(viewMatrix, glm::vec3(xMin, yMin, 0.0f));
		program.SetViewMatrix(viewMatrix);
		untxtprogram.SetViewMatrix(viewMatrix);
		//cout << betty.position.x << " " << betty.position.y << endl;
		break;

	case STATE_GAME_LEVEL2:
		betty.acceleration.x = 0.0f;
		betty.acceleration.y = 0.0f;
		betty.canJump = false;

		rightClick = false;
		leftClick = false;

		if (keys[SDL_SCANCODE_RIGHT] && betty.collideR == false) {
			betty.acceleration.x = 1.0f;
			rightClick = true;
		}
		if (keys[SDL_SCANCODE_LEFT] && betty.collideL == false) {
			betty.acceleration.x = -1.0f;
			leftClick = true;
		}
		if (keys[SDL_SCANCODE_SPACE] && betty.collideU == false && betty.collideD) {
			betty.canJump = true;
			betty.velocity.y = 1.5f;
			betty.acceleration.y = 2.0f;
		}

		if (win2) {
			cout << "congrats2!" << endl;
			reset(map3);
			mode = STATE_GAME_LEVEL3;
		}

		else if (fail) {
			cout << "It's ok, please try again!";
			reset(map2);
		}
		else {
			betty.update(elapsed);
			betty.acceleration.x = 0.0f;
			follower.position.x = betty.position.x;
			follower.position.y = betty.position.y;
			follower.Update(elapsed);
		}
		enemy.update(elapsed);

		viewMatrix = glm::mat4(1.0f);
		if (betty.position.x < goal.position.x) {
			xMin = min(-betty.position.x, -1.88f);
			yMin = min(-betty.position.y, 1.0f);
		}
		else {
			xMin = min(-betty.position.x, -goal.position.x);
			yMin = min(-betty.position.y, 1.0f);
		}
		viewMatrix = glm::translate(viewMatrix, glm::vec3(xMin, yMin, 0.0f));
		program.SetViewMatrix(viewMatrix);
		untxtprogram.SetViewMatrix(viewMatrix);
		//cout << betty.position.x << " " << betty.position.y << endl;
		break;

	case STATE_GAME_LEVEL3:
		betty.acceleration.x = 0.0f;
		betty.acceleration.y = 0.0f;
		betty.canJump = false;

		rightClick = false;
		leftClick = false;

		if (keys[SDL_SCANCODE_RIGHT] && betty.collideR == false) {
			betty.acceleration.x = 1.0f;
			rightClick = true;
		}
		if (keys[SDL_SCANCODE_LEFT] && betty.collideL == false) {
			betty.acceleration.x = -1.0f;
			leftClick = true;
		}
		if (keys[SDL_SCANCODE_SPACE] && betty.collideU == false && betty.collideD) {
			betty.canJump = true;
			betty.velocity.y = 1.5f;
			betty.acceleration.y = 2.0f;
		}

		if (win3) {
			cout << "congrats3!" << endl;
			reset(map3);
			mode = STATE_GAME_OVER;
		}

		else if (fail) {
			cout << "It's ok, please try again!";
			reset(map3);
		}
		else {
			betty.update(elapsed);
			betty.acceleration.x = 0.0f;
			follower.position.x = betty.position.x;
			follower.position.y = betty.position.y;
			follower.Update(elapsed);
		}
		enemy.update(elapsed);

		viewMatrix = glm::mat4(1.0f);
		if (betty.position.x < goal.position.x) {
			xMin = min(-betty.position.x, -1.88f);
			yMin = min(-betty.position.y, 1.0f);
		}
		else {
			xMin = min(-betty.position.x, -goal.position.x);
			yMin = min(-betty.position.y, 1.0f);
		}
		viewMatrix = glm::translate(viewMatrix, glm::vec3(xMin, yMin, 0.0f));
		program.SetViewMatrix(viewMatrix);
		untxtprogram.SetViewMatrix(viewMatrix);
		//cout << betty.position.x << " " << betty.position.y << endl;
		break;
	default:
		break;
	}
}

void reset(FlareMap &m) {
	entities.clear();
	fail = false;
	for (FlareMapEntity entity : m.entities) {
		placeEntity(entity.type, entity.x, entity.y, m);
	}
}

void Render() {
	//for all game elements
	glClear(GL_COLOR_BUFFER_BIT);

	betty.texture = bettyTexture;
	helper.texture = helperTexture;
	enemy.texture = angelTex;
	goal.texture = starTex;

	if (mode == STATE_MAIN_MENU) {
		RenderMainMenu();
	}
	else if (mode == STATE_GAME_LEVEL1) {
		glClearColor(0.6f, 0.8f, 1.0f, 0.0f);
		int bcountX = 4;
		int bcountY = 4;
		if (rightClick) {
			betty.drawUniform(program, bettyRight[playerInd], bcountX, bcountY);
		}
		else if (leftClick){
			betty.drawUniform(program, bettyLeft[playerInd], bcountX, bcountY);
		}
		else {
			betty.drawUniform(program, 0, bcountX, bcountY);
		}

		if (rightClickH) {
			helper.drawUniform(program, bettyRight[playerInd], bcountX, bcountY);
		}
		else if (leftClickH) {
			helper.drawUniform(program, bettyLeft[playerInd], bcountX, bcountY);
		}
		else {
			helper.drawUniform(program, hInd, bcountX, bcountY);
		}
		follower.Render(untxtprogram);
		enemy.drawUniform(program, runAnimation[currentIndex], 3, 4);
		goal.drawUniform(program, 0, 1, 1);
		renderMap(16, 8, map);
	}
	else if (mode == STATE_GAME_LEVEL2) {
		glClearColor(1.0f, 0.6f, 0.0f, 0.0f);
		int index = 0;
		int bcountX = 4;
		int bcountY = 4;
		if (rightClick) {
			betty.drawUniform(program, bettyRight[playerInd], bcountX, bcountY);
		}
		else if (leftClick) {
			betty.drawUniform(program, bettyLeft[playerInd], bcountX, bcountY);
		}
		else {
			betty.drawUniform(program, 0, bcountX, bcountY);
		}
		enemy.drawUniform(program, runAnimation[currentIndex], 3, 4);
		goal.drawUniform(program, 0, 1, 1);
		follower.Render(untxtprogram);
		renderMap(16, 8, map2);
	}
	else if (mode == STATE_GAME_LEVEL3) {
		glClearColor(0.0f, 0.1f, 0.0f, 0.0f);
		int index = 0;
		int bcountX = 4;
		int bcountY = 4;
		if (rightClick) {
			betty.drawUniform(program, bettyRight[playerInd], bcountX, bcountY);
		}
		else if (leftClick) {
			betty.drawUniform(program, bettyLeft[playerInd], bcountX, bcountY);
		}
		else {
			betty.drawUniform(program, 0, bcountX, bcountY);
		}
		enemy.drawUniform(program, runAnimation[currentIndex], 3, 4);
		goal.drawUniform(program, 0, 1, 1);
		follower.Render(untxtprogram);
		renderMap(16, 8, map3);
	}
	else if (mode == STATE_GAME_OVER) {
		if (fail) {
			RenderGameOver();
		}
		else {
			glm::mat4 viewMatrix = glm::mat4(1.0f);
			program.SetViewMatrix(viewMatrix);
			goal.position.x = 0.0f;
			goal.position.y = 0.0f;
			goal.size += 0.5f;
			goal.drawUniform(program, 0, 1, 1);
			RenderGameOver();
		}
	}

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

		//animation time update
		animationElapsed += elapsed;
		if (animationElapsed > 1.0 / framesPerSecond) {
			currentIndex++;
			playerInd++;
			animationElapsed = 0.0;
			if (currentIndex > numFrames - 1) {
				currentIndex = 0;
			}
			if (playerInd > playerFrames - 1) {
				playerInd = 0;
			}
		}
		accumulator = elapsed;
		ProcessEvents();
		Render();
	}
	Cleanup();
	return 0;
}