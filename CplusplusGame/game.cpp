#define is_down(b) input->buttons[b].is_down
#define pressed(b) (input->buttons[b].is_down && input->buttons[b].changed)
#define released(b) (!input->buttons[b].is_down && input->buttons[b].changed)

struct Frame
{
	float x;
	float y;
	u_int id;
};
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


	bool interpolate_prediction;
	Frame past[3];
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

internal void run_game(Input* input, float dt);
internal u_int run_collision(Player* target);
internal void server_pass(float dt);
internal void predict_players(float dt);
internal void reset_prediction_frames();


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
		bool empty_list;
		other_players >> empty_list;
		if (empty_list)
		{
			u_int player_count;
			if (other_players >> player_count)
			{
				for (int i = 0; i < player_count; i++)
				{
					Player* new_player = new Player;
					other_players >> new_player->player_pos_x;
					other_players >> new_player->player_pos_y;
					other_players >> new_player->frame;
					other_players >> new_player->color;
					other_players >> new_player->index;

					new_player->player_spawn_x = (float)(-1.6f + (new_player->index * 0.2));
					new_player->player_spawn_y = -0.5f;
					new_player->player_half_width = 0.04f;
					new_player->player_half_height = 0.04f;
					new_player->x_speed = 0.f;
					new_player->y_speed = 0.f;
					new_player->x_acceleration = 0.f;
					new_player->y_acceleration = 0.f;

					new_player->player_old_x = new_player->player_pos_x;
					new_player->player_old_y = new_player->player_pos_y;

					new_player->score = 0;
					new_player->interpolate_prediction = false;
					new_player->past[0] = Frame{ new_player->player_pos_x, new_player->player_pos_y, new_player->frame };
					new_player->past[1] = Frame{ new_player->player_pos_x, new_player->player_pos_y, new_player->frame };
					new_player->past[2] = Frame{ new_player->player_pos_x, new_player->player_pos_y, new_player->frame };

					//players.insert(std::next(players.begin(), new_player->index), new_player);
					players.push_back(new_player);
				}
			}
		}
		players.insert(std::next(players.begin(), player.index), &player);
		//players.push_back(&player);
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

			
			// Player Collision Detection	
			collision_result = run_collision(&player);
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
				reset_prediction_frames();
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
				reset_prediction_frames();
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

	server_pass(dt);
	frame_count++;
}

internal void server_pass(float dt)
{
	if ((player.player_old_x != player.player_pos_x) || (player.player_old_y != player.player_pos_y))
	{
		sf::Packet send_pos_update;
		send_pos_update << player.index << player.player_pos_x << player.player_pos_y << frame_count;
		update_server_position(send_pos_update);
	}

	sf::Packet update;
	sf::Socket::Status status = socket_tcp.receive(update);
	if (status == sf::Socket::Done)
	{
		int header;
		if (update >> header)
		{
			u_int dis_index;
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

					players.insert(std::next(it, new_player->index), new_player);
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
				case (PacketType::Disconnect):
					if (update >> dis_index)
					{
						players.erase(std::next(it, dis_index));
					}
					break;
				default:
					break;
			}
		}

	}
	
	sf::Packet pos_update;
	sf::IpAddress sender = SERVER_IP;
	unsigned short spu2 = SERVER_PORT_UDP;
	sf::Socket::Status status_udp = socket_udp.receive(pos_update, sender, spu2);
	if (status_udp == sf::Socket::Done)
	{
		float newX;
		float newY;
		u_int index;
		u_int frame;
		if (pos_update >> index >> newX >> newY >> frame)
		{
			std::list<Player*>::iterator it = players.begin();
			while ((*it)->index != index)
			{
				it++;
			}

			// Check if update is in chronologic order
			if (frame > (*it)->frame)
			{
				if ((*it)->interpolate_prediction)
				{
					(*it)->frame = frame;
					(*it)->player_pos_x = (newX * 0.5) + ((*it)->player_pos_x * 0.5);
					(*it)->player_pos_y = (newY * 0.5) + ((*it)->player_pos_y * 0.5);

					(*it)->past[0] = (*it)->past[1];
					(*it)->past[1] = (*it)->past[2];
					(*it)->past[2] = Frame{ newX, newY, frame };
					(*it)->interpolate_prediction = false;
				}
				else
				{
					(*it)->frame = frame;
					(*it)->player_pos_x = newX;
					(*it)->player_pos_y = newY;

					(*it)->past[0] = (*it)->past[1];
					(*it)->past[1] = (*it)->past[2];
					(*it)->past[2] = Frame{ newX, newY, frame };
				}

				(*it)->player_old_x = (*it)->player_pos_x;
				(*it)->player_old_y = (*it)->player_pos_y;
				
			}
		}
	}
	else	// If we don't get a position update from the server then we predict player movement from past behavior 
	{
		predict_players(dt);
	}

}


