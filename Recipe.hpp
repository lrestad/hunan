#include <vector>
#include <deque>
#include <string>

struct Recipe {
	Recipe();
	Recipe(std::vector<std::string> _entrees, std::vector<std::string> _sides, bool _valid);
	std::vector<std::string> entrees;
	std::vector<std::string> sides;
	bool valid;
	void TryAddSide(std::string side);
	void TryAddEntree(std::string entree);
	bool is_match(Recipe* _recipe); //check if the ingredients of two recipes match
	std::string to_image_name();
};

struct RecipeQueueSystem {
	std::vector<std::string> allowed_entrees = {"chicken", "veggies"};
	std::vector<std::string> allowed_sides = {"rice", "noodles", "dumpling"};
	std::deque<Recipe*> recipe_queue;

	float add_recipe_delay = 1.5f;
	float add_recipe_timer = 0.0f;
	int queue_max_size = 5;

	void generate_order(unsigned int level, unsigned int total_time, unsigned int elapsed);
};