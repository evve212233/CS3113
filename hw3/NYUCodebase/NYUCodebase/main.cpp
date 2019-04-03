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
#include <string>
#include <vector>
#define FIXED_TIMESTAMP 0.0166f

using namespace std;
SDL_Window* displayWindow;

// declaration of functions
void RenderMainMenu();
void DrawSpriteSheet(ShaderProgram &program, int index, int spriteCountX, int spriteCountY);
void DrawText(ShaderProgram &program, int fontTexture, string text, float row, float col, float size, float spacing);
void setup();
void processEvents();
void updates(float elapsed);
void render();
void cleanUp();

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
ShaderProgram program;
/*-------------------SheetSprite Class----------------------------*/
class SheetSprite {
public:
	SheetSprite() {};
	SheetSprite(unsigned int textureID, float u, float v, float w, float h, float sz) : texID(textureID), u(u), v(v), width(w), height(h), size(sz) {};
	void Draw(ShaderProgram &program);
	unsigned int texID;
	glm::vec2 actualSize; //check collision in x and y direction
	float u, v, width, height, size;
};
void SheetSprite::Draw(ShaderProgram &program) {
	glBindTexture(GL_TEXTURE_2D, texID);
	GLfloat texCoords[] = {
		u, v + height, u + width, v, u, v,
		u + width, v, u, v + height, u + width, v + height
	};
	float aspect = width / height;
	actualSize.x = size * aspect;
	actualSize.y = size;

	float vertices[] = {
		-0.5f * size * aspect, -0.5f * size,
		0.5f * size * aspect, 0.5f * size,
		-0.5f * size * aspect, 0.5f * size,
		0.5f * size * aspect, 0.5f * size,
		-0.5f * size * aspect, -0.5f * size ,
		0.5f * size * aspect, -0.5f * size };

	glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(program.positionAttribute);
	glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(program.texCoordAttribute);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisableVertexAttribArray(program.positionAttribute);
	glDisableVertexAttribArray(program.texCoordAttribute);
}

/*-----------Entity class--------------------------------*/
class Entity {
public:
	/*---------------Entity properties-------------------------*/
	void Draw(ShaderProgram &program);
	void Update(float elapsed);
	bool CollidesWith(Entity &entity);
	glm::vec2 position, velocity, size;
	string type;
	SheetSprite sprite;
	float health = 1;

	/*-------------initialize constructors---------------------*/
	Entity() {};
	Entity(const string& tp, SheetSprite& spriteSht, float x, float y, float Vx, float Vy, float xSize, float ySize) {
		position = glm::vec2(x, y);
		velocity = glm::vec2(Vx, Vy);
		size = glm::vec2(xSize, ySize);
		type = tp;
		sprite = spriteSht;
	};
};

/*--------------------------Entity class functions----------------------------------*/
void Entity::Draw(ShaderProgram &program) {
	glm::mat4 modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(position.x, position.y, 1.0f));
	modelMatrix = glm::scale(modelMatrix, glm::vec3(0.8f, 1.0f, 1.0f));
	program.SetModelMatrix(modelMatrix);
	sprite.Draw(program);
}

void Entity::Update(float elapsed) {
	position.x += (elapsed * velocity.x);
	position.y += (elapsed * velocity.y);
	if (type == "player") {
		if (position.x - (sprite.actualSize.x * 0.5f) <= -1.77f) {
			position.x = -1.77f + sprite.actualSize.x / 2.0f;
		}

		if (position.x + (sprite.actualSize.x *0.5f) >= 1.77f) {
			position.x = 1.77f - sprite.actualSize.x / 2.0f;
		}
	}
}

bool Entity::CollidesWith(Entity &entity) {
	bool xCollide = (abs(entity.position.x - position.x) - (0.5f)*(entity.sprite.actualSize.x + sprite.actualSize.x)) < 0;
	bool yCollide = (abs(entity.position.y - position.y) - (0.5f)*(entity.sprite.actualSize.y + sprite.actualSize.y)) < 0;
	return xCollide && yCollide;
}