// CUT or fix cause my god is it not helpful

internal void predict_players(float dt)
{
	for (std::list<Player*>::iterator it = players.begin(); it != players.end(); it++)
	{
		if ((*it)->index == player.index)
		{
			continue;
		}
		if ((*it)->player_pos_x == (*it)->player_spawn_x && (*it)->player_pos_y == (*it)->player_spawn_y)
		{
			// DO NOT PREDICT
			// Currently not in a motion based gamemode
			continue;
		}
		else
		{
			// predict position of player from past two frames
			float v1_x = (float) ((*it)->past[1].x - (*it)->past[0].x) / (std::abs((float)(*it)->past[1].id - (*it)->past[0].id) * dt);
			float v2_x = (float) ((*it)->past[2].x - (*it)->past[1].x) / (std::abs((float)(*it)->past[2].id - (*it)->past[1].id) * dt);
			float v1_y = (float) ((*it)->past[1].y - (*it)->past[0].y) / (std::abs((float)(*it)->past[1].id - (*it)->past[0].id) * dt);
			float v2_y = (float) ((*it)->past[2].y - (*it)->past[1].y) / (std::abs((float)(*it)->past[2].id - (*it)->past[1].id) * dt);

			float a_x = (float) (v2_x - v1_x) / (std::abs((float) (*it)->past[2].id - (*it)->past[1].id) * dt);
			float a_y = (float) (v2_y - v1_y) / (std::abs((float) (*it)->past[2].id - (*it)->past[1].id) * dt);

			float t = std::abs((float) frame_count - (*it)->past[2].id) * dt;

			(*it)->player_pos_x += v2_x * t + (0.5 * a_x * t * t);
			(*it)->player_pos_y += v2_y * t + (0.5 * a_y * t * t);

			run_collision((*it));
			(*it)->player_old_x = (*it)->player_pos_x;
			(*it)->player_old_y = (*it)->player_pos_y;

			(*it)->interpolate_prediction = true;

		}
	}

	return;
}

