#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"
#include "load_save_png.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"
#include "Sound.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include <map>
#include <random>
#include <functional>

bool end_game = false;

GLuint counter_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > counter_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("counter-level.pnct"));
	counter_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

// temporary empty texture
Load< GLuint > empty_tex(LoadTagDefault, []() {
	GLuint tex;
	glGenTextures(1, &tex);

	glBindTexture(GL_TEXTURE_2D, tex);
	std::vector< glm::u8vec4 > tex_data(1, glm::u8vec4(0xff));
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex_data.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);
	return new GLuint(tex);
});

Load< GLuint > rice_tex(LoadTagDefault, [](){
	return new GLuint(Mode::load_texture(data_path("textures/rice.png")));
});

Load< GLuint > noodles_tex(LoadTagDefault, [](){
	return new GLuint(Mode::load_texture(data_path("textures/noodles.png")));
});

Load< GLuint > chicken_tex(LoadTagDefault, [](){
	return new GLuint(Mode::load_texture(data_path("textures/chicken.png")));
});

Load< GLuint > vegetables_tex(LoadTagDefault, [](){
	return new GLuint(Mode::load_texture(data_path("textures/vegetables.png")));
});

Load< GLuint > dumplings_tex(LoadTagDefault, [](){
	return new GLuint(Mode::load_texture(data_path("textures/dumplings.png")));
});

Load< GLuint > cash_register_tex(LoadTagDefault, [](){
	return new GLuint(Mode::load_texture(data_path("textures/CashRegister.png")));
});

Load< Sound::Sample > cha_ching_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("sounds/cha-ching.wav"));
});

Load< Sound::Sample > incorrect_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("sounds/incorrect.wav"));
});

Load< Sound::Sample > crowd_sample_(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("sounds/crowd-noise.wav"));
});

Load< Sound::Sample > music_sample_(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("sounds/hunan-rap.wav"));
});

std::map< std::string, GLuint > ingredient_to_tex;
enum simplified_clickable_name {
	rice, noodles, chicken, dumpling, veggies, checkout, neutral, styrofoam
};
std::map< std::string, simplified_clickable_name > clickable_location_name_to_simplified;

Load< Scene > counter_scene(LoadTagDefault, []() -> Scene const * {

	Scene *new_scene = new Scene(data_path("counter-level.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = counter_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = counter_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

		// std::printf("mesh_name: %s\n", mesh_name.c_str());
		if (mesh_name.find("Clickable") != std::string::npos) {
			// Scene::Clickable clickable(transform, mesh.min, mesh.max,);
			std::cout << "Adding clickable " << mesh_name << std::endl;

			scene.clickableLocations.emplace_back(transform, mesh.min, mesh.max, false);
		} else if (mesh_name.find("Cash Register") != std::string::npos) {
			drawable.pipeline.textures[0].texture = *cash_register_tex;
		}
		// else if (mesh_name.find("Styrofoam.Base.0") != std::string::npos) {
		// 	std::cout << "Adding styrofoam " << mesh_name << std::endl;
		// 	scene.mesh_name_to_drawables_idx[mesh_name] = 
		// 		scene.drawables.size() - 1;

		// 	// drawable.pipeline.textures[0].texture = tex;
		// 	// drawable.pipeline.textures[0].target = GL_TEXTURE_2D;
		// }
	});

	return new_scene;
});

WalkMesh const *walkmesh = nullptr;
Load< WalkMeshes > counter_walkmeshes(LoadTagDefault, []() -> WalkMeshes const * {
	WalkMeshes *ret = new WalkMeshes(data_path("counter-level.w"));
	walkmesh = &ret->lookup("WalkMesh");
	return ret;
});

