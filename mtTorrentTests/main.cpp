#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include "Test.h"

int main(int argc, char* argv[])
{
	try
	{
		TorrentTest test;
		test.start();
	}
	catch (std::exception& e)
	{
		std::cout << "End exception: " << e.what() << "\n";
	}

	return 0;
}
