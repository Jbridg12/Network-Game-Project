#define NOMINMAX

#include <Windows.h>
#include <iostream>
#include <iterator>
#include <string>
#include <list>
#include <algorithm>
#include <SFML\System.hpp>
#include <SFML\Network.hpp>
#include <SFML\Window.hpp>
#include <SFML\Graphics.hpp>
#include "platform.h"
#include "utils.cpp"

global_variable u_short on = 1;
global_variable BUFFER_STATE buf;
sf::Font font;

/*
	TODO: 
	- Implement cleanup for structures and connections
	- LINE 436 
*/


struct Message
{
	float newX;
	float newY;
	u_int frameID;
	u_int index;
};

struct Player
{
	float player_pos_x;
	float player_pos_y;
	u_int frame_num;
	u_int color;
	u_int index;
};

#include "render.cpp"
#include "game.cpp"

void init_server();
void run_server(Player* player);

#define SERVER_PORT_UDP 54000
#define SERVER_PORT_TCP 53000
#define SERVER_IP "127.0.0.1"
#define RESET_TIMER 600
#define MAX_ROUND 3

struct Connection
{
	sf::TcpSocket* client;
	unsigned short client_UDP;
	u_int gamemode;

	Player player;
	u_int score;
};
enum PacketType
{
	NextTurn,
	NewBlock,
	ScoreUpdate,
	NewPlayer,
	EndOfGame,
	Reset,
	Disconnect,
	PacketTypes
};

u_int setup_client(Connection* conn, bool initial = true);
u_int world_update(sf::TcpSocket* client, sf::Packet current_update);
u_int next_turn(Connection* conn);
void end_game(Connection* conn, u_int result);
void reset(Connection* conn);
void update_player_position(u_int index, u_int frame, float newX, float newY);
void send_new_block(u_int index, float x, float y, float half_width, float half_height);
void send_score_update();
void send_disconnect(u_int index);

Player player;

u_int current_round;
u_int high_score;
u_int tie;
u_int timer;
bool end_of_game;


sf::UdpSocket socket_udp;
sf::TcpListener listener;
std::list<Connection*> clients;
std::list<u_int> available_index = {0, 1, 2, 3};

u_short server_udp_port = SERVER_PORT_UDP;
u_int colors[4] = {0xf6c345, 0xde193e, 0x25b4ff, 0x8a2be2};

void init_server()
{
	socket_udp.setBlocking(false);
	listener.setBlocking(false);

	// bind the socket to a port
	if (socket_udp.bind(SERVER_PORT_UDP) != sf::Socket::Done)
	{
		printf("UDP bind failed\n");
		return;
	}
	//sf::IpAddress public_address = sf::IpAddress::getPublicAddress(sf::seconds(3));
	if (listener.listen(SERVER_PORT_TCP) != sf::Socket::Done)
	{
		printf("TCP bind failed\n");
		return;
	}

	clients = {};
	current_round = 0;
	high_score = 0;
	tie = 0;
	timer = 0;
	end_of_game = false;
}

