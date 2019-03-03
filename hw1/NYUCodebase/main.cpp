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

int main(int argc, char *argv[])
{
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
    glewInit();
#endif
	//setup
	glViewport(0, 0, 640, 360);

	//set up variable for keeping time
	float lastFrameTicks = 0.0f;
	float pos1 = -1.5f;
	float pos2 = 1.5f;
	float angle = 0.0f;
	ShaderProgram program;
	//for textured polygon
	program.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
	GLuint pTexture = LoadTexture(RESOURCE_FOLDER"eevee1.png");
	GLuint dTexture = LoadTexture(RESOURCE_FOLDER"eevee2.png");
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//for untextured polygon
	ShaderProgram programU;
	programU.Load(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");
	glm::mat4 viewMatrix = glm::mat4(1.0f);
	glm::mat4 projectionMatrix = glm::mat4(1.0f);
	projectionMatrix = glm::ortho(-1.777f, 1.777f, -1.0f, 1.0f, -1.0f, 1.0f);
	glUseProgram(programU.programID);

    SDL_Event event;
    bool done = false;
    while (!done) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                done = true;
            }
        }
		//draw textured eevees
		glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		//draw espeon
		glm::mat4 pModelMatrix = glm::mat4(1.0f);
		pModelMatrix = glm::translate(pModelMatrix, glm::vec3(-1.0f, 0.0f, 0.0f));
		//pModelMatrix = glm::scale(pModelMatrix, glm::vec3(0.0f, 0.9f, 0.0f));
		program.SetModelMatrix(pModelMatrix);
		program.SetProjectionMatrix(projectionMatrix);
		program.SetViewMatrix(viewMatrix);
		glBindTexture(GL_TEXTURE_2D, pTexture);
		float texturedVertices[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, texturedVertices);
		glEnableVertexAttribArray(program.positionAttribute);
		float texCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);
		//draw umbreon
		glm::mat4 dModelMatrix = glm::mat4(1.0f);
		dModelMatrix = glm::translate(dModelMatrix, glm::vec3(1.0f, 0.0f, 0.0f));
		program.SetModelMatrix(dModelMatrix);
		program.SetProjectionMatrix(projectionMatrix);
		program.SetViewMatrix(viewMatrix);

		glBindTexture(GL_TEXTURE_2D, dTexture);

		float texturedVertices2[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, texturedVertices2);
		glEnableVertexAttribArray(program.positionAttribute);
		float texCoords2[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords2);
		glEnableVertexAttribArray(program.texCoordAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);
		//SDL_GL_SwapWindow(displayWindow);

		//draw untextured triangles
		float untexturedVertices[] = { 0.5f, -0.5f, 0.0f, 0.5f, -0.5f, -0.5f };

		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;

		angle += elapsed;
		pos1 += elapsed;
		pos2 -= elapsed;
		//first triangle
		programU.SetColor(0.9f, 0.1f, 0.9f, 0.9f);
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		if (pos2 <= -2) {
			pos2 = 2;
		}
		modelMatrix = glm::translate(modelMatrix, glm::vec3(pos2, 0.8f, 0.0f));
		modelMatrix = glm::rotate(modelMatrix, -angle, glm::vec3(0.0f, 0.0f, 1.0f));
		modelMatrix = glm::scale(modelMatrix, glm::vec3(0.3f, 0.3f, 1.0f));
		programU.SetModelMatrix(modelMatrix);
		programU.SetProjectionMatrix(projectionMatrix);
		programU.SetViewMatrix(viewMatrix);

		glVertexAttribPointer(programU.positionAttribute, 2, GL_FLOAT, false, 0, untexturedVertices);
		glEnableVertexAttribArray(programU.positionAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glDisableVertexAttribArray(programU.positionAttribute);

		//second triangle
		programU.SetColor(0.1f, 0.0f, 0.1f, 1.0f);
		modelMatrix = glm::mat4(1.0f);
		if (pos1 >= 2) {
			pos1 = -2;
		}
		modelMatrix = glm::translate(modelMatrix, glm::vec3(pos1, 0.8f, 0.0f));
		modelMatrix = glm::rotate(modelMatrix, angle, glm::vec3(0.0f, 0.0f, 1.0f));
		modelMatrix = glm::scale(modelMatrix, glm::vec3(0.3f, 0.3f, 1.0f));
		programU.SetModelMatrix(modelMatrix);
		programU.SetProjectionMatrix(projectionMatrix);
		programU.SetViewMatrix(viewMatrix);

		glVertexAttribPointer(programU.positionAttribute, 2, GL_FLOAT, false, 0, untexturedVertices);
		glEnableVertexAttribArray(programU.positionAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glDisableVertexAttribArray(programU.positionAttribute);

		//third triangle
		programU.SetColor(0.9f, 0.1f, 0.9f, 0.9f);
		modelMatrix = glm::mat4(1.0f);
		if (pos2 <= -2) {
			pos2 = 2;
		}
		modelMatrix = glm::translate(modelMatrix, glm::vec3(pos2, -0.8f, 0.0f));
		modelMatrix = glm::rotate(modelMatrix, -angle, glm::vec3(0.0f, 0.0f, 1.0f));
		modelMatrix = glm::scale(modelMatrix, glm::vec3(0.3f, 0.3f, 1.0f));
		programU.SetModelMatrix(modelMatrix);
		programU.SetProjectionMatrix(projectionMatrix);
		programU.SetViewMatrix(viewMatrix);

		glVertexAttribPointer(programU.positionAttribute, 2, GL_FLOAT, false, 0, untexturedVertices);
		glEnableVertexAttribArray(programU.positionAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glDisableVertexAttribArray(programU.positionAttribute);

		//fourth triangle
		programU.SetColor(0.1f, 0.0f, 0.1f, 1.0f);
		modelMatrix = glm::mat4(1.0f);
		if (pos1 >= 2) {
			pos1 = -2;
		}
		modelMatrix = glm::translate(modelMatrix, glm::vec3(pos1, -0.8f, 0.0f));
		modelMatrix = glm::rotate(modelMatrix, angle, glm::vec3(0.0f, 0.0f, 1.0f));
		modelMatrix = glm::scale(modelMatrix, glm::vec3(0.3f, 0.3f, 1.0f));
		programU.SetModelMatrix(modelMatrix);
		programU.SetProjectionMatrix(projectionMatrix);
		programU.SetViewMatrix(viewMatrix);

		glVertexAttribPointer(programU.positionAttribute, 2, GL_FLOAT, false, 0, untexturedVertices);
		glEnableVertexAttribArray(programU.positionAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glDisableVertexAttribArray(programU.positionAttribute);

		glVertexAttribPointer(programU.positionAttribute, 2, GL_FLOAT, false, 0, untexturedVertices);
		glEnableVertexAttribArray(programU.positionAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glDisableVertexAttribArray(programU.positionAttribute);
		SDL_GL_SwapWindow(displayWindow);
    }
    
    SDL_Quit();
    return 0;
}