// Quaternion lookat function adapted from Dakota's game3, originally
// taken from https://stackoverflow.com/a/49824672
glm::quat safe_quat_lookat(glm::vec3 const &fromPos, glm::vec3 const &toPos, 
		glm::vec3 const &rotateAround = glm::vec3(0.0f, 0.0f, 1.0f)) {
	glm::vec3 dir = toPos - fromPos;
	float dir_len = glm::length(dir);

	// Make sure dir is valid
	if (!(dir_len > 0.0001f)) {
		return glm::quat(1, 0, 0, 0); // identity
	}

	dir /= dir_len;
	
	// quatLookAt requires that dir is *not* parallel to the axis to rotate around.
	if (glm::abs(glm::dot(dir, rotateAround)) > 0.9999f) {
		return glm::quat(1, 0, 0, 0); // identity (I don't expect this to happen tbh)
	}
	return glm::quatLookAt(dir, rotateAround);
}

// This is a prety bad way to do things but it works for now
const glm::vec3 player_start_pos = glm::vec3(0, -8.9f, 3.5f);
const glm::vec3 hat_start_pos = glm::vec3(0, 0, 3.5f);
const glm::vec3 base_start_pos = glm::vec3(4.04f, -18.16f, 27.8f);
const glm::vec3 side_pos = glm::vec3(3.5f, -18.1f, 28.5f);
const glm::vec3 entree_right_pos = glm::vec3(4.49f, -18.8f, 27.9f);
const glm::vec3 entree_left_pos = glm::vec3(4.52f, -18.0f, 28.4f);
PlayMode::PlayMode() : scene(*counter_scene) {
	//create a player and hat transform:
	{
		scene.transforms.emplace_back();
		Scene::Transform *player_transform = &scene.transforms.back();
		player.transform = player_transform;
		player.transform->position = player_start_pos;
		Mesh const &player_mesh = counter_meshes->lookup("Player.Capsule");

		scene.drawables.emplace_back(player_transform);
		Scene::Drawable &player_drawable = scene.drawables.back();

		player_drawable.pipeline = lit_color_texture_program_pipeline;

		player_drawable.pipeline.vao = counter_meshes_for_lit_color_texture_program;
		player_drawable.pipeline.type = player_mesh.type;
		player_drawable.pipeline.start = player_mesh.start;
		player_drawable.pipeline.count = player_mesh.count;
		
		// hat
		scene.transforms.emplace_back();
		Scene::Transform *hat_transform = &scene.transforms.back();
		hat_transform->parent = player_transform;
		hat_transform->position = hat_start_pos;
		Mesh const &hat_mesh = counter_meshes->lookup("Player.Hat");

		scene.drawables.emplace_back(hat_transform);
		Scene::Drawable &hat_drawable = scene.drawables.back();

		hat_drawable.pipeline = lit_color_texture_program_pipeline;

		hat_drawable.pipeline.vao = counter_meshes_for_lit_color_texture_program;
		hat_drawable.pipeline.type = hat_mesh.type;
		hat_drawable.pipeline.start = hat_mesh.start;
		hat_drawable.pipeline.count = hat_mesh.count;
	}

	// temporary: spawn styrofoam in styrofoam start location and then move to the side
	{
		Mesh const &base_mesh = counter_meshes->lookup("Styrofoam.Base");
		Mesh const &side_mesh = counter_meshes->lookup("Styrofoam.Side");
		Mesh const &entree_right_mesh = counter_meshes->lookup("Styrofoam.Entree.Right");
		Mesh const &entree_left_mesh = counter_meshes->lookup("Styrofoam.Entree.Left");

		scene.transforms.emplace_back();
		Scene::Transform *base_transform = &scene.transforms.back();
		scene.transforms.emplace_back();
		Scene::Transform *side_transform = &scene.transforms.back();
		scene.transforms.emplace_back();
		Scene::Transform *entree_right_transform = &scene.transforms.back();
		scene.transforms.emplace_back();
		Scene::Transform *entree_left_transform = &scene.transforms.back();
		base_transform->position = base_start_pos;
		side_transform->position = side_pos;
		entree_right_transform->position = entree_right_pos;
		entree_left_transform->position = entree_left_pos;
		// temporary: move off-screen. might not need this at all tbh.
		// base_transform->position.x = 100.0f;

		scene.drawables.emplace_back(base_transform);
		styrofoam_base = &scene.drawables.back();
		styrofoam_base->pipeline = lit_color_texture_program_pipeline;
		styrofoam_base->pipeline.vao = counter_meshes_for_lit_color_texture_program;
		styrofoam_base->pipeline.type = base_mesh.type;
		styrofoam_base->pipeline.start = base_mesh.start;
		styrofoam_base->pipeline.count = base_mesh.count;

		scene.drawables.emplace_back(side_transform);
		styrofoam_side = &scene.drawables.back();
		styrofoam_side->pipeline = lit_color_texture_program_pipeline;
		styrofoam_side->pipeline.vao = counter_meshes_for_lit_color_texture_program;
		styrofoam_side->pipeline.type = side_mesh.type;
		styrofoam_side->pipeline.start = side_mesh.start;
		styrofoam_side->pipeline.count = side_mesh.count;

		scene.drawables.emplace_back(entree_right_transform);
		styrofoam_entree_right = &scene.drawables.back();
		styrofoam_entree_right->pipeline = lit_color_texture_program_pipeline;
		styrofoam_entree_right->pipeline.vao = counter_meshes_for_lit_color_texture_program;
		styrofoam_entree_right->pipeline.type = entree_right_mesh.type;
		styrofoam_entree_right->pipeline.start = entree_right_mesh.start;
		styrofoam_entree_right->pipeline.count = entree_right_mesh.count;

		scene.drawables.emplace_back(entree_left_transform);
		styrofoam_entree_left = &scene.drawables.back();
		styrofoam_entree_left->pipeline = lit_color_texture_program_pipeline;
		styrofoam_entree_left->pipeline.vao = counter_meshes_for_lit_color_texture_program;
		styrofoam_entree_left->pipeline.type = entree_left_mesh.type;
		styrofoam_entree_left->pipeline.start = entree_left_mesh.start;
		styrofoam_entree_left->pipeline.count = entree_left_mesh.count;
	}

	// Create map from ingredients to textures
	{
		ingredient_to_tex = {
			{"rice", *rice_tex},
			{"noodles", *noodles_tex},
			{"chicken", *chicken_tex},
			{"dumpling", *dumplings_tex},
			{"veggies", *vegetables_tex}
		};
	}

	// Create map from clickable location names to textures
	{
		clickable_location_name_to_simplified = {
			{"Clickable.Styrofoam", styrofoam},
			{"Clickable.Chicken", chicken},
			{"Clickable.Checkout.001", checkout},
			{"Clickable.Checkout.002", checkout},
			{"Clickable.Rice", rice},
			{"Clickable.Noodles", noodles},
			{"Clickable.Vegetables", veggies},
			{"Clickable.Dumplings", dumpling},
			{"Clickable.Neutral", neutral}
		};
	}


	//create a player camera attached to a child of the player transform:
	// scene.transforms.emplace_back();
	// scene.cameras.emplace_back(&scene.transforms.back());
	camera = &scene.cameras.back();
	// camera->fovy = glm::radians(60.0f);
	// camera->near = 0.01f;
	// camera->transform->parent = player.transform;

	//player's eyes are 1.8 units above the ground:
	// player.camera->transform->position = glm::vec3(0.0f, 0.0f, 10.0f);

	//rotate camera facing direction (-z) to player facing direction (+y):
	// player.camera->transform->rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

	//start player walking at nearest walk point:
	// player.at = walkmesh->nearest_walk_point(player.transform->position);


	textRenderer = TextRenderer("Roboto-Regular.ttf");
}