void run_server(Player* player)
{

	// TCP stuff
	sf::TcpSocket* client = new sf::TcpSocket;
	sf::Socket::Status conn_status = listener.accept(*client);

	if (conn_status == sf::Socket::Error)
	{
		OutputDebugString("Connection failed\n");
	}
	else if (conn_status == sf::Socket::Disconnected)
	{
		OutputDebugString("Socket Disconnection\n");
	}
	else if (conn_status == sf::Socket::NotReady)
	{
		OutputDebugString("No New Connection\n");
	}
	else if (conn_status == sf::Socket::Done)
	{
		OutputDebugString("Client Connected\n");
		if (clients.size() < 4)
		{
			Connection* conn = new Connection;
			u_int index = available_index.front();
			available_index.pop_front();
			conn->client = client;
			conn->client_UDP = SERVER_PORT_UDP + 1 + index;
			conn->score = 0;
			conn->gamemode = 0;
			conn->player = { (float)(-1.6f + (index * 0.2)), -0.5f, 0, colors[index], index };
			setup_client(conn);

			clients.insert(std::next(clients.begin(), index), conn);
		}
		
	}

	std::list<Connection*>::iterator it = clients.begin();
	while (it != clients.end())
	{
		sf::Packet packet;
		sf::Socket::Status recv =  (*it)->client->receive(packet);
		if (recv == sf::Socket::Error)
		{
 			OutputDebugString("Unexpected error\n");
		}
		else if (recv == sf::Socket::Disconnected)
		{
			OutputDebugString("Socket Disconnection\n");
			available_index.push_back((*it)->player.index);
			available_index.sort();
			if ((*it)->gamemode != 0)
			{
				if ((*it) == clients.back())
				{
					clients.front()->gamemode = 3 - (*it)->gamemode;;
					next_turn(clients.front());
				}
				else
				{
					std::list<Connection*>::iterator next = std::next(it);
					(*next)->gamemode = (*it)->gamemode;
					next_turn(*next);
				}
			}
			send_disconnect((*it)->player.index);
			it = clients.erase(it);

		}
		else if (recv == sf::Socket::NotReady)
		{
			OutputDebugString("Socket out of data\n");
			it++;
		}
		else if (recv == sf::Socket::Done)
		{
			int header;
			int end_mode = 0;
			if (packet >> header)
			{
				switch (header)
				{
					case (PacketType::NextTurn):
						OutputDebugString("Next Turn\n");

						u_int index;
						if (packet >> index)
						{
							if (clients.size() > 1)
							{
								u_int next_index = index + 1;
								while (std::find(available_index.begin(), available_index.end(), next_index) != available_index.end())
								{
									next_index++;
								}

								u_int next_gamemode = (*it)->gamemode;
								if (next_index > clients.back()->player.index)
								{
									if ((*it)->gamemode == 2)
									{
										current_round++;
										if (current_round >= MAX_ROUND)
										{
											end_of_game = 1;
											for (std::list<Connection*>::iterator tie_check = clients.begin(); tie_check != clients.end(); tie_check++)
											{
												if ((*tie_check)->score == high_score) tie++;
											}
											for (std::list<Connection*>::iterator it2 = clients.begin(); it2 != clients.end(); it2++)
											{
												if ((*it2)->score < high_score)
												{
													end_game(*it2, 0);

												}
												else
												{
													if (tie > 1)
													{
														end_game(*it2, 2);
													}
													else
													{
														end_game(*it2, 1);
													}
												}
											}

											return;

										}

									}
									
									next_index = (*clients.begin())->player.index;
									next_gamemode = 3 - (*it)->gamemode;
									
								}

								std::list<Connection*>::iterator it2 = clients.begin();
								while ((*it2)->player.index != next_index)
								{
									it2++;
								}

								// Send to next player
								(*it2)->gamemode = next_gamemode;
								next_turn(*it2);

								// Send to current player
								(*it)->gamemode = 0;
								next_turn(*it);
								
							}
							else
							{
								OutputDebugString("Don't play by yourself\n");
								return;
							}

							
						}
						break;
					case (PacketType::NewBlock):
						float x, y, half_width, half_height;
						if (packet >> x >> y >> half_width >> half_height)
						{
							PlacedBlock new_block{ x, y, half_width, half_height };
							all_game_blocks.push_back(new_block);
							send_new_block((*it)->player.index, x, y, half_width, half_height);
						}
						break;
					case (PacketType::ScoreUpdate):
						if ((*it)->score == high_score) high_score++;
						(*it)->score++;
						send_score_update();
						break;
				}
			}
			
			it++;
		}
	}

	// UDP Stuff
	for (std::list<Connection*>::iterator uit = clients.begin(); uit != clients.end(); uit++)
	{
		unsigned short clientPort = (*uit)->client_UDP;
		sf::Packet packet;
		sf::IpAddress sender = SERVER_IP;
		sf::Socket::Status recv_udp = socket_udp.receive(packet, sender, clientPort);

		if (recv_udp == sf::Socket::Error)
		{
			//OutputDebugString("Receive failed\n");
			return;
		}
		else if (recv_udp == sf::Socket::Disconnected)
		{
			//OutputDebugString("Socket Disconnection\n");
			return;
		}
		else if (recv_udp == sf::Socket::NotReady)
		{
			OutputDebugString("Not recieving position update\n");
		}
		else if (recv_udp == sf::Socket::Done)
		{
			float newX;
			float newY;
			u_int index;
			u_int frame;
			if (packet >> index >> newX >> newY >> frame)
			{
				std::list<Connection*>::iterator it2 = clients.begin();
				while ((*it2)->player.index != index)
				{
					it2++;
				}

				if (frame > (*it2)->player.frame_num)
				{
					(*it2)->player.frame_num = frame;
					(*it2)->player.player_pos_x = newX;
					(*it2)->player.player_pos_y = newY;

					update_player_position(index, frame, newX, newY);
				}
			}



		}
	}
	
	if (end_of_game)
	{
		if (timer++ >= RESET_TIMER)
		{
			init_game();
			for (std::list<Connection*>::iterator it2 = clients.begin(); it2 != clients.end(); it2++)
			{
				reset(*it2);
				(*it2)->player.player_pos_x = (float)(-1.6f + ((*it2)->player.index * 0.2));
				(*it2)->player.player_pos_y = -0.5f;
				(*it2)->player.frame_num = 0;
				(*it2)->score = 0;
				(*it2)->gamemode = (it2 == clients.begin()) ? 1 : 0;
				setup_client((*it2), false);
			}

			current_round = 0;
			high_score = 0;
			tie = 0;
			timer = 0;
			end_of_game = false;

			(*clients.begin())->gamemode = 1;
			next_turn(*clients.begin());

		}
	}


}

