#include <stdio.h>

class Point
{
	double _pt[2];
public:
	double& x;
	double& y;
	
	Point()			 : x(_pt[0]), y(_pt[1]) { clear();}
	Point(double X, double Y): x(_pt[0]), y(_pt[1]) { set(X,Y);}
	
	void clear()			{ x=0; y=0;}
	void set(double X, double Y)	{ x=X; y=Y;}
	
	double& operator[](int i)
	{
		switch (i)
		{
		case 0: return _pt[0];
		case 1: return _pt[1];
		default: return _pt[0]; //prevent segmentation fault
		}
	}
	
	static void hi()
	{
		printf("hi\n");
	}
};

int main()
{
	Point P1;
	P1.set(1.2,2.2);
	printf( "P1.x=%f, P1.y=%f\n", P1.x,P1.y);
	
	Point P2(10.1,10.2);
	printf( "P2.x=%f, P2.y=%f\n", P2.x,P2.y);
	
	Point P3;
	for ( int i=0; i<2; i++)
		P3[i]=4.4;
	printf( "P3.x=%f, P3.y=%f\n", P3.x,P3.y);
	
	P3.hi();
	
	return 1;
}