PlayMode::~PlayMode() {
}

void PlayMode::try_submit_recipe(Recipe recipe) {
	if (recipe_queue_system.recipe_queue.empty()) {
		return;
	}
	if (recipe_queue_system.recipe_queue.front()->is_match(&recipe)) {

		recipe_queue_system.recipe_queue.pop_front();
		game_stat.satisfac += 0.1f;
		game_stat.satisfac = std::min(game_stat.satisfac, 5.0f);
		player.score++;
		game_stat.num_helped++;
		Sound::play(*cha_ching_sample, 0.8f, 0.0f);
	}
	else {
		std::cerr << "Recipe does not match!" <<  std::endl;
		game_stat.satisfac -= 0.5f;
		game_stat.satisfac = std::max(game_stat.satisfac, 0.0f);
		Sound::play(*incorrect_sample, 0.8f, 0.0f);
		return;
	}
	// always delete recipe? not sure if we want that so add warning for now
	std::cout << "Player turned in recipe, removing from inventory." << std::endl;
	player.active_recipe = Recipe();
}

// Original implementation was to get world space coord from screen coord. Now
// just gets some point on the ray cuz idk why it's not working.
glm::vec3 PlayMode::ray_point_from_screen(int x, int y, GLfloat depth) {
	// get world space coordinates of point at cursor location
	glm::mat4 projection = camera->make_projection();
	glm::mat4 view = camera->transform->make_world_to_local();
	glm::vec4 viewport = glm::vec4(0, 0, windowW, windowH);
	glm::vec3 wincoord = glm::vec3(x, windowH - y - 1, depth);
	glm::vec3 worldcoord = glm::unProject(wincoord, view, projection, viewport);
	std::printf("Method 1: %f, %f, %f\n", worldcoord.x, worldcoord.y, worldcoord.z);
	// glm::vec4 viewport = glm::vec4(0.0f, 0.0f, windowW, windowH);
	// glm::vec3 camerapos = camera->transform->position;
	// glm::vec3 camera_angle = glm::eulerAngles(camera->transform->rotation);
    // glm::mat4 tmpView = glm::lookAt(camerapos,
    //                                 camera_angle,glm::vec3(0,1,0)); // up == z?
    // glm::mat4 tmpProj = glm::perspective( 90.0f, (float)(windowW/windowH), 0.1f, 1000.0f);
    // glm::vec3 screenPos = glm::vec3(x, windowH-y - 1, 0.0f);

    // glm::vec3 worldcoord = glm::unProject(screenPos, tmpView, tmpProj, viewport);
	// std::printf("Method 2: %f, %f, %f\n", worldcoord.x, worldcoord.y, worldcoord.z);
	return worldcoord;
}

