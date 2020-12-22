#include <stdio.h>

int f(int x)
{
	int y = x;
	int z = y + 1;
	return z;
}

int main()
{
    int a = 1 + 2 + 3;
    int b = 5;
    int c = a - b;
    int d, e;
    int g;

    if(c < 5)
    {
        d = 6;
    }
    else
    {
        d = 1;
    }


    printf("d value is %d ", d);

    g = f(6);

    a = a + g + 1;

    printf("f(6) value is %d ", f(6));
    printf("a value is %d ", a);

    g = g + (b != c);

    printf("g value is %d ", g);
    return 0;
}
