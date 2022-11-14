internal void 
draw_rect_in_pixels(u_int x0, u_int y0, u_int x1, u_int y1, u_int color)
{
	x0 = clamp(0, x0, buf.b_width);
	x1 = clamp(0, x1, buf.b_width);
	y0 = clamp(0, y0, buf.b_height);
	y1 = clamp(0, y1, buf.b_height);
	
	for (int y = y0; y < y1; y++)
	{
		u_int* pixel = (u_int*)buf.memory + x0 + y * buf.b_width;

		for (int x = x0; x < x1; x++)
		{
			*pixel++ = color;
		}
	}
}

internal void
draw_rect(float x, float y, float half_x, float half_y, u_int color)
{
	x *= buf.b_height / 2;
	y *= buf.b_height / 2;
	half_x *= buf.b_height / 2;
	half_y *= buf.b_height / 2;

	x += buf.b_width / 2.f;
	y += buf.b_height / 2.f;

	int x0 = x - half_x;
	int x1 = x + half_x;
	int y0 = y - half_y;
	int y1 = y + half_y;

	draw_rect_in_pixels(x0, y0, x1, y1, color);
}


internal void 
clear_screen(u_int color)
{
	u_int* pixel = (u_int*)buf.memory;
	for (int y = 0; y < buf.b_height; y++)
	{
		for (int x = 0; x < buf.b_width; x++)
		{
			*pixel++ = color;
		}
	}
}