bool PlayMode::bbox_intersect(glm::vec3 pos, glm::vec3 dir, glm::vec3 min, glm::vec3 max) {
    float tmin = (min.x - pos.x) / dir.x; 
    float tmax = (max.x - pos.x) / dir.x; 
 
    if (tmin > tmax) std::swap(tmin, tmax); 
 
    float tymin = (min.y - pos.y) / dir.y; 
    float tymax = (max.y - pos.y) / dir.y; 
 
    if (tymin > tymax) std::swap(tymin, tymax); 
 
    if ((tmin > tymax) || (tymin > tmax)) 
        return false; 
 
    if (tymin > tmin) 
        tmin = tymin; 
 
    if (tymax < tmax) 
        tmax = tymax; 
 
    float tzmin = (min.z - pos.z) / dir.z; 
    float tzmax = (max.z - pos.z) / dir.z; 
 
    if (tzmin > tzmax) std::swap(tzmin, tzmax); 
 
    if ((tmin > tzmax) || (tzmin > tmax)) 
        return false; 
 
    if (tzmin > tmin) 
        tmin = tzmin; 
 
    if (tzmax < tmax) 
        tmax = tzmax; 
 
    return true; 
}

Scene::ClickableLocation *PlayMode::trace_ray(glm::vec3 position, glm::vec3 ray) {
	// std::printf("length of scene.clickables: %lu\n", scene.clic.size());
	for (auto &it : scene.clickableLocations) {
		Scene::Transform *clickable_trans = it.transform;
		glm::vec3 trans_min = clickable_trans->make_local_to_world() * glm::vec4(it.min, 1.f);
		glm::vec3 trans_max = clickable_trans->make_local_to_world() * glm::vec4(it.max, 1.f);
		std::printf("transformed min: %f, %f, %f. transformed max: %f, %f, %f\n",
			trans_min.x, trans_min.y, trans_min.z, trans_max.x, trans_max.y, trans_max.z);
		if(bbox_intersect(position, ray, trans_min, trans_max)) {
			return &it;
		}
	}
	return nullptr;
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
	
	if (end_game) {
		return false;
	}
	if (game_stat.satisfac <= 0) {
		return false;
	}
	windowW = window_size.x;
	windowH = window_size.y;
	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_SPACE) {
			recipe_queue_system.recipe_queue.pop_front();
			return true;
		}
	} else if (evt.type == SDL_MOUSEBUTTONDOWN) {
		if (!game_stat.playing) {
			game_stat.playing = true;
		} else {
			handle_click(evt);
		}
	} 

	return false;
}

