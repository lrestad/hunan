#include "Recipe.hpp"
#include <algorithm>
#include <random>

Recipe::Recipe() {}

Recipe::Recipe(std::vector<std::string> _entrees, std::vector<std::string> _sides, bool _valid) {
	entrees = std::vector<std::string>(_entrees.begin(), _entrees.end());
	sides = std::vector<std::string>(_sides.begin(), _sides.end());
	valid = _valid;
}

bool Recipe::is_match(Recipe* _recipe) {
	std::vector<std::string> entrees1 = entrees;
	std::vector<std::string> entrees2 = _recipe->entrees;
	std::sort(entrees1.begin(), entrees1.end());
	std::sort(entrees2.begin(), entrees2.end());

	std::vector<std::string> sides1 = sides;
	std::vector<std::string> sides2 = _recipe->sides;
	std::sort(sides1.begin(), sides1.end());
	std::sort(sides2.begin(), sides2.end());

	return sides1 == sides2 && entrees1 == entrees2;
}

void Recipe::TryAddSide(std::string side) {
	if (sides.size() < 2)
		sides.push_back(side);
}

void Recipe::TryAddEntree(std::string entree) {
	if (entrees.size() < 1)
		entrees.push_back(entree);
}

void RecipeQueueSystem::generate_order(unsigned int level, unsigned int total_time, unsigned int elapsed) {

	// std::printf("generating order %lu \n", recipe_queue.size());

	// Do not add a new order
	/*if ((recipe_queue.size() >= 5) || (total_time / 5000 == (total_time + elapsed) / 5000)) {
		return;
	}*/

	unsigned int valid_recipe;
	unsigned int num_sides;
	unsigned int entre_idx;
	unsigned int side_idx;

	// Do not let the queue get too long
	if (recipe_queue.size() >= 5) {
		return;
	}

	// Generate an order
	std::vector<std::string> entrees;
	std::vector<std::string> sides;
	switch (level) {
		// Level one: only valid orders with chicken and rice
		case 1:
			// Add exactly 1 entre
			entrees.push_back("chicken");
				
			// Add 0-2 sides
			num_sides = rand() % 3;
			for (int i = 0; i < num_sides; i++) {
				sides.push_back("rice");
			}

			recipe_queue.push_back(new Recipe(entrees, sides, true));
			return;

		// Level 3: can have invalid orders, uses all ingredients
		case 3:
			valid_recipe = rand() % 4;
			if (valid_recipe == 0) {
				// Add 0 entres
					
				// Add 1-3 sides
				num_sides = rand() % 3 + 1;
				for (int i = 0; i < num_sides; i++) {
					side_idx = rand() % allowed_sides.size();
					sides.push_back(allowed_sides[side_idx]);
				}

				recipe_queue.push_back(new Recipe(entrees, sides, false));
				return;

			} else if (valid_recipe == 1) {
				// Add 2 entres
				for (int i = 0; i < 2; i++) {
					entre_idx = rand() % allowed_entrees.size();
					entrees.push_back(allowed_entrees[entre_idx]);
				}
					
				// Add 0-1 sides
				num_sides = rand() % 2;
				for (int i = 0; i < num_sides; i++) {
					side_idx = rand() % allowed_sides.size();
					sides.push_back(allowed_sides[side_idx]);
				}

				recipe_queue.push_back(new Recipe(entrees, sides, false));
				return;
			}

		// Level 2 (and the valid half of Level 3): uses all ingredients
		default:
			// Add exactly 1 entre
			entre_idx = rand() % allowed_entrees.size();
			entrees.push_back(allowed_entrees[entre_idx]);
				
			// Add 0-2 sides
			num_sides = rand() % 3;
			for (int i = 0; i < num_sides; i++) {
				side_idx = rand() % allowed_sides.size();
				sides.push_back(allowed_sides[side_idx]);
			}

			recipe_queue.push_back(new Recipe(entrees, sides, true));
			return;
	}
}