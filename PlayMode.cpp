#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"
#include "load_save_png.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

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

// From: https://github.com/ixchow/15-466-f18-base3/blob/586f23cf0bbaf80e8e70277442c4e0de7e7612f5/GameMode.cpp#L95-L113
GLuint load_texture(std::string const &filename) {
	glm::uvec2 size;
	std::vector< glm::u8vec4 > data;
	load_png(filename, &size, &data, LowerLeftOrigin);

	GLuint tex = 0;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
	GL_ERRORS();

	return tex;
}

Load< GLuint > rice_tex(LoadTagDefault, [](){
	return new GLuint(load_texture(data_path("textures/rice.png")));
});

Load< GLuint > noodles_tex(LoadTagDefault, [](){
	return new GLuint(load_texture(data_path("textures/noodles.png")));
});

Load< GLuint > chicken_tex(LoadTagDefault, [](){
	return new GLuint(load_texture(data_path("textures/chicken.png")));
});

Load< GLuint > vegetables_tex(LoadTagDefault, [](){
	return new GLuint(load_texture(data_path("textures/vegetables.png")));
});

Load< GLuint > dumplings_tex(LoadTagDefault, [](){
	return new GLuint(load_texture(data_path("textures/dumplings.png")));
});

std::map< std::string, GLuint > ingredient_to_tex;

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
const glm::vec3 base_start_pos = glm::vec3(4.04f, -17.98f, 27.59f);
const glm::vec3 side_pos = glm::vec3(3.59f, -17.86f, 28.23f);
const glm::vec3 entree_right_pos = glm::vec3(4.49f, -18.68f, 27.76f);
const glm::vec3 entree_left_pos = glm::vec3(4.52f, -17.90f, 28.32f);
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

	recipe_system.start(1500);

	textRenderer = TextRenderer("Roboto-Regular.ttf");
}

PlayMode::~PlayMode() {
}

void PlayMode::try_submit_recipe(Recipe recipe) {
	if (recipe_system.recipe_queue.empty()) {
		return;
	}
	if (recipe_system.recipe_queue.front()->is_match(&recipe)) {
		recipe_system.recipe_queue.pop_front();
		player.score++;
	}
	// always delete recipe? not sure if we want that so add warning for now
	std::cout << "Player turned in recipe, removing from inventory." << std::endl;
	player.active_recipe = Recipe(1, 2);
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

	windowW = window_size.x;
	windowH = window_size.y;
	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		} else if (evt.key.keysym.sym == SDLK_a) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.downs += 1;
			down.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_r) {
			r.downs += 1;
			r.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_c) {
			c_button.downs += 1;
			c_button.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_p) {
			p_button.downs += 1;
			p_button.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			recipe_system.recipe_queue.pop_front();
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_r) {
			r.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_c) {
			c_button.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_p) {
			p_button.pressed = false;
			return true;
		}
	} else if (evt.type == SDL_MOUSEBUTTONDOWN) {
		// if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
		// 	SDL_SetRelativeMouseMode(SDL_TRUE);
		// 	return true;
		// }
		handle_click(evt);
	} 
	// else if (evt.type == SDL_MOUSEMOTION) {
	// 	if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
	// 		glm::vec2 motion = glm::vec2(
	// 			evt.motion.xrel / float(window_size.y),
	// 			-evt.motion.yrel / float(window_size.y)
	// 		);
	// 		glm::vec3 upDir = walkmesh->to_world_smooth_normal(player.at);
	// 		player.transform->rotation = glm::angleAxis(-motion.x * camera->fovy, upDir) * player.transform->rotation;

	// 		float pitch = glm::pitch(camera->transform->rotation);
	// 		pitch += motion.y * camera->fovy;
	// 		//camera looks down -z (basically at the player's feet) when pitch is at zero.
	// 		pitch = std::min(pitch, 0.95f * 3.1415926f);
	// 		pitch = std::max(pitch, 0.05f * 3.1415926f);
	// 		camera->transform->rotation = glm::angleAxis(pitch, glm::vec3(1.0f, 0.0f, 0.0f));

	// 		return true;
	// 	}
	// }

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
		clickableLocation->on_click(player.transform);
	} else {
		std::cout << "no clickable detected\n";
	}
}

