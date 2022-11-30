#include "Recipe.hpp"
#include <algorithm>
#include <random>
#include <iostream>
#include <chrono>
#include <cmath>

static const time_t default_time = 20;
static const float const_e = std::exp(1.0f);
RecipeInfo::RecipeInfo() {}
RecipeInfo::RecipeInfo(RecipeInfo * recipe_info) {
	this->create_time = recipe_info->create_time;
	this->expected_time = recipe_info->expected_time;
}
Recipe::Recipe(size_t _max_sides, size_t _max_entrees) :
	max_sides(_max_sides), max_entrees(_max_entrees) {}
Recipe::Recipe(
	std::vector<std::string>_sides, std::vector<std::string>_entrees, RecipeInfo* _recipe_info,
		size_t _max_sides, size_t _max_entrees)
{
	sides = std::vector<std::string>(_sides.begin(), _sides.end());
	entrees = std::vector<std::string>(_entrees.begin(), _entrees.end());
	recipe_info = new RecipeInfo(_recipe_info);
	max_sides = _max_sides;
	max_entrees = _max_entrees;
}

float Recipe::calc_cs() {
	time_t sec_passed = time(NULL) - recipe_info->create_time;
	std::cout << "time passed: " + std::to_string((long)sec_passed) << std::endl;
	int diff = (int)(default_time - sec_passed);
	if (diff < 0) return -1.0f;
	float base = ((float)sec_passed)/ ((float)default_time) * const_e;
	float cs = std::min(0.5f, 0.2f * (- log(base) + 1));
	cs = round(cs * 10) / 10;
	std::cout << "cs change: " + std::to_string(cs) << std::endl;
	return cs;
}

bool Recipe::is_match(Recipe* _recipe) {
	std::vector<std::string> entrees1 = entrees;
	std::vector<std::string> entrees2 = _recipe -> entrees;
	std::sort(entrees1.begin(), entrees1.end());
	std::sort(entrees2.begin(), entrees2.end());
	std::vector<std::string> sides1 = sides;
	std::vector<std::string> sides2 = _recipe -> sides;
	std::sort(sides1.begin(), sides1.end());
	std::sort(sides2.begin(), sides2.end());
	return sides1 == sides2 && entrees1 == entrees2;
}
void Recipe::TryAddSide(std::string side) {
	if (sides.size() < max_sides)
		sides.push_back(side);
}
void Recipe::TryAddEntree(std::string entree) {
	if (entrees.size() < max_entrees)
		entrees.push_back(entree);
}
void RecipeQueueSystem::init(){}
void RecipeQueueSystem::start(long interval){
	std::cerr << "Recipe queue system started" << std::endl;
	auto loop_func = [=]() {
		while (!q_signal){
			if (recipe_queue.size() >= max_queue_size) continue;
			Recipe* recipe = generate_recipe();
			{
				std::unique_lock<std::mutex> q_lock(q_mtx);
				recipe_queue.push_back(recipe);
				std::cerr << "Creating new Recipe" << std::endl;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(interval));
		}
	};
	std::cerr << "lambda defined" << std::endl;
	std::thread t(loop_func);
	t.detach();
}
Recipe * RecipeQueueSystem::generate_recipe() {
	//hardcode for now
	int num_entrees = rand() % 2 + 1;
	std::vector<std::string> sides;
	std::vector<std::string> entrees;
	int ingred_idx = rand() % possible_sides.size();
	sides.push_back(possible_sides[ingred_idx]);
	for (int i = 0; i < num_entrees; ++i) {
		ingred_idx = rand() % possible_entrees.size();
		entrees.push_back(possible_entrees[ingred_idx]);
	}
	RecipeInfo recipe_info; 
	recipe_info.create_time = time(NULL);
	recipe_info.expected_time = default_time;
	Recipe* recipe = new Recipe(sides, entrees, &recipe_info, 100, 100);
	std::cout << "create time: " + std::to_string((int32_t)recipe->recipe_info->create_time) << std::endl;
	return recipe;
}