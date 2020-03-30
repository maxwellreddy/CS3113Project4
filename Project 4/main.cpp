// Maxwell Reddy
// CS 3113
// Assignment 4

#define GL_SILENCE_DEPRECATION

#ifdef _WINDOWS
#include <GL/glew.h>
#include <vector> 
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "Entity.h"

#define BLOCK_COUNT 20

#define ENEMY_COUNT 3

#define FIRE_COUNT 3

struct GameState {
	Entity* player;
	Entity* blocks;
	Entity* enemy;
	Entity* fire;
};

GameState state;

SDL_Window* displayWindow;
bool gameIsRunning = true;
bool restarting = false;

int fireIndex = 0;
int fireCount = 0;
int remainingFire = 3;

ShaderProgram program;
glm::mat4 viewMatrix, modelMatrix, projectionMatrix;

GLuint LoadTexture(const char* filePath) {
	int w, h, n;
	unsigned char* image = stbi_load(filePath, &w, &h, &n, STBI_rgb_alpha);

	if (image == NULL) {
		std::cout << "Unable to load image. Make sure the path is correct\n";
		assert(false);
	}

	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	stbi_image_free(image);
	return textureID;
}


void Initialize() {
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("Project 4: Arle Attacks Everyone With Fire", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
	glewInit();
#endif

	glViewport(0, 0, 640, 480);

	program.Load("shaders/vertex_textured.glsl", "shaders/fragment_textured.glsl");

	viewMatrix = glm::mat4(1.0f);
	modelMatrix = glm::mat4(1.0f);
	projectionMatrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

	program.SetProjectionMatrix(projectionMatrix);
	program.SetViewMatrix(viewMatrix);

	glUseProgram(program.programID);

	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
	glEnable(GL_BLEND);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


	// Initialize Game Objects

	// Initializes the player Arle (The girl with orange hair and the blue and white clothing)
	state.player = new Entity();
	state.player->entityType = PLAYER;
	state.player->direction = RIGHT;
	state.player->position = glm::vec3(0.0f, 1.0f, 0);
	state.player->acceleration = glm::vec3(0, -9.81f, 0);
	state.player->textureID = LoadTexture("Player.png");
	state.player->height = 2.0f;
	state.player->width = 0.9f;

	// Initialize blocks
	state.blocks = new Entity[BLOCK_COUNT];

	GLuint blockTextureID = LoadTexture("Block.png");
	for (int i = 0; i < BLOCK_COUNT; i++) {
		state.blocks[i].entityType = BLOCK;
		state.blocks[i].textureID = blockTextureID;
		state.blocks[i].type = 0;
		if (i <= 9) {
			state.blocks[i].position = glm::vec3(float(i - 4.5), -2.25f, 0);
		}
		else if (i >= 10 && i <= 14) {
			state.blocks[i].position = glm::vec3(-4.5f, float(i - 10) -2.25f, 0);
		}
		else if (i >= 15 && i <= 19) {
			state.blocks[i].position = glm::vec3(4.5f, float(i - 15) -2.25f, 0);
		}
	}

	for (int i = 0; i < BLOCK_COUNT; i++) {
		state.blocks[i].Update(0, NULL, 0, NULL, 0);
	}

	// Initializes enemies
	state.enemy = new Entity[ENEMY_COUNT];
	//Creating Onion Pixy (The onion creature with a bat)
	state.enemy[0].textureID = LoadTexture("Enemy1.png");
	// Creating Draco (The girl with green hair and red clothes)
	state.enemy[1].textureID = LoadTexture("Enemy2.png");
	// Creating Witch (The character with dark clothes and riding the broomstick)
	state.enemy[2].textureID = LoadTexture("Enemy3.png");
	state.enemy[0].position = glm::vec3(-3.5f, -1.2f, 0.0f);
	state.enemy[1].position = glm::vec3(3.5f, -1.0f, 0.0f);
	state.enemy[1].acceleration = glm::vec3(0.0f, -9.81f, 0.0f);
	state.enemy[2].position = glm::vec3(-3.0f, 0.0f, 0.0f);
	state.enemy[0].aiType = RUNNER;
	state.enemy[1].aiType = JUMPER;
	state.enemy[2].aiType = FLYING;
	state.enemy[1].height = 1.8f;
	state.enemy[1].width = 0.8f;
	state.enemy[1].height = 1.8f;
	state.enemy[1].width = 0.8f;
	state.enemy[2].height = 1.8f;
	state.enemy[2].width = 0.8f;

	for (int i = 0; i < ENEMY_COUNT; i++) {
		state.enemy[i].entityType = ENEMY;
	}

	state.fire = new Entity[FIRE_COUNT];
}

// Function to add text
void DrawText(ShaderProgram* program, GLuint fontTextureID, std::string text, float size, float spacing, glm::vec3 position)
{
	float width = 1.0f / 16.0f;
	float height = 1.0f / 16.0f;

	std::vector<float> vertices;
	std::vector<float> texCoords;

	for (int i = 0; i < text.size(); i++) {

		int index = (int)text[i];
		float offset = (size + spacing) * i;
		float u = (float)(index % 16) / 16.0f;
		float v = (float)(index / 16) / 16.0f;
		vertices.insert(vertices.end(), {
			 offset + (-0.5f * size), 0.5f * size,
			 offset + (-0.5f * size), -0.5f * size,
			 offset + (0.5f * size), 0.5f * size,
			 offset + (0.5f * size), -0.5f * size,
			 offset + (0.5f * size), 0.5f * size,
			 offset + (-0.5f * size), -0.5f * size,
			});
		texCoords.insert(texCoords.end(), {
		u, v,
		u, v + height,
		u + width, v,
		u + width, v + height,
		u + width, v,
		u, v + height,
			});

	} 

	glm::mat4 modelMatrix = glm::mat4(1.0f);

	modelMatrix = glm::translate(modelMatrix, position);
	program->SetModelMatrix(modelMatrix);

	glUseProgram(program->programID);

	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices.data());
	glEnableVertexAttribArray(program->positionAttribute);

	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords.data());
	glEnableVertexAttribArray(program->texCoordAttribute);

	glBindTexture(GL_TEXTURE_2D, fontTextureID);
	glDrawArrays(GL_TRIANGLES, 0, (int)(text.size() * 6));

	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);
}

