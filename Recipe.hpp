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
};

struct RecipeQueueSystem {
	std::vector<std::string> allowed_entrees = {"chicken", "veggies"};
	std::vector<std::string> allowed_sides = {"rice", "noodles", "dumplings"};
	std::deque<Recipe*> recipe_queue;

	void generate_order(unsigned int level, unsigned int total_time, unsigned int elapsed);
};