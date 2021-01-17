// Raspberry Pi 3B testsystem for time synchronisation system

#include <iostream>
#include "RpiPinout.h"
#include "RPITestHandler.h"

using namespace std;

int main(int argc, char* argv[]) {

	try
	{
		if (argc != 2)
		{
			cerr << "Error: Input argument not valid. Argument: <Number of testsystems>" << endl;
			return -1;
		}
		else
		{
			// Parse number of test systems
			int nrTestSystems = std::atoi(argv[1]);

			if (nrTestSystems <= 0)
			{
				cerr << "Error: Number of testsystems has to be a positive non zero integer" << endl;
				return -1;
			}
			else if (nrTestSystems > MAX_NR_TEST_SYSTEMS)
			{
				cerr << "Error: Maximum number of testsystems is " << MAX_NR_TEST_SYSTEMS << endl;
				return -1;
			}
			else
			{
				RPITestHandler testHandler{ nrTestSystems };
				testHandler.startObserving();
			}
		}
	}
	catch (const std::exception& e)
	{
		cerr << "Error: " << e.what() << endl;
		return -1;
	}

	return 0;
}