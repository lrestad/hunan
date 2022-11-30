#include "Mode.hpp"

#include "Scene.hpp"
#include "TextRenderer.hpp"

#include <glm/glm.hpp>

struct MainMenuMode : Mode {
    MainMenuMode();
    virtual ~MainMenuMode();

    bool isEnabled = true;

    //functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

    //local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	// Scene camera is fixed, uses blender scene camera.
	Scene::Camera *camera = nullptr;

    Scene::Drawable *cube = nullptr;
    float total_time = 0.0f;
    float cube_rot_speed = 0.5f;
    float cube_amplitude = 1.0f;
    float cube_period = 4.0f;

    TextRenderer textRenderer;
    int windowW;
    int windowH;
};
