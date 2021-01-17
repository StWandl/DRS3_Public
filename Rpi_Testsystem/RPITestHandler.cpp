
#include <iostream>
#include <chrono>
#include <thread>
#include <iomanip>
#include <string>
#include <wiringPi.h>
#include "RPITestHandler.h"
#include "RpiPinout.h"

using namespace std;

RPITestHandler::RPITestHandler(int const& nrOfSystemsUnderTest) : mNrOfSystemsUnderTest{nrOfSystemsUnderTest}
{
	mState = INIT;
}

void RPITestHandler::startObserving()
{
	const string escapeGoUpLines{ "\033[" + std::to_string(mNrOfSystemsUnderTest) + 'A' };

	while (true)
	{
		switch (mState)
		{
		case INIT:
			wiringPiSetup();
			initHandler();			
			mState = RUN;
			break;
		case RUN:
			for (auto & sysUTest : mVecRPIUnderTest)
			{
				sysUTest.updatePins();
			}			
			cout << escapeGoUpLines;
			this_thread::sleep_for(chrono::milliseconds(updateRateMs));
			break;
		case ERROR:
			cerr << "Error in observing state!" << endl;
			break;
		default:
			mState = ERROR;
			break;
		}
	}
}

void RPITestHandler::initHandler()
{
	for (size_t i = 0; i < mNrOfSystemsUnderTest; i++)
	{
		mVecRPIUnderTest.emplace_back(i, colWidth);
	}

	printInfo();
}

void RPITestHandler::printInfo() const
{
	cout << "-------------------------------------------------------------------------" << endl;
	cout << "Time synchronisation test monitor" << endl;
	cout << "-------------------------------------------------------------------------" << endl;
	cout << "Header Pin Information" << endl;
	cout << "-------------------------------------------------------------------------" << endl;
	printInfoLine<string>("TestSystemNr", "MasterClientPin", "ConnectionPin", "SendReceivePin");

	for (size_t i = 0; i < mNrOfSystemsUnderTest; i++)
	{
		printInfoLine<int>(i+1, TEST_SYSTEM_PINS[i].Master_Client_Pin.Header_Pin,
			                       TEST_SYSTEM_PINS[i].Connection_Pin.Header_Pin,
								   TEST_SYSTEM_PINS[i].Send_Receive_Pin.Header_Pin);
	}

	cout << "-------------------------------------------------------------------------" << endl;
	cout << "<<Press enter to start observing>>" << endl;
	cout << "-------------------------------------------------------------------------" << endl;

	cin.get();

	cout << "-------------------------------------------------------------------------" << endl;
	cout << "Current Status Information:" << endl;
	cout << "-------------------------------------------------------------------------" << endl;

	printInfoLine<string>("TestSystemNr", "MasterClientPin", "ConnectionPin", "SendReceivePin");
}

template <typename In>
void RPITestHandler::printInfoLine(In const& col1, In const& col2, In const& col3, In const& col4) const
{
	cout << "|" << setw(colWidth) << left << col1 << "|" << setw(colWidth) << left << col2 << "|" 
		 << setw(colWidth) << left << col3 << "|" << setw(colWidth) << left << col4 << "|" << endl;
}
