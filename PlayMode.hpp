#include "Mode.hpp"

#include "Scene.hpp"
#include "WalkMesh.hpp"
#include "Mesh.hpp"
#include "Sound.hpp"

#include <glm/glm.hpp>

#include "TextRenderer.hpp"

#include <vector>
#include <deque>
#include <functional>
#include <memory>

#include "Recipe.hpp"

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up, enter, escape, r_button;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	// Scene camera is fixed, uses blender scene camera.
	Scene::Camera *camera = nullptr;

	//player info:
	struct Player {
		// Member Variables

		WalkPoint at;
		//transform is at player's feet and will be yawed by mouse left/right motion:
		Scene::Transform *transform = nullptr;
		// active ingredients the player is holding.
		Recipe active_recipe;
		// time remaining on player's current action.
		float action_time_left = 0.0f;
		// Player's score.
		int score = 0;

		// Methods

		// Constants
	} player;

	struct GameStat {
		unsigned long target_time = 3000;
		unsigned long curr_time_elapsed = 0;
		unsigned int target_order = 10;
		unsigned int curr_order = 0;
		float curr_score = 0.0f;
		float satisfac = 5.0f; //customer satisfaction 
		bool game_started = false;
		bool game_over;
		unsigned int num_helped = 0;
		unsigned int helped_goal = 15;
		bool playing = false;
		unsigned int curr_lvl = 1;
	}game_stat;

	RecipeQueueSystem recipe_queue_system;

	void try_submit_recipe(Recipe recipe);
	void handle_click(SDL_Event evt);
	void on_click_location(Scene::ClickableLocation *clickableLocation, Scene::Transform *to_move);
	glm::vec3 ray_point_from_screen(int x, int y, GLfloat depth);
	Scene::ClickableLocation *trace_ray(glm::vec3 position, glm::vec3 ray);
	bool bbox_intersect(glm::vec3 pos, glm::vec3 dir, glm::vec3 min, glm::vec3 max);

	// Text Renderer and info
	int windowW;
	int windowH;
	TextRenderer textRenderer;

	// Player's styrofoam container.
	Scene::Drawable *styrofoam_base = nullptr;
	Scene::Drawable *styrofoam_entree = nullptr;
	Scene::Drawable *styrofoam_side_right = nullptr;
	Scene::Drawable *styrofoam_side_left = nullptr;

	// Background sounds tracker (ambient crowd, music?)
	std::shared_ptr< Sound::PlayingSample > crowd_sample = nullptr;
	std::shared_ptr< Sound::PlayingSample > music_sample = nullptr;
};