u_int setup_client(Connection* conn, bool initial)
{
	sf::Packet initial_world;
	initial_world << 0;
	initial_world << (int) all_game_blocks.size(); // Number of blocks to expect
	for (std::list<PlacedBlock>::iterator it = all_game_blocks.begin(); it != all_game_blocks.end(); it++)
	{
		initial_world << (*it).x << (*it).y << (*it).half_width << (*it).half_height << (*it).finish_block;
	}

	sf::Socket::Status world_send_status = conn->client->send(initial_world);
	if (world_send_status == sf::Socket::Partial)
	{
		while (conn->client->send(initial_world) == sf::Socket::Partial)
		{
			//OutputDebugString("Only Parts\n");
		}
	}
	if (world_send_status != sf::Socket::Done)
	{
		OutputDebugString("These error messages go nowhere but anyway world update failed.");
		return -1;
	}

	if (initial)
	{
		sf::Packet player_start;
		player_start << PacketType::NewPlayer << conn->player.player_pos_x << conn->player.player_pos_y << conn->player.frame_num << conn->player.color << conn->player.index;
		sf::Socket::Status send_status = conn->client->send(player_start);
		if (send_status == sf::Socket::Partial)
		{
			//while (conn->client->send(&player_start, sizeof(Message)) == sf::Socket::Partial)
			//{
				//OutputDebugString("Only Parts\n");
			//}
			OutputDebugString("Only Parts\n");
		}
		if (send_status != sf::Socket::Done)
		{
			OutputDebugString("These error messages go nowhere but anyway world update failed.");
			return 0;
		}


		// Send other players to initial player as well check if it exists in the game code, have a flag to indicate whether to expect them or not
		sf::Packet all_players;

		// Update all other clients to recognize have new player in game
		if (clients.size())
		{
			all_players << true;
			all_players << (u_int)clients.size();
			for (std::list<Connection*>::iterator it = clients.begin(); it != clients.end(); it++)
			{
				sf::Socket::Status send_status = (*it)->client->send(player_start);
				all_players << (*it)->player.player_pos_x << (*it)->player.player_pos_y << (*it)->player.frame_num << (*it)->player.color << (*it)->player.index;
			}

			
			sf::Socket::Status send_status2 = conn->client->send(all_players);
			if (send_status != sf::Socket::Done)
			{
				//OutputDebugString("These error messages go nowhere but anyway world update failed.");
				return -1;
			}

		}
		else
		{
			all_players << false;
			sf::Socket::Status send_status2 = conn->client->send(all_players);
			if (send_status != sf::Socket::Done)
			{
				//OutputDebugString("These error messages go nowhere but anyway world update failed.");
				return -1;
			}
		}

			conn->client->setBlocking(false);
		}
		

	return 1;
}

