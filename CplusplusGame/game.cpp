#define is_down(b) input->buttons[b].is_down
#define pressed(b) (input->buttons[b].is_down && input->buttons[b].changed)
#define released(b) (!input->buttons[b].is_down && input->buttons[b].changed)

internal void run_game(Input* input, float dt);
internal u_int run_collision(BetterRectangle adjusted_player_pos);
internal void server_interact();

struct Player
{
	// Physics Variables
	float player_pos_x;
	float player_pos_y;

	float player_spawn_x;
	float player_spawn_y;

	float player_old_x;
	float player_old_y;

	float player_half_width;
	float player_half_height;

	float x_speed;
	float y_speed;
	float x_acceleration;
	float y_acceleration;

	// Server variables
	u_int index;
	u_int score;
};
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

GAMEMODE current_gamemode;

Player player;
u_int frame_count;
bool my_turn = true;
bool spawned_next_platform = false;
list<PlacedBlock> all_game_blocks;

internal void init_game()
{
	// Initialize

	// Parse information about client from the server
	WorldUpdate initial_world;
	Message initial_player;
	init_server_connection(&initial_world, &initial_player);

	// Using server world info, set up the gamespace
	current_gamemode = (GAMEMODE) initial_world.current_gamemode;

	PlacedBlock start {
		initial_world.start_x,
		initial_world.start_y,
		initial_world.start_half_width,
		initial_world.start_half_height,
		initial_world.start_finish_block
	};

	PlacedBlock finish {
		initial_world.finish_x,
		initial_world.finish_y,
		initial_world.finish_half_width,
		initial_world.finish_half_height,
		initial_world.finish_block
	};
	all_game_blocks = {start, finish};

	//---------------------------------------------------------------------------

	// Read Player starting position from server
	player.index = initial_player.index;
	player.player_spawn_x = initial_player.newX;
	player.player_spawn_y = initial_player.newY;

	player.player_pos_x = player.player_spawn_x;
	player.player_pos_y = player.player_spawn_y;
	player.player_half_width = 0.04f;
	player.player_half_height = 0.04f;
	player.x_speed = 0.f;
	player.y_speed = 0.f;
	player.x_acceleration = 0.f;
	player.y_acceleration = 0.f;

	player.player_old_x = player.player_pos_x;
	player.player_old_y = player.player_pos_y;
	frame_count = 0;

	
	player.score = 0;
	return;

}

