#include "Recipe.hpp"
#include <random>
#include <iostream>
#include <chrono>
RecipeInfo::RecipeInfo() {}
RecipeInfo::RecipeInfo(RecipeInfo * recipe_info) {}
Recipe::Recipe(
	std::vector<std::string>_ingredients, RecipeInfo* _recipe_info)
{
	ingredients = std::vector<std::string>(_ingredients.begin(), _ingredients.end());
	recipe_info = new RecipeInfo(_recipe_info);
}
void RecipeQueueSystem::init(){}
void RecipeQueueSystem::start(long interval){
	std::cerr << "Recipe queue system started" << std::endl;
	auto loop_func = [=]() {
		while (!q_signal){
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
	int num_ingred = rand() % 4 + 2; 
	std::vector<std::string>ingredients;
	for (int i = 0; i < num_ingred; ++i) {
		int ingred_idx = rand() % ingredient_list.size();
		ingredients.push_back(ingredient_list[ingred_idx]);
	}
	RecipeInfo recipe_info; 
	Recipe* recipe = new Recipe(ingredients, &recipe_info);
	return recipe;
}