u_int world_update(sf::TcpSocket* client, sf::Packet current_update)
{
	
	sf::Socket::Status send_status = client->send(current_update);
	if (send_status == sf::Socket::Partial)
	{
		while (client->send(current_update) == sf::Socket::Partial)
		{
			//OutputDebugString("Only Parts\n");
		}
	}
	if (send_status != sf::Socket::Done)
	{
		//OutputDebugString("These error messages go nowhere but anyway world update failed.");
		return -1;
	}

	return 1;
}

u_int next_turn(Connection* conn)
{
	sf::Packet next_turn_message;
	next_turn_message << PacketType::NextTurn << conn->gamemode;

	sf::Socket::Status send = conn->client->send(next_turn_message);
	if (send == sf::Socket::Error)
	{
		//OutputDebugString("Receive failed\n");
		return -1;
	}
	else if (send == sf::Socket::Disconnected)
	{
		//OutputDebugString("Socket Disconnection\n");
		return -1;
	}
	else if (send == sf::Socket::NotReady)
	{
		//OutputDebugString("Socket out of data\n");
		return -1;
	}

	return 1;
}

void end_game(Connection* conn, u_int result)
{
	sf::Packet end_game;
	end_game << PacketType::EndOfGame << result;

	sf::Socket::Status send = conn->client->send(end_game);
	if (send != sf::Socket::Done)
	{
		OutputDebugString("Send failed\n");
		return;
	}
}

void reset(Connection* conn)
{
	sf::Packet reset;
	reset << PacketType::Reset;

	sf::Socket::Status send = conn->client->send(reset);
	if (send != sf::Socket::Done)
	{
		OutputDebugString("Send failed\n");
		return;
	}
}

void update_player_position(u_int index, u_int frame, float newX, float newY)
{
	sf::Packet update;
	update << index << newX << newY << frame;
	for (std::list<Connection*>::iterator it = clients.begin(); it != clients.end(); it++)
	{
		if ((*it)->player.index == index) continue;
		if (socket_udp.send(update, SERVER_IP, (*it)->client_UDP) != sf::Socket::Done)
		{
			printf("Send Error\n");
			return;
		}
	}
}

void send_new_block(u_int index, float x, float y, float half_width, float half_height)
{
	sf::Packet update;
	update << PacketType::NewBlock << x << y << half_width << half_height;
	for (std::list<Connection*>::iterator it = clients.begin(); it != clients.end(); it++)
	{
		if ((*it)->player.index == index) continue;
		if ((*it)->client->send(update) != sf::Socket::Done)
		{
			printf("Send Error\n");
			return;
		}
	}
}

void send_score_update()
{
	sf::Packet update;
	update << PacketType::ScoreUpdate;
	for (std::list<Connection*>::iterator it = clients.begin(); it != clients.end(); it++)
	{
		update << (*it)->score;
	}
	for (std::list<Connection*>::iterator it = clients.begin(); it != clients.end(); it++)
	{
		if ((*it)->client->send(update) != sf::Socket::Done)
		{
			printf("Send Error\n");
			return;
		}
	}
}

void send_disconnect(u_int index)
{
	sf::Packet update;
	update << PacketType::Disconnect << index;
	for (std::list<Connection*>::iterator it = clients.begin(); it != clients.end(); it++)
	{
		if ((*it)->player.index == index) continue;
		if ((*it)->client->send(update) != sf::Socket::Done)
		{
			printf("Send Error\n");
			return;
		}
	}
}

