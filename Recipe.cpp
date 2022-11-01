#include "Recipe.hpp"
#include <random>
#include <iostream>
#include <chrono>
RecipeInfo::RecipeInfo() {}
RecipeInfo::RecipeInfo(RecipeInfo * recipe_info) {}
Recipe::Recipe() {};
Recipe::Recipe(
	std::vector<std::string>_ingredients, RecipeInfo* _recipe_info)
{
	ingredients = std::vector<std::string>(_ingredients.begin(), _ingredients.end());
	recipe_info = new RecipeInfo(_recipe_info);
}
bool Recipe::is_match(Recipe* _recipe) {
	std::vector<std::string> ingreds = ingredients;
	std::vector<std::string> _ingreds = _recipe -> ingredients;
	std::sort(ingreds.begin(), ingreds.end());
	std::sort(_ingreds.begin(), _ingreds.end());
	return ingreds == _ingreds;
}
void Recipe::AddIngredient(std::string ingredient) {
	ingredients.push_back(ingredient);
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
	int num_sides = rand() % 2 + 1;
	int num_entrees = rand() % 2 + 1;
	std::vector<std::string>ingredients;
	for (int i = 0; i < num_sides; ++i) {
		int ingred_idx = rand() % possible_sides.size();
		ingredients.push_back(possible_sides[ingred_idx]);
	}
	for (int i = 0; i < num_entrees; ++i) {
		int ingred_idx = rand() % possible_entrees.size();
		ingredients.push_back(possible_entrees[ingred_idx]);
	}
	RecipeInfo recipe_info; 
	Recipe* recipe = new Recipe(ingredients, &recipe_info);
	return recipe;
}