// bullet global
int bulletInd = 0;
const int maxBullets = 30;
float bTimer = 0.1f;

struct GameState {
	Entity player;
	Entity enemies[12];
	Entity bullets[30];
	void setBullets(SheetSprite &bullet) {
		for (int i = 0; i < maxBullets; i++) {
			bullets[i] = Entity("bullet", bullet, 0.0f, -1.5f, 0.0f, 0.0f, 1.0f, 1.7f);
		}
	}
	int score = 0;
};

/*-----------------GLOBALS-----------------*/
// setups
glm::mat4 viewMatrix = glm::mat4(1.0f);
glm::mat4 projectionMatrix = glm::mat4(1.0f);
GLuint bettyTex;
GLuint fontTex;
GLuint spriteTex;
GLuint heartTex;
SDL_Event event;
bool done = false;

//game mode
enum GameMode { STATE_MAIN_MENU, STATE_GAME_LEVEL, STATE_GAME_OVER };
GameMode mode = STATE_MAIN_MENU;
GameState state;
// elapse time globals
float lastFrameTicks = 0.0f;

// basic sprite sheet globals
int spriteCountX = 4;
int spriteCountY = 4;

// sprite sheet globals for animation
float ticks = 0.0f, elapsed = 0.0f;
const int runAnimation[] = { 0, 4, 8, 12 };
const int numFrames = 4;
float animationElapsed = 0.0f;
float framesPerSecond = 80.0f;
int currentIndex = 0;
float accumulator = 0.0f;

/*----------------Functions---------------*/
void RenderMainMenu() {
	ticks = (float)SDL_GetTicks() / 1000.0f;
	elapsed = ticks - lastFrameTicks;
	if (elapsed < FIXED_TIMESTAMP) {
		lastFrameTicks = elapsed;
	}
	while (elapsed >= FIXED_TIMESTAMP) {
		elapsed -= FIXED_TIMESTAMP;
	}
	animationElapsed += 2.0f *elapsed;
	if (animationElapsed > 1.0 / framesPerSecond) {
		currentIndex++;
		animationElapsed = 0.0f;
		if (currentIndex > numFrames - 1) {
			currentIndex = 0;
		}
	}

	DrawSpriteSheet(program, runAnimation[currentIndex], spriteCountX, spriteCountY);

	string text = "DESCENDANT OF CUPID";
	DrawText(program, fontTex, text, -1.28f, 0.6f, 0.18f, 0.003f);
	string text2 = "Press: SPACE to send heart";
	string text3 = "       LEFT/RIGHT to move";
	string text4 = "Click anywhere to start the game";
	DrawText(program, fontTex, text2, -1.0f, -0.3f, 0.1f, 0.003f);
	DrawText(program, fontTex, text3, -1.01f, -0.5f, 0.08f, 0.003f);
	DrawText(program, fontTex, text4, -1.02f, -0.8f, 0.08f, 0.003f);
}

void RenderGameLevel() {
	DrawText(program, fontTex, "People in love with you: " + to_string(state.score), -1.33f, 0.95f, 0.1f, 0.01f);
	state.player.Draw(program);
	for (Entity &enemy : state.enemies) {
		enemy.Draw(program);
	}
	for (Entity &bullet : state.bullets) {
		bullet.Draw(program);
	}
}


