#define SERVER_PORT_UDP 54000
#define SERVER_PORT_TCP 53000
#define SERVER_IP "127.0.0.1"


// TODO  - restructure heirarchy to better fit data structures
//		aka move some structs back to game.cpp and abstract the
//		references
//
//		- Implement better error checking in network code
//



struct Message
{
	float newX;
	float newY;
	int frame_ID;
	u_int index;
	u_int gamemode;
};

struct PlacedBlock
{
	float x;
	float y;
	float half_width;
	float half_height;
	float speed = 0.005f;

	bool finish_block = false;
};

struct WorldUpdate
{
	// Needs to not require PlaceBlock defined here
	// TODO later
	//std::list<PlacedBlock> all_blocks;
	PlacedBlock start;
	PlacedBlock finish;
	u_int current_gamemode;
};

enum GAMEMODE
{
	Waiting,
	Playing,
	Placing,
	Gamemode_Count
};

using namespace sf;
IpAddress ip;
unsigned short udp_port;
TcpSocket socket_tcp;

internal void init_server_connection(WorldUpdate* initial_world,
									Message* initial_player,
									IpAddress new_ip = SERVER_IP,
									unsigned short new_udp_port = SERVER_PORT_UDP
									)
{
	ip = new_ip;
	udp_port = new_udp_port;

	Socket::Status status = socket_tcp.connect(ip, SERVER_PORT_TCP);
	if (status != Socket::Done)
	{
		OutputDebugString("wrong");
	}

	// Recieve initial World Status via TCP
	std::size_t received;
	Socket::Status initial_world_update_status = socket_tcp.receive(initial_world, sizeof(WorldUpdate), received);
	if (initial_world_update_status == Socket::Partial)
	{
		//while ()
		printf("wrong");
	}
	if (initial_world_update_status != Socket::Done)
	{
		printf("These error messages go nowhere but anyway world update failed.");
	}

	// Recieve intial player position via TCP
	Socket::Status initial_player_status = socket_tcp.receive(initial_player, sizeof(Message), received);
	if (initial_player_status == Socket::Partial)
	{
		//while ()
		OutputDebugString("wrong");
	}
	if (initial_player_status != Socket::Done)
	{
		OutputDebugString("These error messages go nowhere but anyway player start failed.");
	}


	socket_tcp.setBlocking(false);
	return;
}

internal void update_server_position(Message update)
{
	UdpSocket socket;

	if (socket.send(&update, sizeof(Message), ip, udp_port) != Socket::Done)
	{
		printf("Send Error\n");
		return;
	}

}

internal void server_send_next_turn(u_int index)
{
	Packet packet;
	packet << index;

	Socket::Status status = socket_tcp.send(packet);
	if (status == Socket::Done)
	{
		OutputDebugString("Sent a turn update\n");
	}
	return;
}

internal void server_turn_update(GAMEMODE* gamemode)
{
	Message turn_update;
	size_t received;

	Socket::Status status = socket_tcp.receive(&turn_update, sizeof(Message), received);
	if (status == Socket::Done)
	{
		OutputDebugString("Got a turn update\n");
		*gamemode = (GAMEMODE) turn_update.gamemode;
	}
	return;
}
