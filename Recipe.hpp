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
	std::vector<std::string> ingredients;
	RecipeInfo* recipe_info;
	Recipe(std::vector<std::string>_ingredients, RecipeInfo* _recipe_info);
	bool is_match(Recipe* _recipe); //check if the ingredients of two recipes match
};

struct RecipeQueueSystem{

	//dummy ingredient list contains all possible ingredients.
	std::vector<std::string>ingredient_list = {"chicken", "rice"};

	std::deque<Recipe *>recipe_queue;
	std::mutex q_mtx;
	bool q_signal = false; //signal the periodical recipe generation to stop
	
	int max_queue_size = 5;
	void init();
	void start(long interval); //start a periodical recipe generation
	Recipe * generate_recipe();
};