// Windows Stuff
LRESULT CALLBACK winCallback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;

	switch (uMsg) {
	case WM_CLOSE: {}
	case WM_DESTROY: {
		on = 0;
	} break;
	case WM_SIZE: {
		RECT screen;
		GetClientRect(hwnd, &screen);
		buf.b_height = screen.bottom - screen.top;
		buf.b_width = screen.right - screen.left;

		buf.buffer_size = buf.b_width * buf.b_height * sizeof(u_int);

		if (buf.memory) VirtualFree(buf.memory, 0, MEM_RELEASE);
		buf.memory = VirtualAlloc(0, buf.buffer_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

		buf.b_bitmapinfo.bmiHeader.biSize = sizeof(buf.b_bitmapinfo.bmiHeader);
		buf.b_bitmapinfo.bmiHeader.biWidth = buf.b_width;
		buf.b_bitmapinfo.bmiHeader.biHeight = buf.b_height;
		buf.b_bitmapinfo.bmiHeader.biPlanes = 1;
		buf.b_bitmapinfo.bmiHeader.biBitCount = 32;
		buf.b_bitmapinfo.bmiHeader.biCompression = BI_RGB;

	}

	default: {
		result = DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	}

	return result;
}

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	// Create Player to Draw
	
	player.player_pos_x = 0.f;
	player.player_pos_y = 0.f;



	// Create Window Class

	WNDCLASS window_class = {};
	window_class.style = CS_HREDRAW | CS_VREDRAW;
	window_class.lpszClassName = "Game Window Class";
	window_class.lpfnWndProc = winCallback;

	// Register Window
	RegisterClass(&window_class);
	init_game();
	init_server();

	// Draw It
	HWND window = CreateWindowA(window_class.lpszClassName,
		"The Game Screen",
		WS_VISIBLE | WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		1280,
		720,
		0,
		0,
		hInstance,
		0
	);

	HDC hdc = GetDC(window);

	float delta_time = 0.0166666f;
	LARGE_INTEGER frame_begin_time;
	QueryPerformanceCounter(&frame_begin_time);

	float performance_frequency;
	{
		LARGE_INTEGER perf;
		QueryPerformanceFrequency(&perf);
		performance_frequency = (float)perf.QuadPart;
	}

	while (on)
	{
		// Input
		MSG message;

		while (PeekMessage(&message, window, 0, 0, PM_REMOVE))
		{

			switch (message.message)
			{
			case WM_KEYUP: {

			}
			case WM_KEYDOWN: {
				u_int vk_code = (u_int)message.wParam;
				bool is_down = ((message.lParam & (1 << 31)) == 0);

				if (vk_code == VK_SPACE)
				{
					// START THE GAME
					if (clients.size() >= 2)
					{
						std::list<Connection*>::iterator it = clients.begin();
						(*it)->gamemode = 1;
						next_turn(*it);
					}
				}

			} break;
			default: {
				TranslateMessage(&message);
				DispatchMessage(&message);
			}

			}

		}

		// Simulate
		Message p1;
		run_server(&player);
		clear_screen(0x00ff11);


		for (std::list<PlacedBlock>::iterator it = all_game_blocks.begin(); it != all_game_blocks.end(); ++it)
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
		for (std::list<Connection*>::iterator it2 = clients.begin(); it2 != clients.end(); ++it2)
		{
			draw_rect((*it2)->player.player_pos_x, (*it2)->player.player_pos_y, 0.04f, 0.04f, (*it2)->player.color);
		}


		// Render
		StretchDIBits(hdc, 0, 0, buf.b_width, buf.b_height, 0, 0, buf.b_width, buf.b_height, buf.memory, &buf.b_bitmapinfo, DIB_RGB_COLORS, SRCCOPY);


		LARGE_INTEGER frame_end_time;
		QueryPerformanceCounter(&frame_end_time);
		delta_time = (float)(frame_end_time.QuadPart - frame_begin_time.QuadPart) / performance_frequency;
		frame_begin_time = frame_end_time;
	}

}