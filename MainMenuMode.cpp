#include "MainMenuMode.hpp"
#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "Mesh.hpp"
#include "data_path.hpp"
#include "load_save_png.hpp"
#include "TextRenderer.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <string>

GLuint mm_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > mm_meshes(LoadTagDefault, []() -> MeshBuffer const * {
    MeshBuffer const *ret = new MeshBuffer(data_path("main-menu.pnct"));
	mm_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< GLuint > hunan_logo_tex(LoadTagDefault, []() {
    return new GLuint(Mode::load_texture(data_path("textures/hunan-logo.png")));
});

Load< Scene > mm_scene(LoadTagDefault, []() -> Scene const * {
    Scene *new_scene = new Scene(data_path("main-menu.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){

        if (mesh_name.find("HunanCube") != std::string::npos) {
            return;
        }
		Mesh const &mesh = mm_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = mm_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;
	});

    return new_scene;
});

MainMenuMode::MainMenuMode() : scene(*mm_scene), 
        bg_sprite(Sprite(data_path("textures/MainMenu-Image.png"))) {
	camera = &scene.cameras.back();

    // create the cube
    {
        scene.transforms.emplace_back();
        Scene::Transform *cube_trans = &scene.transforms.back();
        Mesh const &cube_mesh = mm_meshes->lookup("HunanCube");

        scene.drawables.emplace_back(cube_trans);
        cube = &scene.drawables.back();
        cube->pipeline = lit_color_texture_program_pipeline;
        cube->pipeline.vao = mm_meshes_for_lit_color_texture_program;
        cube->pipeline.type = cube_mesh.type;
        cube->pipeline.start = cube_mesh.start;
        cube->pipeline.count = cube_mesh.count;
        cube->pipeline.textures[0].texture = *hunan_logo_tex;
    }

    textRenderer = TextRenderer("Montserrat-BoldItalic.ttf");
    isEnabled = true;
}

MainMenuMode::~MainMenuMode() {

}

bool MainMenuMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
    if (!isEnabled)
        return false;

    if (evt.type == SDL_MOUSEBUTTONDOWN) {
        Mode::set_current(std::make_shared< PlayMode >());
        // delete(this);
        return true;
    }

    return false;
}

void MainMenuMode::update(float elapsed) {
    if (!isEnabled)
        return;
    // Move cube coolly
    {
        glm::vec3 eulers = glm::eulerAngles(cube->transform->rotation);
        eulers.z += cube_rot_speed * elapsed;
        cube->transform->rotation = glm::quat(eulers);
        total_time += elapsed;
        cube->transform->position.z = cube_amplitude * 
            glm::sin(total_time * glm::pi<float>() * (2 / cube_period)) +
                cube_offset.z;
    }
}

void MainMenuMode::draw(glm::uvec2 const &drawable_size) {
    if (!isEnabled)
        return;
    
    windowW = drawable_size.x;
    windowH = drawable_size.y;

	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

    // Draw background first
    bg_sprite.set_drawable_size(drawable_size);
    bg_sprite.draw(glm::vec2(windowW / 2, windowH / 2), 2.0f);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	// glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	scene.draw(*camera);

    // Render text last.
    {
		textRenderer.render_text("HUNAN", windowW / 8, 3 * windowH / 10, 2.0f, glm::vec3(1.0f, 0.0f, 0.0f));
        textRenderer.render_text("Click to play", 5 * windowW / 16 - 20, windowH / 4, 0.5f, glm::vec3(1.0f, 0.0f, 0.0f));
	}
}
