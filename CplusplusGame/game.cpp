#define is_down(b) input->buttons[b].is_down
#define pressed(b) (input->buttons[b].is_down && input->buttons[b].changed)
#define released(b) (!input->buttons[b].is_down && input->buttons[b].changed)

internal void run_game(Input* input, float dt, char* score_buffer);
internal u_int run_collision(BetterRectangle adjusted_player_pos);
internal void server_pass();

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
	u_int color;
	u_int index;
	u_int score;
	u_int frame;
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
	Placing,
	Playing,
	Gamemode_Count
};

bool spawned_next_platform = false;
bool end_of_game = false;
u_int frame_count;
u_int result;

Player player;
GAMEMODE current_gamemode;
list<PlacedBlock> all_game_blocks;
list<Player*> players;

internal void init_game(bool initial = true)
{
	// Initialize
	end_of_game = false;
	all_game_blocks = {};
	result = 0;
	frame_count = 0;

	if (initial)
		players = {};

	// Parse information about client from the server
	sf::Packet initial_world;
	Message initial_player;
	sf::Packet other_players;
	init_server_connection(&initial_world, &initial_player, &other_players, initial);

	// Using server world info, set up the gamespace
	int g_temp;
	initial_world >> g_temp;
	current_gamemode = (GAMEMODE) g_temp;

	int blocks;
	initial_world >> blocks;
	for (int i = 0; i < blocks; i++)
	{
		PlacedBlock new_block;
		initial_world >> new_block.x >> new_block.y >> new_block.half_width >> new_block.half_height >> new_block.finish_block;
		all_game_blocks.push_back(new_block);
	}

	//---------------------------------------------------------------------------

	if (initial)
	{
		// Read Player starting position from server
		player.index = initial_player.index;
		player.player_spawn_x = initial_player.newX;
		player.player_spawn_y = initial_player.newY;
		player.color = initial_player.color;

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
		player.frame = initial_player.frame_ID;
		frame_count = player.frame;
		player.score = 0;

		//--------------------------------------------------------------------------

		// Read other players status from server
		if (player.index != 0)
		{
			u_int player_count;
			if (other_players >> player_count)
			{
				for (int i = 0; i < player_count; i++)
				{
					Player* new_player = new Player;
					other_players >> new_player->player_spawn_x;
					other_players >> new_player->player_spawn_y;
					other_players >> new_player->frame;
					other_players >> new_player->color;
					other_players >> new_player->index;

					new_player->player_pos_x = new_player->player_spawn_x;
					new_player->player_pos_y = new_player->player_spawn_y;
					new_player->player_half_width = 0.04f;
					new_player->player_half_height = 0.04f;
					new_player->x_speed = 0.f;
					new_player->y_speed = 0.f;
					new_player->x_acceleration = 0.f;
					new_player->y_acceleration = 0.f;

					new_player->player_old_x = new_player->player_pos_x;
					new_player->player_old_y = new_player->player_pos_y;

					new_player->score = 0;

					players.push_back(new_player);
				}
			}
		}
		players.push_back(&player);
	}
	else
	{
		for (std::list<Player*>::iterator it = players.begin(); it != players.end(); it++)
		{
			(*it)->player_pos_x = (*it)->player_spawn_x;
			(*it)->player_pos_y = (*it)->player_spawn_y;
			(*it)->player_old_x = (*it)->player_pos_x;
			(*it)->player_old_y = (*it)->player_pos_y;
			(*it)->frame = 0;
			(*it)->x_speed = 0.f;
			(*it)->y_speed = 0.f;
			(*it)->x_acceleration = 0.f;
			(*it)->y_acceleration = 0.f;
			(*it)->score = 0;
		}
	}
	return;

}

internal void run_game(Input* input, float dt)
{
	// Update
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

			for (list<Player*>::iterator it2 = players.begin(); it2 != players.end(); ++it2)
			{
				draw_rect((*it2)->player_pos_x, (*it2)->player_pos_y, (*it2)->player_half_width, (*it2)->player_half_height, (*it2)->color);
				draw_number((*it2)->score, -1.5f + .2f * (*it2)->index, 0.9f, 0.02f, (*it2)->color);
			}

			if (end_of_game)
			{
				draw_result(result, player.color);
			}
			
			break;
		/*
			Platforming Gamemode
		*/
		case (GAMEMODE::Playing):
			if (is_down(BUTTON_A)) player.x_acceleration -= 30;
			if (is_down(BUTTON_D)) player.x_acceleration += 30;
			if (pressed(BUTTON_SPACE))
			{
				//player.y_acceleration += 1000;
				if (grounded)
					player.y_acceleration += 700;
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

			
			// Player Collision Detection	
			collision_result = run_collision(adjusted_player_position);
			if (collision_result > 0)
			{
				if (collision_result == 1) server_send_score_update();
				player.player_pos_x = player.player_spawn_x;
				player.player_pos_y = player.player_spawn_y;
				player.x_speed = 0.f;
				player.y_speed = 0.f;
				player.x_acceleration = 0.f;
				player.y_acceleration = 0.f;
				spawned_next_platform = false;
				server_send_next_turn(player.index);
			}
			
			// Rendering Game Objects
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

			for (list<Player*>::iterator it2 = players.begin(); it2 != players.end(); ++it2)
			{
				draw_rect((*it2)->player_pos_x, (*it2)->player_pos_y, (*it2)->player_half_width, (*it2)->player_half_height, (*it2)->color);
				draw_number((*it2)->score, -1.5f + .2f * (*it2)->index, 0.9f, 0.02f, (*it2)->color);
			}

			break;

		/*
			Placing new blocks Gamemode 
		*/
		case (GAMEMODE::Placing):
			if (released(BUTTON_Z))
			{
				server_send_new_block(all_game_blocks.back().x, all_game_blocks.back().y, all_game_blocks.back().half_width, all_game_blocks.back().half_height);
				server_send_next_turn(player.index);
			}
			
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
			for (list<Player*>::iterator it2 = players.begin(); it2 != players.end(); ++it2)
			{
				draw_rect((*it2)->player_pos_x, (*it2)->player_pos_y, (*it2)->player_half_width, (*it2)->player_half_height, (*it2)->color);
				draw_number((*it2)->score, -1.5f + .2f * (*it2)->index, 0.9f, 0.02f, (*it2)->color);
			}
			break;
		default:
			break;
	}

	server_pass();
	frame_count++;
}

