#include <vector>
#include <deque>
#include <string>
#include <thread>
#include <mutex>
struct RecipeInfo {
	std::string create_time;
	//customer info? score info? time limit?

	RecipeInfo();
	RecipeInfo(RecipeInfo* recipe_info);
};
struct Recipe {
	size_t max_sides = 100;
	std::vector<std::string> sides;
	size_t max_entrees = 100;
	std::vector<std::string> entrees;
	RecipeInfo* recipe_info;
	Recipe(size_t _max_sides = 1, size_t _max_entrees = 2);
	Recipe(std::vector<std::string>_sides, std::vector<std::string>_entrees, RecipeInfo* _recipe_info,
		size_t _max_sides = 1, size_t _max_entrees = 2);
	void TryAddSide(std::string side);
	void TryAddEntree(std::string entree);
	bool is_match(Recipe* _recipe); //check if the ingredients of two recipes match
};

struct RecipeQueueSystem{

	//dummy ingredient list contains all possible ingredients.
	std::vector<std::string>possible_sides = {"rice", "noodles"};
	std::vector<std::string>possible_entrees = {"chicken", "dumpling", "veggies"};

	std::deque<Recipe *>recipe_queue;
	std::mutex q_mtx;
	bool q_signal = false; //signal the periodical recipe generation to stop
	
	int max_queue_size = 6;
	void init();
	void start(long interval); //start a periodical recipe generation
	Recipe * generate_recipe();
};