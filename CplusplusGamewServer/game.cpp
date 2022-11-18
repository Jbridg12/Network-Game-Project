/*struct Player
{
	float player_pos_x;
	float player_pos_y;

	float player_old_x;
	float player_old_y;

	float player_half_width;
	float player_half_height;

	float x_speed;
	float y_speed;
	float x_acceleration;
	float y_acceleration;
};
*/
struct PlacedBlock
{
	float x;
	float y;
	float half_width;
	float half_height;
	bool finish_block = false;
	float speed = 0.005f;
};

enum GAMEMODE
{
	Waiting,
	Playing,
	Placing,
	Gamemode_Count
};


//Player player;
u_int score;
u_int frame_count;
std::list<PlacedBlock> all_game_blocks;
PlacedBlock start;
PlacedBlock finish;

internal void init_game()
{

	start.x = -1.16f;
	start.y = -0.8f;
	start.half_width = 0.7f;
	start.half_height = 0.2f;

	finish.x = 0.8f;
	finish.y = 0.6f;
	finish.half_width = 0.2f;
	finish.half_height = 0.05f;
	finish.finish_block = true;

	all_game_blocks = { start, finish };
	return;

}


