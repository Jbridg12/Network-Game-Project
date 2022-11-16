#include <Windows.h>
#include <iostream>
#include <string>
#include <list>
//#include <Winsock2.h>
#include <SFML\System.hpp>
#include <SFML\Network.hpp>
#include "platform.h"
#include "utils.cpp"

global_variable u_short on = 1;
global_variable BUFFER_STATE buf;


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
void run_server(Message* data, Player* player);

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
		run_server(&p1, &player);
		clear_screen(0x00ff11);

		//char stuff[128];
		//sprintf_s(stuff, "X: %f | Y: %f\n", player.player_pos_x, player.player_pos_y);
		//OutputDebugString(stuff);

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

using namespace sf;

struct Connection
{
	TcpSocket* client;
	u_int gamemode;

	// TODO implement id for each client
};

u_int setup_client(TcpSocket* client);
u_int world_update(TcpSocket* client, Packet current_update);
u_int next_turn(Connection* conn);

UdpSocket sock;
TcpListener listener;
std::list<Connection*> clients;
SocketSelector selector;

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
	selector.add(listener);
	clients = {};
}

void run_server(Message* data, Player* player)
{
	size_t received;

	// TCP stuff
	TcpSocket* client = new TcpSocket;
	Socket::Status conn_status = listener.accept(*client);

	if (conn_status == Socket::Error)
	{
		OutputDebugString("Connection failed\n");
	}
	else if (conn_status == Socket::Disconnected)
	{
		OutputDebugString("Socket Disconnection\n");
	}
	else if (conn_status == Socket::NotReady)
	{
		OutputDebugString("No Connection\n");
	}
	else if (conn_status == Socket::Done)
	{
		OutputDebugString("Client Connected\n");
		
		setup_client(client);
		
		Connection* conn = new Connection;
		conn->client = client;
		conn->gamemode = 1;
		clients.push_back(conn);

	}
	
	Message end_turn_message;
	for (std::list<Connection*>::iterator it = clients.begin(); it != clients.end(); it++)
	{
		Packet packet;
		Socket::Status recv =  (*it)->client->receive(packet);
		if (recv == Socket::Error)
		{
 			OutputDebugString("Unexpected error\n");
			//WSAGetLastError();
		}
		else if (recv == Socket::Disconnected)
		{
			//OutputDebugString("Socket Disconnection\n");
		}
		else if (recv == Socket::NotReady)
		{
			OutputDebugString("Socket out of data\n");
		}
		else if (recv == Socket::Done)
		{
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
			
		}
	}


	// TODO implement constant server loop features
	// Including fix UDP

	// UDP Stuff

	unsigned short serverPort = SERVER_PORT_UDP;
	Packet packet;
	IpAddress sender;
	Socket::Status recv_udp = sock.receive(packet, sender, serverPort);

	if (recv_udp == Socket::Error)
	{
		//OutputDebugString("Receive failed\n");
		return;
	}
	else if (recv_udp == Socket::Disconnected)
	{
		//OutputDebugString("Socket Disconnection\n");
		return;
	}
	else if (recv_udp == Socket::NotReady)
	{
		//OutputDebugString("Socket out of data\n");
	}
	else if (recv_udp == Socket::Done)
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

u_int setup_client(TcpSocket* client)
{

	Packet current_update;
	current_update << start.x << start.y << start.half_width << start.half_height << finish.x << finish.y << finish.half_width << finish.half_height << 1;
	world_update(client, current_update);


	Packet player_start;
	player_start << (float)(-1.6f + (clients.size() * 0.2)) << -0.5f << 0 << clients.size();
	//player_start << 1.f << 1.f << 1 << 1 << 1;
	Socket::Status send_status = client->send(player_start);
	if (send_status == Socket::Partial)
	{
		while (client->send(&player_start, sizeof(Message)) == Socket::Partial)
		{
			//OutputDebugString("Only Parts\n");
		}
	}
	if (send_status != Socket::Done)
	{
		//OutputDebugString("These error messages go nowhere but anyway world update failed.");
		return -1;
	}

	client->setBlocking(false);
	//client->setBlocking(true);
	return 1;
}

u_int world_update(TcpSocket* client, Packet current_update)
{
	
	Socket::Status send_status = client->send(current_update);
	if (send_status == Socket::Partial)
	{
		while (client->send(current_update) == Socket::Partial)
		{
			//OutputDebugString("Only Parts\n");
		}
	}
	if (send_status != Socket::Done)
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
	Packet next_turn_message;
	next_turn_message << conn->gamemode;

	Socket::Status send = conn->client->send(next_turn_message);
	if (send == Socket::Error)
	{
		//OutputDebugString("Receive failed\n");
		return -1;
	}
	else if (send == Socket::Disconnected)
	{
		//OutputDebugString("Socket Disconnection\n");
		return -1;
	}
	else if (send == Socket::NotReady)
	{
		//OutputDebugString("Socket out of data\n");
		return -1;
	}

	return 1;
}