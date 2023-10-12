#include <iostream>
using namespace std;

int main() {
    cout << "\033[5;10H" << 'X' << endl;
	while(1){cout << "\033[5;10H" << 'X' << endl;}
    return 0;
}