internal u_int run_collision(Player* target)
{

	// Complex Collsion detection so each step is commented to explain what it does

	BetterRectangle adjusted_player_pos = adjust_to_screen(target->player_pos_x,
															target->player_pos_y,
															target->player_half_width,
															target->player_half_height);

	/*	Section 0
		Keep player on screen.
	*/
	if (adjusted_player_pos.x0 < 0 ||
		adjusted_player_pos.x1 > buf.b_width
		)
	{
		target->player_pos_x = target->player_old_x;
		target->x_speed *= 0.f;
		target->x_acceleration = 0.f;
	}

	if (adjusted_player_pos.y0 < 0)
	{
		target->player_pos_y = target->player_old_y;
		target->y_speed *= 0.f;
		target->y_acceleration = 0.f;
		return 2;
	}
	else if (adjusted_player_pos.y1 > buf.b_height)
	{
		target->player_pos_y = target->player_old_y;
		target->y_speed *= 0.f;
		target->y_acceleration = 0.f;
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
		if ((target->player_pos_x + target->player_half_width) <= right_bounds &&
			(target->player_pos_x + target->player_half_width) >= left_bounds
			)
		{
			// Now check if the player's top is within the object as well
			if ((target->player_pos_y + target->player_half_height) >= lower_bounds &&
				(target->player_pos_y + target->player_half_height) <= up_bounds)
			{
				// Check if the player is colliding with the finish flag
				if (it->finish_block) return 1;

				// Finally determine if the x direction was the axis that collided
				if (((target->player_old_y + target->player_half_height) >= lower_bounds &&
					(target->player_old_y + target->player_half_height) <= up_bounds) ||
					((target->player_old_y - target->player_half_height) >= lower_bounds &&
					(target->player_old_y - target->player_half_height) <= up_bounds))
				{
					// If so perform x adjustments
					target->player_pos_x = target->player_old_x;
					target->x_speed *= 0.f;
					target->x_acceleration = 0.f;
				}
				else
				{
					// If not then perform y adjustments
					target->player_pos_y = target->player_old_y;
					target->y_speed *= 0.f;
					target->y_acceleration = 0.f;
				}

			}
			// Check if the player's bottom side is inside the object 
			if ((target->player_pos_y - target->player_half_height) >= lower_bounds &&
				(target->player_pos_y - target->player_half_height) <= up_bounds)
			{
				if (it->finish_block) return 1;

				//Perforom x/y axis collision check
				if (((target->player_old_y + target->player_half_height) >= lower_bounds &&
					(target->player_old_y + target->player_half_height) <= up_bounds) ||
					((target->player_old_y - target->player_half_height) >= lower_bounds &&
						(target->player_old_y - target->player_half_height) <= up_bounds))
				{
					// Adjust X
					target->player_pos_x = target->player_old_x;
					target->x_speed *= 0.f;
					target->x_acceleration = 0.f;
				}
				else
				{
					//Adjust Y
					target->player_pos_y = target->player_old_y;
					target->y_speed *= 0.f;
					target->y_acceleration = 0.f;
				}
			}
			
		}

		/*	Section 2
			Check if the left side of the player is inside of any objects on the
			screen and adjust accordingly.

			This needs to be an entire new condition because of the slightly different checks
			and actions necessary based on what side has collided.

			Follows the exact same structure as section 1, so comments were omitted.
		*/

		if ((target->player_pos_x - target->player_half_width) <= right_bounds && 
			(target->player_pos_x - target->player_half_width) >= left_bounds
			)
		{
			if ((target->player_pos_y + target->player_half_height) >= lower_bounds &&
				(target->player_pos_y + target->player_half_height) <= up_bounds)
			{
				if (it->finish_block) return 1;

				if (((target->player_old_y + target->player_half_height) >= lower_bounds &&
					(target->player_old_y + target->player_half_height) <= up_bounds) ||
					((target->player_old_y - target->player_half_height) >= lower_bounds &&
						(target->player_old_y - target->player_half_height) <= up_bounds))
				{
					target->player_pos_x = target->player_old_x;
					target->x_speed *= 0.f;
					target->x_acceleration = 0.f;
				}
				else
				{
					target->player_pos_y = target->player_old_y;
					target->y_speed *= 0.f;
					target->y_acceleration = 0.f;
				}
			}
			if ((target->player_pos_y - target->player_half_height) >= lower_bounds &&
				(target->player_pos_y - target->player_half_height) <= up_bounds)
			{
				if (it->finish_block) return 1;

				if (((target->player_old_y + target->player_half_height) >= lower_bounds &&
					(target->player_old_y + target->player_half_height) <= up_bounds) ||
					((target->player_old_y - target->player_half_height) >= lower_bounds &&
						(target->player_old_y - target->player_half_height) <= up_bounds))
				{
					target->player_pos_x = target->player_old_x;
					target->x_speed *= 0.f;
					target->x_acceleration = 0.f;
				}
				else
				{
					target->player_pos_y = target->player_old_y;
					target->y_speed *= 0.f;
					target->y_acceleration = 0.f;
				}
			}
			
		}
		
	}

	return 0;
}

internal void reset_prediction_frames()
{
	for (std::list<Player*>::iterator it = players.begin(); it != players.end(); it++)
	{
		if ((*it)->index != player.index)
		{
			(*it)->past[0] = Frame{ (*it)->player_spawn_x, (*it)->player_spawn_y, (*it)->frame };
			(*it)->past[1] = Frame{ (*it)->player_spawn_x, (*it)->player_spawn_y, (*it)->frame };
			(*it)->past[2] = Frame{ (*it)->player_spawn_x, (*it)->player_spawn_y, (*it)->frame };
		}
	}
}
