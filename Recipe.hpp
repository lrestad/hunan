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
	std::vector<std::string> sides;
	std::vector<std::string> entrees;
	RecipeInfo* recipe_info;
	Recipe();
	Recipe(std::vector<std::string>_sides, std::vector<std::string>_entrees, RecipeInfo* _recipe_info);
	void AddSide(std::string side);
	void AddEntree(std::string entree);
	bool is_match(Recipe* _recipe); //check if the ingredients of two recipes match
	// Eventually we'll need to provide more info such as the names of the
	// children so that we can find them and texture them
	static std::string mesh_name_from_recipe(const Recipe &recipe);

	static const std::string inventory_meshname_map[2][3];
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