void DrawSpriteSheet(ShaderProgram &program, int index, int spriteCountX, int spriteCountY) {
	float u = (float)(((int)index) % spriteCountX) / (float)spriteCountX;
	float v = (float)(((int)index) / spriteCountY) / (float)spriteCountY;
	float spriteWidth = 1.0 / (float)spriteCountX;
	float spriteHeight = 1.0 / (float)spriteCountY;
	glBindTexture(GL_TEXTURE_2D, bettyTex);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glm::mat4 bettyMatrix = glm::mat4(1.0f);
	bettyMatrix = glm::scale(bettyMatrix, glm::vec3(0.2f, 0.2f, 0.0f));
	program.SetModelMatrix(bettyMatrix);
	float verCoords[] = { -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f,  -0.5f, -0.5f, 0.5f, -0.5f };
	glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, verCoords);
	glEnableVertexAttribArray(program.positionAttribute);

	float texCoords[] = { u, v + spriteHeight, u + spriteWidth, v, u, v, u + spriteWidth, v, u, v + spriteHeight, u + spriteWidth, v + spriteHeight };
	glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(program.texCoordAttribute);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDisableVertexAttribArray(program.positionAttribute);
	glDisableVertexAttribArray(program.texCoordAttribute);
}


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
/*------------------------------------Game Functions---------------------------------------------*/
void setup() {
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("Descendant of Cupid", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
	glewInit();
#endif
	glViewport(0, 0, 640, 360);
	//for textured girl
	program.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
	bettyTex = LoadTexture(RESOURCE_FOLDER"betty_0.png");
	float spriteWidth = 1.0 / (float)spriteCountX;
	float spriteHeight = 1.0 / (float)spriteCountY;
	SheetSprite bettySprite(bettyTex, 0.0f, 0.0f, spriteWidth, spriteHeight, 0.5f);
	fontTex = LoadTexture(RESOURCE_FOLDER"font.png");
	spriteTex = LoadTexture(RESOURCE_FOLDER"enemy.png");
	heartTex = LoadTexture(RESOURCE_FOLDER"heart.png");
	glUseProgram(program.programID);



	SheetSprite girl = SheetSprite(bettyTex, 0.0f, 0.0f, 0.25f, 0.25f, 0.30f);
	SheetSprite enemy = SheetSprite(spriteTex, 0.0f, 0.0f, 0.25f, 0.25f, 0.28f);
	SheetSprite bullet = SheetSprite(heartTex, 0.0f, 0.0f, 1.0f, 1.0f, 0.28f);

	Entity currPlayer = Entity("player", girl, 0.0f, -0.7f, 3.0f, 0.0f, 1.0f, 1.7f);
	state.player = currPlayer;

	float x = -1.7;
	float y = 0.1;
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 10; j++) {
			state.enemies[j] = Entity("enemy", enemy, x, y, 0.0f, 0.0f, 1.6f, 2.0f);
			x += 0.3;
		}
		x = -1.7;
		y += 0.3;
	}
	state.setBullets(bullet);
}

void processEvents() {
	while (SDL_PollEvent(&event)) {
		switch (mode) {
		case STATE_MAIN_MENU:
			while (SDL_PollEvent(&event)) {
				if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
					done = true;
				}
				else if (event.type == SDL_MOUSEBUTTONDOWN) {
						mode = STATE_GAME_LEVEL;
				}
			}
			break;
		case STATE_GAME_LEVEL:
			while (SDL_PollEvent(&event)) {
				if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
					done = true;
				}
				else if (event.key.keysym.scancode == SDL_SCANCODE_SPACE && bTimer >= 0.1) {
					state.bullets[bulletInd].position.x = state.player.position.x;
					state.bullets[bulletInd].position.y = state.player.position.y;
					state.bullets[bulletInd].velocity.y = 3.0f;
					bulletInd++;
					if (bulletInd >= maxBullets) {
						bulletInd = 0;
					}
					bTimer = 0;
				}
				break;
		case STATE_GAME_OVER:
			while (SDL_PollEvent(&event)) {
				if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
					done = true;
				}
			}
			break;
			}
		}
	}
}