void ProcessInput() {

	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_QUIT:
		case SDL_WINDOWEVENT_CLOSE:
			gameIsRunning = false;
			break;

		case SDL_KEYDOWN:
			switch (event.key.keysym.sym) {
			case SDLK_LEFT:
				// Move the player left
				break;

			case SDLK_RIGHT:
				// Move the player right
				break;

			case SDLK_z:
				// Makes the player jump
				if (state.player->canJump == true && state.player->isDead == false) {
					state.player->jump = true;
				}
				break;
			case SDLK_x:
				// Makes the player shoot a fireball
				if ((fireCount < 3 || (fireCount == 3 && state.fire[fireIndex].isActive == false)) && state.player->isDead == false) {
					state.fire[fireIndex].position = state.player->position;
					state.fire[fireIndex].isActive = true;
					state.fire[fireIndex].entityType = FIRE;
					if (state.player->direction == LEFT) {
						state.fire[fireIndex].velocity = glm::vec3(-4.0f, 0.0f, 0);
						state.fire[fireIndex].direction = LEFT;
					}
					else if (state.player->direction == RIGHT) {
						state.fire[fireIndex].velocity = glm::vec3(4.0f, 0.0f, 0);
						state.fire[fireIndex].direction = RIGHT;
					}
					state.fire[fireIndex].textureID = LoadTexture("Fire.png");
					state.fire[fireIndex].height = 1.0f;
					state.fire[fireIndex].width = 1.0f;
					fireIndex += 1;
					if (fireIndex == 3) {
						fireIndex = 0;
					}
					if (fireCount != 3) {
						fireCount += 1;
					}
				}
				
				
				break;
			}
			
			break; // SDL_KEYDOWN
		}
	}

	const Uint8* keys = SDL_GetKeyboardState(NULL);

	// Left and right keys move the player
	if (keys[SDL_SCANCODE_LEFT] && state.player->isDead == false) {
		state.player->velocity.x = -2.0f;
		state.player->direction = LEFT;
	}
	else if (keys[SDL_SCANCODE_RIGHT] && state.player->isDead == false) {
		state.player->velocity.x = 2.0f;
		state.player->direction = RIGHT;
	}
	else {
		state.player->velocity.x = 0.0f;
	}
}

