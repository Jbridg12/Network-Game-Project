#pragma once

struct BUFFER_STATE {
	void* memory;
	int b_height, b_width, buffer_size;

	BITMAPINFO b_bitmapinfo;
};