internal void run_game(Input* input, float dt)
{
	// Update
	//char stuff[128];
	//sprintf_s(stuff, "X: %f | Y: %f\n", player.player_pos_x, player.player_pos_y);
	//OutputDebugString(stuff);
	clear_screen(0x00ff11);


	u_int collision_result = 1;
	bool turn_end = false;

	bool grounded = false;
	if (player.player_old_y == player.player_pos_y)
		grounded = true;

	player.player_old_x = player.player_pos_x;
	player.player_old_y = player.player_pos_y;

	switch (current_gamemode)
	{
		/*
			Gamemode for when waiting for the player turn
		*/
		case (GAMEMODE::Waiting):
			if (released(BUTTON_Z))
			{
				server_send_next_turn(player.index);
				//turn_end = true;
			}
			for (list<PlacedBlock>::iterator it = all_game_blocks.begin(); it != all_game_blocks.end(); ++it)
			{
				if (it->finish_block)
				{
					draw_checkered_block(it->x, it->y, it->half_width, it->half_height, false);
				}
				else
				{
					draw_rect(it->x, it->y, it->half_width, it->half_height, 0x0000ff);
				}
			}
			break;
		/*
			Platforming Gamemode
		*/
		case (GAMEMODE::Playing):
			//if (is_down(BUTTON_W)) player.y_acceleration += 30;
			//if (is_down(BUTTON_S)) player.y_acceleration -= 30;
			if (is_down(BUTTON_A)) player.x_acceleration -= 30;
			if (is_down(BUTTON_D)) player.x_acceleration += 30;
			if (pressed(BUTTON_SPACE))
			{
				player.y_acceleration += 1000;
				//if (grounded)
					//player.y_acceleration += 1000;
			}
			if (pressed(BUTTON_Z))
			{
				spawned_next_platform = false;
				//current_gamemode = GAMEMODE::Placing;
			}

			// Method to add friction in specific directions to lower speed over time
			//player.y_acceleration -= player.y_speed * 20.0f;
			player.x_acceleration -= player.x_speed * 25.0f;
			
			player.player_pos_y += (player.y_speed * dt) + (player.y_acceleration * dt * dt * 0.5f);
			player.player_pos_x += (player.x_speed * dt) + (player.x_acceleration * dt * dt * 0.5f);
			player.y_speed += player.y_acceleration * dt;
			player.x_speed += player.x_acceleration * dt;


			// Reset acceleration every frame, except for Y due to gravity.
			player.y_acceleration = -15.0f;
			player.x_acceleration = 0.f;


			BetterRectangle adjusted_player_position = adjust_to_screen(player.player_pos_x, 
																		player.player_pos_y, 
																		player.player_half_width, 
																		player.player_half_height);

			collision_result = run_collision(adjusted_player_position);
			if (collision_result > 0)
			{
				if (collision_result == 1) player.score++;
				player.player_pos_x = player.player_spawn_x;
				player.player_pos_y = player.player_spawn_y;
				spawned_next_platform = false;
				server_send_next_turn(player.index);
			}
			

			for (list<PlacedBlock>::iterator it = all_game_blocks.begin(); it != all_game_blocks.end(); ++it)
			{
				if (it->finish_block)
				{
					draw_checkered_block(it->x, it->y, it->half_width, it->half_height, false);
				}
				else
				{
					draw_rect(it->x, it->y, it->half_width, it->half_height, 0x0000ff);
				}
			}

			draw_rect(player.player_pos_x, player.player_pos_y, player.player_half_width, player.player_half_height, 0xff00ff);
			break;

		/*
			Placing new blocks Gamemode 
		*/
		case (GAMEMODE::Placing):
			if (released(BUTTON_Z))
			{
				server_send_next_turn(player.index);
				//turn_end = true;
			}

			if (my_turn)
			{
				if (!spawned_next_platform) {
					PlacedBlock new_platform;

					new_platform.x = 0.f;
					new_platform.y = 0.f;
					new_platform.half_width = 0.1f;
					new_platform.half_height = 0.05f;
					all_game_blocks.push_back(new_platform);

					spawned_next_platform = true;
				}

				if (is_down(BUTTON_W)) all_game_blocks.back().y += 1 * all_game_blocks.back().speed;
				if (is_down(BUTTON_S)) all_game_blocks.back().y -= 1 * all_game_blocks.back().speed;
				if (is_down(BUTTON_A)) all_game_blocks.back().x -= 1 * all_game_blocks.back().speed;
				if (is_down(BUTTON_D)) all_game_blocks.back().x += 1 * all_game_blocks.back().speed;
			}
			
			for (list<PlacedBlock>::iterator it = all_game_blocks.begin(); it != all_game_blocks.end(); ++it)
			{
				if (it->finish_block)
				{
					draw_checkered_block(it->x, it->y, it->half_width, it->half_height, true);
				}
				else
				{
					draw_rect(it->x, it->y, it->half_width, it->half_height, 0x0000ff);
				}
			}

			draw_rect(player.player_pos_x, player.player_pos_y, player.player_half_width, player.player_half_height, 0xf00001);
			break;
		default:
			break;
	}

	server_interact();
	frame_count++;
}

internal void server_interact()
{
	if ((player.player_old_x != player.player_pos_x) || (player.player_old_y != player.player_pos_y))
	{
		Packet update;
		update << player.player_pos_x << player.player_pos_y;
		update_server_position(update);
	}

	current_gamemode = (GAMEMODE) server_turn_update((u_int)current_gamemode);
}

