#define NOMINMAX

#include <Windows.h>
#include <iostream>
#include <string>
#include <list>
//#include <Winsock2.h>
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
	TODO: Implement Packets with SFML for communication.

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
	int frame_num;

};

#include "render.cpp"
#include "game.cpp"

void init_server();
void run_server(Player* player);

LRESULT CALLBACK winCallback(HWND hwnd,	UINT uMsg,	WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;

	switch (uMsg) {
		case WM_CLOSE:{}
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
	Player player;
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

	if (!font.loadFromFile("Chunk Five Print.otf"))
	{
		OutputDebugString("Failed loading font\n");
	}

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


				} break;
				default: {
					TranslateMessage(&message);
					DispatchMessage(&message);
				}
					
			}
			
		}

		// Simulate
		//run_game(&input, delta_time);
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
		draw_rect(player.player_pos_x, player.player_pos_y, 0.04f, 0.04f, 0xff00ff);

		

		// Render
		StretchDIBits(hdc, 0, 0, buf.b_width, buf.b_height, 0, 0, buf.b_width, buf.b_height, buf.memory, &buf.b_bitmapinfo, DIB_RGB_COLORS, SRCCOPY);


		LARGE_INTEGER frame_end_time;
		QueryPerformanceCounter(&frame_end_time);
		delta_time = (float)(frame_end_time.QuadPart - frame_begin_time.QuadPart) / performance_frequency;
		frame_begin_time = frame_end_time;
	}
	
} 


#define SERVER_PORT_UDP 54000
#define SERVER_POT_TCP 53000
#define SERVER_IP "127.0.0.1"


struct Connection
{
	sf::TcpSocket* client;
	u_int gamemode;

	// TODO implement id for each client
};
enum PacketType
{
	NextTurn,
	NewBlock,
	ScoreUpdate,
	PacketTypes
};
u_int setup_client(sf::TcpSocket* client);
u_int world_update(sf::TcpSocket* client, sf::Packet current_update);
u_int next_turn(Connection* conn);

sf::UdpSocket sock;
sf::TcpListener listener;
std::list<Connection*> clients;
sf::SocketSelector selector;

void init_server()
{
	sock.setBlocking(false);
	listener.setBlocking(false);

	// bind the socket to a port
	if (sock.bind(SERVER_PORT_UDP) != sf::Socket::Done)
	{
		printf("UDP bind failed\n");
		return;
	}

	if (listener.listen(SERVER_POT_TCP) != sf::Socket::Done)
	{
		printf("TCP bind failed\n");
		return;
	}
	clients = {};
}

void run_server(Player* player)
{
	size_t received;

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
		OutputDebugString("No Connection\n");
	}
	else if (conn_status == sf::Socket::Done)
	{
		OutputDebugString("Client Connected\n");
		
		setup_client(client);
		
		Connection* conn = new Connection;
		conn->client = client;
		conn->gamemode = 1;
		clients.push_back(conn);

	}
	
	for (std::list<Connection*>::iterator it = clients.begin(); it != clients.end(); it++)
	{
		sf::Packet packet;
		sf::Socket::Status recv =  (*it)->client->receive(packet);
		if (recv == sf::Socket::Error)
		{
 			OutputDebugString("Unexpected error\n");
			//WSAGetLastError();
		}
		else if (recv == sf::Socket::Disconnected)
		{
			//OutputDebugString("Socket Disconnection\n");
		}
		else if (recv == sf::Socket::NotReady)
		{
			OutputDebugString("Socket out of data\n");
		}
		else if (recv == sf::Socket::Done)
		{
			int header;
			if (packet >> header)
			{
				switch (header)
				{
					case (PacketType::NextTurn):
						OutputDebugString("Next Turn\n");

						u_int index;
						if (packet >> index)
						{
							// TEMP
							(*it)->gamemode = ((*it)->gamemode + 1) % 3;
							if ((index + 1) < clients.size())
							{
								// TODO: Send next client in list the signal for next turn
							}

							next_turn(*it);
						}
						break;
					case (PacketType::NewBlock):
						float x, y, half_width, half_height;
						if (packet >> x >> y >> half_width >> half_height)
						{
							PlacedBlock new_block{ x, y, half_width, half_height };
							all_game_blocks.push_back(new_block);
						}
						break;
					case (PacketType::ScoreUpdate):
						break;
				}
			}
			
			
			
		}
	}


	// TODO implement constant server loop features
	// Including fix UDP

	// UDP Stuff

	unsigned short serverPort = SERVER_PORT_UDP;
	sf::Packet packet;
	sf::IpAddress sender;
	sf::Socket::Status recv_udp = sock.receive(packet, sender, serverPort);

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
		//OutputDebugString("Socket out of data\n");
	}
	else if (recv_udp == sf::Socket::Done)
	{
		float newX;
		float newY;

		if (packet >> newX >> newY)
		{
			player->player_pos_x = newX;
			player->player_pos_y = newY;
		}
		
	}
	
}

u_int setup_client(sf::TcpSocket* client)
{

	sf::Packet current_update;
	current_update << start.x << start.y << start.half_width << start.half_height << finish.x << finish.y << finish.half_width << finish.half_height << 1;
	world_update(client, current_update);


	sf::Packet player_start;
	player_start << (float)(-1.6f + (clients.size() * 0.2)) << -0.5f << 0 << clients.size();
	//player_start << 1.f << 1.f << 1 << 1 << 1;
	sf::Socket::Status send_status = client->send(player_start);
	if (send_status == sf::Socket::Partial)
	{
		while (client->send(&player_start, sizeof(Message)) == sf::Socket::Partial)
		{
			//OutputDebugString("Only Parts\n");
		}
	}
	if (send_status != sf::Socket::Done)
	{
		//OutputDebugString("These error messages go nowhere but anyway world update failed.");
		return -1;
	}

	client->setBlocking(false);
	//client->setBlocking(true);
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
	// TODO: make new struct for each type
	//		 of message the user sends/recieves
	sf::Packet next_turn_message;
	next_turn_message << conn->gamemode;

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