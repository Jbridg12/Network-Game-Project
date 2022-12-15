#define NOMINMAX
#include <Windows.h>
#include <SFML\System.hpp>
#include <SFML\Network.hpp>
#include <SFML\Graphics.hpp>
#include <iterator>
#include <string>
#include <list>
#include <cmath> 
#include <algorithm>
#include "platform.h"
#include "utils.cpp"

using namespace std;
global_variable u_short on = 1;
global_variable BUFFER_STATE buf;

#include "render.cpp"
#include "input.cpp"
#include "server.cpp"
#include "game.cpp"

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
		case WM_ERASEBKGND:
			return (LRESULT)1;
		default: {
			result = DefWindowProc(hwnd, uMsg, wParam, lParam);
		}
			
	}

	return result;
}

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	// Create Window Class

	WNDCLASS window_class = {};
	window_class.style = CS_HREDRAW | CS_VREDRAW;
	window_class.lpszClassName = "Game Window Class";
	window_class.lpfnWndProc = winCallback;

	// Register Window
	RegisterClass(&window_class);
	init_game();

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

	Input input = {};
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

		for (int i = 0; i < BUTTON_COUNT; i++)
		{
			input.buttons[i].changed = false;
		}

		while (PeekMessage(&message, window, 0, 0, PM_REMOVE))
		{

			switch (message.message)
			{
				case WM_KEYUP: {

				}
				case WM_KEYDOWN: {
					u_int vk_code = (u_int)message.wParam;
					bool is_down = ((message.lParam & (1 << 31)) == 0);


					// Macro for case management
					#define handle_input(b, vk)\
					case vk: {\
					input.buttons[b].changed = (is_down != input.buttons[b].is_down); \
					input.buttons[b].is_down = is_down; \
					} break;

					switch (vk_code) {
						handle_input(BUTTON_UP, VK_UP);
						handle_input(BUTTON_DOWN, VK_DOWN);
						handle_input(BUTTON_LEFT, VK_LEFT);
						handle_input(BUTTON_RIGHT, VK_RIGHT); 
						handle_input(BUTTON_W, 'W');
						handle_input(BUTTON_A, 'A');
						handle_input(BUTTON_S, 'S');
						handle_input(BUTTON_D, 'D');
						handle_input(BUTTON_Z, 'Z');
						handle_input(BUTTON_SPACE, VK_SPACE);

					}


				} break;
				default: {
					TranslateMessage(&message);
					DispatchMessage(&message);
				}
					
			}
			
		}
		
		// Simulate
		run_game(&input, delta_time);

		// Render
		StretchDIBits(hdc, 0, 0, buf.b_width, buf.b_height, 0, 0, buf.b_width, buf.b_height, buf.memory, &buf.b_bitmapinfo, DIB_RGB_COLORS, SRCCOPY);


		LARGE_INTEGER frame_end_time;
		QueryPerformanceCounter(&frame_end_time);
		delta_time = (float)(frame_end_time.QuadPart - frame_begin_time.QuadPart) / performance_frequency;
		frame_begin_time = frame_end_time;
	}
	
	//ReleaseDC(window, hdc);

} 