#define FIXED_TIMESTEP 0.0166666f
float lastTicks = 0;
float accumulator = 0.0f;

void Update() {
	float ticks = (float)SDL_GetTicks() / 1000.0f;
	float deltaTime = ticks - lastTicks;
	lastTicks = ticks;

	deltaTime += accumulator;
	if (deltaTime < FIXED_TIMESTEP) {
		accumulator = deltaTime;
		return;
	}

	while (deltaTime >= FIXED_TIMESTEP) {
		state.player->Update(FIXED_TIMESTEP, state.blocks, BLOCK_COUNT, state.enemy, ENEMY_COUNT);

		for (int i = 0; i < ENEMY_COUNT; i++) {
			if (state.enemy[i].aiType == FLYING) {
				state.enemy[i].Update(FIXED_TIMESTEP, state.blocks, BLOCK_COUNT, NULL, 0);
			}
			else if (state.enemy[i].aiType == JUMPER) {
				state.enemy[i].Update(FIXED_TIMESTEP, state.blocks, BLOCK_COUNT, state.fire, FIRE_COUNT);
			}
			else if (state.enemy[i].aiType == RUNNER) {
				state.enemy[i].Update(FIXED_TIMESTEP, state.blocks, BLOCK_COUNT, state.player, 5);
			}
			
		}
		for (int i = 0; i < fireCount; i++) {
			state.fire[i].Update(FIXED_TIMESTEP, state.enemy, ENEMY_COUNT, state.blocks, BLOCK_COUNT);
		}
		
		deltaTime -= FIXED_TIMESTEP;
	}
	if (state.player->isDead == true) {
		state.player->textureID = LoadTexture("PlayerDead.png");
		state.player->height = 1.9f;
	}

	if (state.enemy[0].isDead == true) {
		state.enemy[0].textureID = LoadTexture("Enemy1Dead.png");
	}
	if (state.enemy[1].isDead == true) {
		state.enemy[1].textureID = LoadTexture("Enemy2Dead.png");
	}
	if (state.enemy[2].isDead == true) {
		state.enemy[2].textureID = LoadTexture("Enemy3Dead.png");
	}
	if (state.player->isDead == true || (state.enemy[0].isDead == true && state.enemy[1].isDead == true &&
		state.enemy[2].isDead == true)) {
		restarting = true;
	}
	accumulator = deltaTime;
}


void Render() {
	glClear(GL_COLOR_BUFFER_BIT);

	for (int i = 0; i < BLOCK_COUNT; i++) {
		state.blocks[i].Render(&program);
	}

	for (int i = 0; i < ENEMY_COUNT; i++) {
		state.enemy[i].Render(&program);
	}

	state.player->Render(&program);
	for (int i = 0; i < fireCount; i++) {
		state.fire[i].Render(&program);
	}
	
	
	if (restarting == true) {
		if (state.player->isDead == true) {
			DrawText(&program, LoadTexture("Font.png"), "Game Over", 0.6f, -0.3f, glm::vec3(-1.0f, 0.0f, 0));
		}
		else {
			DrawText(&program, LoadTexture("Font.png"), "You Win!", 0.6f, -0.3f, glm::vec3(-1.0f, 0.0f, 0));
		}
	}
	

	
	SDL_GL_SwapWindow(displayWindow);
}


void Shutdown() {
	SDL_Quit();
}

int main(int argc, char* argv[]) {
	Initialize();

	while (gameIsRunning) {
		ProcessInput();
		Update();
		Render();
	}

	Shutdown();
	return 0;
}