internal void server_pass()
{
	if ((player.player_old_x != player.player_pos_x) || (player.player_old_y != player.player_pos_y))
	{
		sf::Packet send_pos_update;
		send_pos_update << player.index << player.player_pos_x << player.player_pos_y;
		update_server_position(send_pos_update);
	}

	sf::Packet update;
	sf::Socket::Status status = socket_tcp.receive(update);
	if (status == sf::Socket::Done)
	{
		int header;
		if (update >> header)
		{
			PlacedBlock new_block;
			Player* new_player = new Player;
			std::list<Player*>::iterator it = players.begin();
			switch (header)
			{
				case (PacketType::NextTurn):
					current_gamemode = (GAMEMODE)server_turn_update(update, (u_int)current_gamemode);
					break;
				case (PacketType::NewBlock):
					if(server_new_block_update(update, &(new_block.x), &(new_block.y), &(new_block.half_width), &(new_block.half_height)) != 0)
						all_game_blocks.push_back(new_block);
					
					break;
				case (PacketType::ScoreUpdate):
					u_int new_score;
					while (update >> new_score) 
					{
						(*it)->score = new_score;
						it++;
					}
					break;
				case (PacketType::NewPlayer):
					update >> new_player->player_spawn_x;
					update >> new_player->player_spawn_y;
					update >> new_player->frame;
					update >> new_player->color;
					update >> new_player->index;

					new_player->player_pos_x = new_player->player_spawn_x;
					new_player->player_pos_y = new_player->player_spawn_y;
					new_player->player_half_width = 0.04f;
					new_player->player_half_height = 0.04f;
					new_player->x_speed = 0.f;
					new_player->y_speed = 0.f;
					new_player->x_acceleration = 0.f;
					new_player->y_acceleration = 0.f;

					new_player->player_old_x = new_player->player_pos_x;
					new_player->player_old_y = new_player->player_pos_y;

					new_player->score = 0;

					players.push_back(new_player);
					break;
				case (PacketType::EndOfGame):
					if (update >> result)
					{
						end_of_game = true;
						current_gamemode = GAMEMODE::Waiting;
					}
					break;
				case (PacketType::Reset):
					init_game(false);
					break;
				default:
					break;
			}
		}

	}
	
	sf::Packet pos_update;
	sf::IpAddress sender = SERVER_IP;
	unsigned short server_port_udp = SERVER_PORT_UDP;
	sf::Socket::Status status_udp = socket_udp.receive(pos_update, sender, server_port_udp);
	if (status_udp == sf::Socket::Done)
	{
		float newX;
		float newY;
		u_int index;
		if (pos_update >> index >> newX >> newY)
		{
			std::list<Player*>::iterator it = players.begin();
			for (int i = 0; i < index; i++)
			{
				it++;
			}

			(*it)->player_pos_x = newX;
			(*it)->player_pos_y = newY;
		}
	}

}

internal u_int run_collision(BetterRectangle adjusted_player_pos)
{

	// Complex Collsion detection so each step is commented to explain what it does

	/*	Section 0
		Keep player on screen.
	*/
	if (adjusted_player_pos.x0 < 0 ||
		adjusted_player_pos.x1 > buf.b_width
		)
	{
		player.player_pos_x = player.player_old_x;
		player.x_speed *= 0.f;
		player.x_acceleration = 0.f;
	}


	/* old if block
		if (adjusted_player_pos.y0 < 0 ||
		adjusted_player_pos.y0 > buf.b_height ||
		adjusted_player_pos.y1 < 0 ||
		adjusted_player_pos.y1 > buf.b_height
		)
	*/
	if (adjusted_player_pos.y0 < 0)
	{
		player.player_pos_y = player.player_old_y;
		player.y_speed *= 0.f;
		player.y_acceleration = 0.f;
		return 2;
	}
	else if (adjusted_player_pos.y1 > buf.b_height)
	{
		player.player_pos_y = player.player_old_y;
		player.y_speed *= 0.f;
		player.y_acceleration = 0.f;
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