void updates(float elapsed) {
	const Uint8* keys = SDL_GetKeyboardState(NULL);

	switch (mode) {
	case STATE_MAIN_MENU:
		break;
	case STATE_GAME_LEVEL:
		if (keys[SDL_SCANCODE_LEFT]) {
			state.player.velocity.x = -1.5f;
		}
		else if (keys[SDL_SCANCODE_RIGHT]) {
			state.player.velocity.x = 1.5f;
		}
		else {
			state.player.velocity.x = 0.0f;
		}
		state.player.Update(elapsed);
		for (Entity &enemy : state.enemies) {
			if (enemy.position.y < 1000) {
				if ((enemy.position.x - enemy.sprite.actualSize.x*0.5f) >= -1.77f) {
					enemy.velocity.x = 0.1f;
				}
				else if ((enemy.position.x + enemy.sprite.actualSize.x*0.5f) <= 1.77f) {
					enemy.velocity.x = -0.1f;
				}
				else {
					enemy.velocity.x = 0.0f;
				}
			}
			if ((enemy.position.x - (enemy.sprite.actualSize.x*0.5f) <= -1.77f)) {
				for (Entity &enemy : state.enemies) {
					enemy.position.y -= 0.08f;
					enemy.position.x += 0.2f;
				}
			}
			else if (enemy.position.x + (enemy.sprite.actualSize.x*0.5f) >= 1.77f) {
				for (Entity &enemy : state.enemies) {
					enemy.position.y -= 0.08f;
					enemy.position.x -= 0.2f;
				}
			}
		}
		for (Entity &enemy : state.enemies) {
			for (Entity &bullet : state.bullets) {
				if (bullet.CollidesWith(enemy)) {
					enemy.position.y = 1000.0f;
					bullet.position.x = -1000.0f;
					state.score++;
				}
			}
			if (enemy.CollidesWith(state.player)) {
				mode = STATE_GAME_OVER;
			}
			if ((enemy.position.y - 0.5f*enemy.sprite.actualSize.y) <= -1.0f) {
				mode = STATE_GAME_OVER;
			}
		}

		for (Entity &bullet : state.bullets) {
			bullet.Update(elapsed);
		}
		for (Entity &enemy : state.enemies) {
			enemy.Update(elapsed);

		}
		bTimer += elapsed;
		if (state.score == 3) {
			mode = STATE_GAME_OVER;
		}

	case STATE_GAME_OVER:
		break;
	}
}

void render() {
	// general
	glClear(GL_COLOR_BUFFER_BIT);
	glClearColor(1.0f, 0.2f, 0.7f, 1.0f);
	projectionMatrix = glm::ortho(-1.77f, 1.77f, -1.0f, 1.0f, -1.0f, 1.0f);
	program.SetProjectionMatrix(projectionMatrix);
	program.SetViewMatrix(viewMatrix);
	switch (mode)
	{
	case STATE_MAIN_MENU:
		RenderMainMenu();
		break;
	case STATE_GAME_LEVEL:
		RenderGameLevel();
		break;
	case STATE_GAME_OVER:
		if (state.score == 3) {
			string msgP = "Congrats, You are loved by all on this planet now!";
			DrawText(program, fontTex, msgP, -1.45f, 0.0f, 0.1f, 0.005f);
		}
		else {
			string msgF = "Not good. Please don't trust others so quickly!";
			DrawText(program, fontTex, msgF, -1.45f, 0.0f, 0.1f, 0.005f);
		}
		break;
	}
	SDL_GL_SwapWindow(displayWindow);
}
void cleanUp() {
	SDL_Quit();
}

int main(int argc, char *argv[])
{
	setup();
	while (!done) {
		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;

		elapsed += accumulator;
		if (elapsed < FIXED_TIMESTAMP) {
			accumulator = elapsed;
			continue;

		}
		while (elapsed >= FIXED_TIMESTAMP) {
			updates(FIXED_TIMESTAMP);
			elapsed -= FIXED_TIMESTAMP;
		}
		accumulator = elapsed;
		processEvents();
		render();
	}
	cleanUp();
	return 0;
}