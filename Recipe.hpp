#include <vector>
#include <deque>
#include <string>
#include <thread>
#include <mutex>
struct RecipeInfo {
	std::string create_time;
	//customer info?

	RecipeInfo();
	RecipeInfo(RecipeInfo* recipe_info);
};
struct Recipe {
	std::vector<std::string> ingredients;
	RecipeInfo* recipe_info;
	Recipe(std::vector<std::string>_ingredients, RecipeInfo* _recipe_info);
};

struct RecipeQueueSystem{

	//dummy ingredient list contains all possible ingredients.
	std::vector<std::string>ingredient_list = {"kungbao_chicken", "mapo tofu", "lo mein"};

	std::deque<Recipe *>recipe_queue;
	std::mutex q_mtx;
	bool q_signal = false; //signal the periodical recipe generation to stop

	void init();
	void start(long interval);
	Recipe * generate_recipe();
};