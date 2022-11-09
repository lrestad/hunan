#include "Recipe.hpp"
#include <random>
#include <iostream>
#include <chrono>

const std::string Recipe::inventory_meshname_map[2][3] = {
	{							// no side
		{"Styrofoam.Base.000"},	// no entrees
		{"Styrofoam.Base.002"}, // one entree
		{"Styrofoam.Base.006"}	// two entrees
	},
	{							// yes side
		{"Styrofoam.Base.001"},	// no entrees
		{"Styrofoam.Base.003"}, // one entree
		{"Styrofoam.Base.007"}	// both entrees
	}
};

RecipeInfo::RecipeInfo() {}
RecipeInfo::RecipeInfo(RecipeInfo * recipe_info) {}
Recipe::Recipe() {};
Recipe::Recipe(
	std::vector<std::string>_sides, std::vector<std::string>_entrees, RecipeInfo* _recipe_info)
{
	sides = std::vector<std::string>(_sides.begin(), _sides.end());
	entrees = std::vector<std::string>(_entrees.begin(), _entrees.end());
	recipe_info = new RecipeInfo(_recipe_info);
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
void Recipe::AddSide(std::string side) {
	sides.push_back(side);
}
void Recipe::AddEntree(std::string entree) {
	entrees.push_back(entree);
}
std::string Recipe::mesh_name_from_recipe(const Recipe &recipe) {
	return Recipe::inventory_meshname_map[recipe.sides.size()][recipe.entrees.size()];
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
	Recipe* recipe = new Recipe(sides, entrees, &recipe_info);
	return recipe;
}