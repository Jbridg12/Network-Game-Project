#define SERVER_PORT_UDP 54000
#define SERVER_PORT_TCP 53000
#define SERVER_IP "127.0.0.1"


// TODO  
//
//		- Implement better error checking in network code
//		- Speed problems when moving a lot

struct Message
{
	float newX;
	float newY;
	u_int frame_ID;
	u_int color;
	u_int index;
};
struct WorldUpdate
{
	float start_x;
	float start_y;
	float start_half_width;
	float start_half_height;
	bool start_finish_block = false;

	float finish_x;
	float finish_y;
	float finish_half_width;
	float finish_half_height;
	bool finish_block = true;

	u_int current_gamemode;
};
enum PacketType
{
	NextTurn,
	NewBlock,
	ScoreUpdate,
	NewPlayer,
	PacketTypes
};
sf::IpAddress ip;
unsigned short udp_port; 
unsigned short server_udp_port;
sf::UdpSocket socket_udp;
sf::TcpSocket socket_tcp;

internal void init_server_connection(WorldUpdate* initial_world,
									 Message* initial_player,
									 sf::Packet* other_players,
									 sf::IpAddress new_ip = SERVER_IP,
									 unsigned short new_server_udp_port = SERVER_PORT_UDP
									)
{
	server_udp_port = SERVER_PORT_UDP;
	ip = new_ip;

	sf::Socket::Status status = socket_tcp.connect(ip, SERVER_PORT_TCP);
	if (status != sf::Socket::Done)
	{
		OutputDebugString("wrong");
	}

	// Recieve initial World Status via TCP
	sf::Packet world_init;
	sf::Socket::Status initial_world_update_status = socket_tcp.receive(world_init);
	if (initial_world_update_status == sf::Socket::Partial)
	{
		//while ()
		OutputDebugString("wrong");
	}
	else if (initial_world_update_status != sf::Socket::Done)
	{
		OutputDebugString("These error messages go nowhere but anyway world update failed.\n");
	}
	if (world_init >> initial_world->start_x >> initial_world->start_y >> initial_world->start_half_width >> initial_world->start_half_height
		>> initial_world->finish_x >> initial_world->finish_y >> initial_world->finish_half_width >> initial_world->finish_half_height
		>> initial_world->current_gamemode)
	{
		OutputDebugString("Parsed initial world information.\n");
	}


	// Recieve intial player position via TCP
	sf::Packet player_init;
	sf::Socket::Status initial_player_status = socket_tcp.receive(player_init);
	if (initial_player_status == sf::Socket::Partial)
	{
		//while ()
		OutputDebugString("wrong\n");
	}
	else if (initial_player_status != sf::Socket::Done)
	{
		OutputDebugString("These error messages go nowhere but anyway player start failed.\n");
	}

	int temp_tag;
	player_init >> temp_tag;
	if (player_init >> initial_player->newX >> initial_player->newY >> initial_player->frame_ID >> initial_player->color)
	{
		OutputDebugString("Parsed initial player information.\n");
	}

	u_int index;
	player_init >> index;
	initial_player->index = index;
	udp_port = server_udp_port + 1 + index;

	// bind the socket to a port
	if (socket_udp.bind(udp_port) != sf::Socket::Done)
	{
		printf("UDP bind failed\n");
		return;
	}

	// After first player requires more 
	if (initial_player->index != 0)
	{
		sf::Socket::Status other_players_status = socket_tcp.receive(*other_players);
		if (other_players_status != sf::Socket::Done) 
		{
			return;
		}
	}

	socket_tcp.setBlocking(false);
	socket_udp.setBlocking(false);
	return;
}

internal void update_server_position(sf::Packet update)
{

	unsigned short server_port_udp = SERVER_PORT_UDP;
	if (socket_udp.send(update, ip, server_port_udp) != sf::Socket::Done)
	{
		printf("Send Error\n");
		return;
	}

}

internal void server_send_next_turn(u_int index)
{
	sf::Packet packet;
	packet << PacketType::NextTurn;
	packet << index;

	sf::Socket::Status status = socket_tcp.send(packet);
	if (status == sf::Socket::Done)
	{
		OutputDebugString("Sent a turn update\n");
	}
	return;
}

internal u_int server_turn_update(sf::Packet turn_update, u_int gamemode)
{
	OutputDebugString("Got a turn update\n");
	u_int new_gamemode;
	if (turn_update >> new_gamemode)
	{
		return new_gamemode;
	}
	return gamemode;
		
}

internal void server_send_new_block(float x, float y, float half_width, float half_height)
{
	sf::Packet new_block;
	new_block << PacketType::NewBlock;
	new_block << x << y << half_width << half_height;
	sf::Socket::Status status = socket_tcp.send(new_block);
	if (status == sf::Socket::Done)
	{
		OutputDebugString("Sent a new block \n");
	}
	return;
}

internal u_int server_new_block_update(sf::Packet block_update, float* x, float* y, float* half_width, float* half_height)
{
	if (block_update >> (*x)  >> (*y) >> (*half_width) >> (*half_height))
	{
		return 1;
	}

	return 0;

}