internal u_int run_collision(BetterRectangle adjusted_player_pos)
{

	// Complex Collsion detection so each step is commented to explain what it does

	/*	Section 0
		Keep player on screen.
	*/
	if (adjusted_player_pos.x0 < 0 ||
		adjusted_player_pos.x0 > buf.b_width ||
		adjusted_player_pos.x1 < 0 ||
		adjusted_player_pos.x1 > buf.b_width
		)
	{
		player.player_pos_x = player.player_old_x;
		player.x_speed *= 0.f;
		player.x_acceleration = 0.f;
	}

	if (adjusted_player_pos.y0 < 0 ||
		adjusted_player_pos.y0 > buf.b_height ||
		adjusted_player_pos.y1 < 0 ||
		adjusted_player_pos.y1 > buf.b_height
		)
	{
		player.player_pos_y = player.player_old_y;
		player.y_speed *= 0.f;
		player.y_acceleration = 0.f;
		//return 0;
	}

	for (list<PlacedBlock>::iterator it = all_game_blocks.begin(); it != all_game_blocks.end(); it++)
	{
		float left_bounds = it->x - it->half_width;
		float right_bounds = it->x + it->half_width;
		float up_bounds = it->y + it->half_height;
		float lower_bounds = it->y - it->half_height;



		/*	Section 1
			Check if the right side of the player is inside of any objects on the
			screen and adjust accordingly.
		*/ 
		 

		// Perform check if right side of player is in object
		if ((player.player_pos_x + player.player_half_width) <= right_bounds &&
			(player.player_pos_x + player.player_half_width) >= left_bounds
			)
		{
			// Now check if the player's top is within the object as well
			if ((player.player_pos_y + player.player_half_height) >= lower_bounds &&
				(player.player_pos_y + player.player_half_height) <= up_bounds)
			{
				// Check if the player is colliding with the finish flag
				if (it->finish_block) return 1;

				// Finally determine if the x direction was the axis that collided
				if (((player.player_old_y + player.player_half_height) >= lower_bounds &&
					(player.player_old_y + player.player_half_height) <= up_bounds) ||
					((player.player_old_y - player.player_half_height) >= lower_bounds &&
					(player.player_old_y - player.player_half_height) <= up_bounds))
				{
					// If so perform x adjustments
					player.player_pos_x = player.player_old_x;
					player.x_speed *= 0.f;
					player.x_acceleration = 0.f;
				}
				else
				{
					// If not then perform y adjustments
					player.player_pos_y = player.player_old_y;
					player.y_speed *= 0.f;
					player.y_acceleration = 0.f;
				}

			}
			// Check if the player's bottom side is inside the object 
			if ((player.player_pos_y - player.player_half_height) >= lower_bounds &&
				(player.player_pos_y - player.player_half_height) <= up_bounds)
			{
				if (it->finish_block) return 1;

				//Perforom x/y axis collision check
				if (((player.player_old_y + player.player_half_height) >= lower_bounds &&
					(player.player_old_y + player.player_half_height) <= up_bounds) ||
					((player.player_old_y - player.player_half_height) >= lower_bounds &&
						(player.player_old_y - player.player_half_height) <= up_bounds))
				{
					// Adjust X
					player.player_pos_x = player.player_old_x;
					player.x_speed *= 0.f;
					player.x_acceleration = 0.f;
				}
				else
				{
					//Adjust Y
					player.player_pos_y = player.player_old_y;
					player.y_speed *= 0.f;
					player.y_acceleration = 0.f;
				}
			}
			
		}

		/*	Section 1
			Check if the left side of the player is inside of any objects on the
			screen and adjust accordingly.

			This needs to be an entire new condition because of the slightly different checks
			and actions necessary based on what side has collided.

			Follows the exact same structure as section 1, so comments were omitted.
		*/

		if ((player.player_pos_x - player.player_half_width) <= right_bounds && 
			(player.player_pos_x - player.player_half_width) >= left_bounds
			)
		{
			if ((player.player_pos_y + player.player_half_height) >= lower_bounds &&
				(player.player_pos_y + player.player_half_height) <= up_bounds)
			{
				if (it->finish_block) return 1;

				if (((player.player_old_y + player.player_half_height) >= lower_bounds &&
					(player.player_old_y + player.player_half_height) <= up_bounds) ||
					((player.player_old_y - player.player_half_height) >= lower_bounds &&
						(player.player_old_y - player.player_half_height) <= up_bounds))
				{
					player.player_pos_x = player.player_old_x;
					player.x_speed *= 0.f;
					player.x_acceleration = 0.f;
				}
				else
				{
					player.player_pos_y = player.player_old_y;
					player.y_speed *= 0.f;
					player.y_acceleration = 0.f;
				}
			}
			if ((player.player_pos_y - player.player_half_height) >= lower_bounds &&
				(player.player_pos_y - player.player_half_height) <= up_bounds)
			{
				if (it->finish_block) return 1;

				if (((player.player_old_y + player.player_half_height) >= lower_bounds &&
					(player.player_old_y + player.player_half_height) <= up_bounds) ||
					((player.player_old_y - player.player_half_height) >= lower_bounds &&
						(player.player_old_y - player.player_half_height) <= up_bounds))
				{
					player.player_pos_x = player.player_old_x;
					player.x_speed *= 0.f;
					player.x_acceleration = 0.f;
				}
				else
				{
					player.player_pos_y = player.player_old_y;
					player.y_speed *= 0.f;
					player.y_acceleration = 0.f;
				}
			}
			
		}
		
	}

	return 0;
}