void PlayMode::handle_click(SDL_Event evt) {
	int x = evt.button.x;
	int y = evt.button.y;
	GLfloat depth;
	glReadPixels(x, y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
	glm::vec3 ray_point = ray_point_from_screen(x, y, depth);
	// Maybe swap order of these?
	// ASSUME CAMERA HAS NO PARENT
	// For some reason (I think it's the rotation component) applying the
	// make_local_to_world() matrix messes up the position.
	// glm::vec4 camera_pos_homo = glm::vec4(camera->transform->position, 1.0f);
	std::printf("camera->transform->scale: %f, %f, %f\n", camera->transform->scale.x, camera->transform->scale.y, camera->transform->scale.z);
	// glm::vec3 camera_world_pos = camera->transform->make_local_to_world() * camera_pos_homo;
	std::printf("camera local pos: %f, %f, %f\n", camera->transform->position.x, camera->transform->position.y, camera->transform->position.z);
	// std::printf("camera_world_pos: %f, %f, %f\n", camera_world_pos.x, camera_world_pos.y, camera_world_pos.z);
	std::printf("point: %f, %f, %f\n", ray_point.x, ray_point.y, ray_point.z);
	// glm::vec3 ray = glm::normalize(ray_point - camera_world_pos);
	glm::vec3 ray = glm::normalize(ray_point - camera->transform->position);
	std::printf("ray: %f, %f, %f\n", ray.x, ray.y, ray.z);
	Scene::ClickableLocation *clickableLocation = trace_ray(camera->transform->position, ray);
	if (clickableLocation != nullptr) {
		on_click_location(clickableLocation, player.transform);
	} else {
		std::cout << "no clickable detected\n";
	}
}

void PlayMode::on_click_location(Scene::ClickableLocation *clickableLocation, Scene::Transform *to_move) {
	if (clickableLocation == nullptr)
		return;
	std::printf("Clicked on %s\n", clickableLocation->transform->name.c_str());
	to_move->position.x = clickableLocation->transform->position.x;
	to_move->position.y = clickableLocation->transform->position.y;
	if (clickableLocation->update_z)
		to_move->position.z = clickableLocation->transform->position.z;
	// add rotating?
	simplified_clickable_name simplified = 
		clickable_location_name_to_simplified[clickableLocation->transform->name];
	switch (simplified) {
	case rice:
		player.active_recipe.TryAddSide("rice");
		break;
	case noodles:
		player.active_recipe.TryAddSide("noodles");
		break;
	case chicken:
		player.active_recipe.TryAddEntree("chicken");
		break;
	case dumpling:
		player.active_recipe.TryAddSide("dumpling");
		break;
	case veggies:
		player.active_recipe.TryAddEntree("veggies");
		break;
	case checkout:
		try_submit_recipe(player.active_recipe);
		break;
	case styrofoam:
		player.active_recipe = Recipe();
		break;
	default:
		// No action needed
		break;
	}
}

void PlayMode::update(float elapsed) {

	// If not currently playing, ignore time
	//! TODO: make each instruction screen its own mode
	if (!game_stat.playing) {
		if (crowd_sample != nullptr) {
			crowd_sample->stop();
			music_sample->stop();
		}
		crowd_sample = nullptr;
		music_sample = nullptr;
		return;
	} else if (crowd_sample == nullptr) {
		// Start looping sounds
		crowd_sample = Sound::loop(*crowd_sample_, 0.05f);
		music_sample = Sound::loop(*music_sample_, 0.75f);
	}

	// Update satisfaction
	if (game_stat.curr_time_elapsed / 5000 < (game_stat.curr_time_elapsed + elapsed) / 5000) {
		game_stat.satisfac -= recipe_queue_system.recipe_queue.size() * 0.001;
	}

	// If satisfaction is too low, end game
	//! TODO: make 'game over' its own mode
	if (game_stat.satisfac <= 0) {
		if (game_stat.curr_lvl < 3) {
			std::printf("game over, stars: %d", game_stat.curr_lvl);
		} else {
			std::printf("game over, level: %d, time: %lu", 3, game_stat.curr_time_elapsed);
		}
		return;
	}

	// If finished with level 1, move to level 2 and reset stats
	if (game_stat.curr_lvl == 1 && game_stat.num_helped >= 15) {
		game_stat.curr_lvl = 2;
		game_stat.curr_time_elapsed = 0;
		game_stat.num_helped = 0;
		game_stat.playing = false;
		game_stat.satisfac = 5.0f;
		//! TODO: clear order queue

	// If finished with level 2, move to level 3 and reset stats
	} else if (game_stat.curr_lvl == 2 && game_stat.num_helped >= 20) {
		game_stat.curr_lvl = 3;
		game_stat.curr_time_elapsed = 0;
		game_stat.num_helped = 0;
		game_stat.playing = false;
		game_stat.satisfac = 5.0f;
		//! TODO: clear order queue
	}

	// Update order queue
	recipe_queue_system.generate_order((unsigned int)game_stat.curr_lvl, (unsigned int)game_stat.curr_time_elapsed, (unsigned int)elapsed);

	// Update level total time
	game_stat.curr_time_elapsed += elapsed;
}

std::string float_to_string(float f, size_t precision) {
	char buffer[20];
	std::snprintf(buffer, 20, "%.1f", f);
	std::string str(buffer);
	return str;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	
	if (!game_stat.playing) {
		//set up light type and position for lit_color_texture_program:
		glUseProgram(lit_color_texture_program->program);
		glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
		glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
		glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
		glUseProgram(0);

		glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
		glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

		// Write level instructions
		if (game_stat.curr_lvl == 1) {
			std::string lvl_1_text = "Level 1";
			textRenderer.render_text(lvl_1_text, 150.0f, windowH - 150.0f, 0.5f, glm::vec3(0.0f, 0.0f, 0.0f));

			std::string lvl_1_instructions_0 = "Every order will be a valid order with:";
			std::string lvl_1_instructions_1 = "    1 Entree - Chicken";
			std::string lvl_1_instructions_2 = "    0-2 Sides - Rice";
			textRenderer.render_text(lvl_1_instructions_0, 150.0f, windowH - 250.0f, 0.25f, glm::vec3(0.0f, 0.0f, 0.0f));
			textRenderer.render_text(lvl_1_instructions_1, 150.0f, windowH - 300.0f, 0.25f, glm::vec3(0.0f, 0.0f, 0.0f));
			textRenderer.render_text(lvl_1_instructions_2, 150.0f, windowH - 350.0f, 0.25f, glm::vec3(0.0f, 0.0f, 0.0f));
		} else if (game_stat.curr_lvl == 2) {
			std::string lvl_2_text = "Level 2";
			textRenderer.render_text(lvl_2_text, 150.0f, windowH - 150.0f, 0.5f, glm::vec3(0.0f, 0.0f, 0.0f));

			std::string lvl_2_instructions_0 = "Every order will be a valid order with:";
			std::string lvl_2_instructions_1 = "    1 Entree - Chicken, Dupling, Veggies";
			std::string lvl_2_instructions_2 = "    0-2 Sides - Rice, Noodles";
			textRenderer.render_text(lvl_2_instructions_0, 150.0f, windowH - 250.0f, 0.25f, glm::vec3(0.0f, 0.0f, 0.0f));
			textRenderer.render_text(lvl_2_instructions_1, 150.0f, windowH - 300.0f, 0.25f, glm::vec3(0.0f, 0.0f, 0.0f));
			textRenderer.render_text(lvl_2_instructions_2, 150.0f, windowH - 350.0f, 0.25f, glm::vec3(0.0f, 0.0f, 0.0f));
		} else {
			std::string lvl_3_text = "Level 3";
			textRenderer.render_text(lvl_3_text, 150.0f, windowH - 150.0f, 0.5f, glm::vec3(0.0f, 0.0f, 0.0f));

			std::string lvl_3_instructions_0 = "Valid orders should have:";
			std::string lvl_3_instructions_1 = "    1 Entree - Chicken, Dupling, Veggies";
			std::string lvl_3_instructions_2 = "    0-2 Sides - Rice, Noodles";
			std::string lvl_3_instructions_3 = "Invalid orders should recieve empty trays.";
			textRenderer.render_text(lvl_3_instructions_0, 150.0f, windowH - 250.0f, 0.25f, glm::vec3(0.0f, 0.0f, 0.0f));
			textRenderer.render_text(lvl_3_instructions_1, 150.0f, windowH - 300.0f, 0.25f, glm::vec3(0.0f, 0.0f, 0.0f));
			textRenderer.render_text(lvl_3_instructions_2, 150.0f, windowH - 350.0f, 0.25f, glm::vec3(0.0f, 0.0f, 0.0f));
			textRenderer.render_text(lvl_3_instructions_3, 150.0f, windowH - 400.0f, 0.25f, glm::vec3(0.0f, 0.0f, 0.0f));
		}
		
		return;
	}

	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	scene.draw(*camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));
	}
	// Print recipes
	{
		// recipes to complete
		int cnt = 1;
		for (Recipe* recipe : recipe_queue_system.recipe_queue) {
			std::string display_text = "Recipe " + std::to_string(cnt++) + ": ";

			for (std::string ingred : recipe->sides) {
				display_text += ingred + ", ";
			}
			for (std::string ingred : recipe->entrees) {
				display_text += ingred + ", ";
			}
			textRenderer.render_text(display_text, (float)20, (float)windowH - cnt * 20.0f, 0.25f, glm::vec3(0.0f));
		}
	}

	// Draw satisfacation
	{
		std::string satf_text = "Customer Satisfaction: " + float_to_string(game_stat.satisfac, 1);
		textRenderer.render_text(satf_text, 20.0f, 40.0f, .35f, glm::vec3(0.0f));
	}

	// Update player recipe textures
	{
		GLuint side_tex_id, entree_tex_id, entree_right_tex_id, entree_left_tex_id;
		switch (player.active_recipe.sides.size()) {
		case 0:
			styrofoam_side->pipeline.textures[0].texture = *empty_tex;
			break;
		case 1:
			side_tex_id = ingredient_to_tex[player.active_recipe.sides[0]];
			styrofoam_side->pipeline.textures[0].texture = side_tex_id;
			break;
		default:
			std::cout << "Invalid active recipe sides size: " << player.active_recipe.sides.size() << "\n";
		}

		switch (player.active_recipe.entrees.size()) {
		case 0:
			styrofoam_entree_right->pipeline.textures[0].texture = *empty_tex;
			styrofoam_entree_left->pipeline.textures[0].texture = *empty_tex;
			break;
		case 1:
			styrofoam_entree_left->pipeline.textures[0].texture = *empty_tex;
			entree_tex_id = ingredient_to_tex[player.active_recipe.entrees[0]];
			styrofoam_entree_right->pipeline.textures[0].texture = entree_tex_id;
			break;
		case 2:
			entree_right_tex_id = ingredient_to_tex[player.active_recipe.entrees[0]];
			styrofoam_entree_right->pipeline.textures[0].texture = entree_right_tex_id;
			entree_left_tex_id = ingredient_to_tex[player.active_recipe.entrees[1]];
			styrofoam_entree_left->pipeline.textures[0].texture = entree_left_tex_id;
			break;
		default:
			std::cout << "Invalid active recipe entrees size: " << player.active_recipe.entrees.size() << "\n";
		}
	}
	if (game_stat.satisfac <= 0) {
		std::string end_text = "GAME OVER";
		textRenderer.render_text(end_text, windowW / 2 - 240.0f, windowH /2 - .0f, 1.0f, glm::vec3(.8f, .2f, .2f));
		return;
	}
	GL_ERRORS();
}
