#define global_variable static
#define internal static


inline int
clamp(u_int min, u_int val, u_int max)
{
	if (val < min) return min;
	if (val > max) return max;
	return val;
}