struct BetterRectangle
{
	float x0;
	float y0;
	float x1;
	float y1;
};

internal BetterRectangle
adjust_to_screen(float x, float y, float half_x, float half_y)
{
	x *= buf.b_height / 2;
	y *= buf.b_height / 2;
	half_x *= buf.b_height / 2;
	half_y *= buf.b_height / 2;

	x += buf.b_width / 2.f;
	y += buf.b_height / 2.f;
	BetterRectangle res;

	res.x0 = x - half_x;
	res.x1 = x + half_x;
	res.y0 = y - half_y;
	res.y1 = y + half_y;

	return res;
}

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
	BetterRectangle rect = adjust_to_screen(x, y, half_x, half_y);

	draw_rect_in_pixels(rect.x0, rect.y0, rect.x1, rect.y1, color);
}

internal void
draw_checkered_block(float x, float y, float half_x, float half_y, bool darken)
{
	BetterRectangle rect = adjust_to_screen(x, y, half_x, half_y);
	float x_slope = (rect.x1 - rect.x0) / 4;
	float y_middle = rect.y0 + ((rect.y1 - rect.y0) / 2);
	u_int color = 0x00;
	for (int i = 0; i < 4; i++)
	{
		float x0_subsquare = rect.x0 + x_slope * i;
		float x1_subsquare = rect.x0 + x_slope * (i + 1);
		if (darken)
		{
			draw_rect_in_pixels(x0_subsquare, rect.y0, x1_subsquare, y_middle, 0);
			draw_rect_in_pixels(x0_subsquare, y_middle, x1_subsquare, rect.y1, 0);
		}
		else
		{
			draw_rect_in_pixels(x0_subsquare, rect.y0, x1_subsquare, y_middle, color);
			color = 16777215 - color;
			draw_rect_in_pixels(x0_subsquare, y_middle, x1_subsquare, rect.y1, color);
		}
		
	}
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