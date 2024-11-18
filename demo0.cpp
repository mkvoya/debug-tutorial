#include <iostream>

using namespace std;

const int N = 3;

int numbers[N] = {11, 12, 13};

int bar()
{
	int s = 0;
	for (int i = 0; i < 10000; ++i)
		s += i;
	return s;
}

int foo()
{
	int i;
	int s = 0;
	for (i = 0; i < 10000; ++i)
		s += i;
	return s + bar();
}

int main()
{
	int i = 0;
	for (int i = 0; i < N; ++i)
		if (numbers[i] == 12)
			break;
	cout << (i == N ? -1 : i)  << endl;
	cout << foo() << endl;
	return 0;
}