void PlayMode::update(float elapsed) {
	//access the recipe queue

	
	//player walking:
	{

	// 	//combine inputs into a move:
	// 	constexpr float PlayerSpeed = 3.0f;
	// 	glm::vec2 move = glm::vec2(0.0f);
	// 	if (left.pressed && !right.pressed) move.x =-1.0f;
	// 	if (!left.pressed && right.pressed) move.x = 1.0f;
	// 	if (down.pressed && !up.pressed) move.y =-1.0f;
	// 	if (!down.pressed && up.pressed) move.y = 1.0f;

	// 	//make it so that moving diagonally doesn't go faster:
	// 	if (move != glm::vec2(0.0f)) move = glm::normalize(move) * PlayerSpeed * elapsed;

	// 	//get move in world coordinate system:
	// 	glm::vec3 remain = player.transform->make_local_to_world() * glm::vec4(move.x, move.y, 0.0f, 0.0f);

	// 	//using a for() instead of a while() here so that if walkpoint gets stuck in
	// 	// some awkward case, code will not infinite loop:
	// 	for (uint32_t iter = 0; iter < 10; ++iter) {
	// 		if (remain == glm::vec3(0.0f)) break;
	// 		WalkPoint end;
	// 		float time;
	// 		walkmesh->walk_in_triangle(player.at, remain, &end, &time);
	// 		player.at = end;
	// 		if (time == 1.0f) {
	// 			//finished within triangle:
	// 			remain = glm::vec3(0.0f);
	// 			break;
	// 		}
	// 		//some step remains:
	// 		remain *= (1.0f - time);
	// 		//try to step over edge:
	// 		glm::quat rotation;
	// 		if (walkmesh->cross_edge(player.at, &end, &rotation)) {
	// 			//stepped to a new triangle:
	// 			player.at = end;
	// 			//rotate step to follow surface:
	// 			remain = rotation * remain;
	// 		} else {
	// 			//ran into a wall, bounce / slide along it:
	// 			glm::vec3 const &a = walkmesh->vertices[player.at.indices.x];
	// 			glm::vec3 const &b = walkmesh->vertices[player.at.indices.y];
	// 			glm::vec3 const &c = walkmesh->vertices[player.at.indices.z];
	// 			glm::vec3 along = glm::normalize(b-a);
	// 			glm::vec3 normal = glm::normalize(glm::cross(b-a, c-a));
	// 			glm::vec3 in = glm::cross(normal, along);

	// 			//check how much 'remain' is pointing out of the triangle:
	// 			float d = glm::dot(remain, in);
	// 			if (d < 0.0f) {
	// 				//bounce off of the wall:
	// 				remain += (-1.25f * d) * in;
	// 			} else {
	// 				//if it's just pointing along the edge, bend slightly away from wall:
	// 				remain += 0.01f * d * in;
	// 			}
	// 		}
	// 	}

	// 	if (remain != glm::vec3(0.0f)) {
	// 		std::cout << "NOTE: code used full iteration budget for walking." << std::endl;
	// 	}

	// 	//update player's position to respect walking:
	// 	player.transform->position = walkmesh->to_world_point(player.at);

	// 	{ //update player's rotation to respect local (smooth) up-vector:
			
	// 		glm::quat adjust = glm::rotation(
	// 			player.transform->rotation * glm::vec3(0.0f, 0.0f, 1.0f), //current up vector
	// 			walkmesh->to_world_smooth_normal(player.at) //smoothed up vector at walk location
	// 		);
	// 		player.transform->rotation = glm::normalize(adjust * player.transform->rotation);
	// 	}

	// 	/*
	// 	glm::mat4x3 frame = camera->transform->make_local_to_parent();
	// 	glm::vec3 right = frame[0];
	// 	//glm::vec3 up = frame[1];
	// 	glm::vec3 forward = -frame[2];

	// 	camera->transform->position += move.x * right + move.y * forward;
	// 	*/
	}

	// Update player's inventory drawables positions
	// {
	// 	switch (player.active_recipe.sides.size()) {
	// 	case 0:
	// 		styrofoam_side->transform->position.x = 100.0f;
	// 		break;
	// 	case 1:
	// 		// styrofoam_side->transform->position = styrofoam_side->transform->make_local_to_world() * glm::vec4(side_pos, 1.0f);
	// 		// styrofoam_side->transform->position = styrofoam_base->transform->make_local_to_world() * glm::vec4(side_pos, 1.0f);
	// 		styrofoam_side->transform->position = side_pos;
	// 		break;
	// 	default:
	// 		std::cout << "Invalid active recipe sides size: " << player.active_recipe.sides.size() << "\n";
	// 	}

	// 	switch (player.active_recipe.entrees.size()) {
	// 	case 0:
	// 		styrofoam_entree_right->transform->position.x = 100.0f;
	// 		styrofoam_entree_left->transform->position.x = 100.0f;
	// 		break;
	// 	case 1:
	// 		styrofoam_entree_right->transform->position = entree_right_pos;
	// 		styrofoam_entree_left->transform->position.x = 100.0f;
	// 		break;
	// 	case 2:
	// 		styrofoam_entree_right->transform->position = entree_right_pos;
	// 		styrofoam_entree_left->transform->position = entree_left_pos;
	// 		break;
	// 	default:
	// 		std::cout << "Invalid active recipe entrees size: " << player.active_recipe.entrees.size() << "\n";
	// 	}
	// }

	// debug stuff
	if (r.pressed && r.downs == 1) {
		player.active_recipe.TryAddSide("noodles");
	}
	if (c_button.pressed && c_button.downs == 1) {
		player.active_recipe.TryAddEntree("veggies");
	}
	if (p_button.pressed) {
		try_submit_recipe(player.active_recipe);
	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
	r.downs = 0;
	c_button.downs = 0;
	p_button.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
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

	/* In case you are wondering if your walkmesh is lining up with your scene, try:
	{
		glDisable(GL_DEPTH_TEST);
		DrawLines lines(player.camera->make_projection() * glm::mat4(player.camera->transform->make_world_to_local()));
		for (auto const &tri : walkmesh->triangles) {
			lines.draw(walkmesh->vertices[tri.x], walkmesh->vertices[tri.y], glm::u8vec4(0x88, 0x00, 0xff, 0xff));
			lines.draw(walkmesh->vertices[tri.y], walkmesh->vertices[tri.z], glm::u8vec4(0x88, 0x00, 0xff, 0xff));
			lines.draw(walkmesh->vertices[tri.z], walkmesh->vertices[tri.x], glm::u8vec4(0x88, 0x00, 0xff, 0xff));
		}
	}
	*/

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.09f;
		lines.draw_text("Mouse motion looks; WASD moves; escape ungrabs mouse",
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text("Mouse motion looks; WASD moves; escape ungrabs mouse",
			glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + + 0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
	}
	// Print recipes
	{
		// recipes to complete
		std::unique_lock<std::mutex>q_lock(recipe_system.q_mtx);
		int cnt = 1;
		for (Recipe* recipe : recipe_system.recipe_queue) {
			std::string display_text = "Recipe " + std::to_string(cnt++) + ": ";

			for (std::string ingred : recipe->sides) {
				display_text += ingred + ", ";
			}
			for (std::string ingred : recipe->entrees) {
				display_text += ingred + ", ";
			}
			textRenderer.render_text(display_text, (float)0, (float)windowH - cnt * 20.0f, 0.5f, glm::vec3(1.0f));
		}

		// player active recipe
		textRenderer.render_text("inventory:", windowW - 160.0f, windowH - 40.0f, 0.5f, glm::vec3(1.0f));
		cnt = 3;
		for (std::string ingred : player.active_recipe.sides) {
			textRenderer.render_text(ingred, windowW - 160.0f, windowH - cnt * 20.0f, 0.5f, glm::vec3(1.0f));
			cnt++;
		}
		for (std::string ingred : player.active_recipe.entrees) {
			textRenderer.render_text(ingred, windowW - 160.0f, windowH - cnt * 20.0f, 0.5f, glm::vec3(1.0f));
			cnt++;
		}
	}
	// Draw score
	{
		std::string score_text = "Score: " + std::to_string(player.score);
		textRenderer.render_text(score_text, windowW - 240.0f, 40.0f, 1.0f, glm::vec3(1.0f));
	}
	// Update player recipe textures
	{
		GLuint side_tex_id, entree_tex_id, entree_right_tex_id, entree_left_tex_id;
		switch (player.active_recipe.sides.size()) {
		case 0:
			// Shouldn't even be in view.
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
			// Shouldn't even be in view.
			break;
		case 1:
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
	GL_ERRORS();
}
