#include <stdio.h>

struct Point { double x,y;};

Point sum()
{
	Point a = {1.5,2.5};
	return a;
}

int main()
{
	Point a = sum();
	printf( "a(%f,%f)\n", a.x,a.y);
	
	return 1;
}
