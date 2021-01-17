
#include <iostream>
#include <wiringPi.h>
#include <iomanip>
#include <string>
#include "RPIUnderTest.h"
#include "RpiPinout.h"

using namespace std;

RPIUnderTest::RPIUnderTest(int const& systemNumber, int const& colWidth) : mSystemNumber{ systemNumber }, mColWidth{ colWidth }
{
	initPins();
}

void RPIUnderTest::initPins()
{
	pinMode(TEST_SYSTEM_PINS[mSystemNumber].Connection_Pin.WiringPi_Pin, INPUT);
	pinMode(TEST_SYSTEM_PINS[mSystemNumber].Master_Client_Pin.WiringPi_Pin, INPUT);
	pinMode(TEST_SYSTEM_PINS[mSystemNumber].Send_Receive_Pin.WiringPi_Pin, INPUT);
}

void RPIUnderTest::updatePins()
{
	string masterClientInfo = "";
	string connectionInfo = "";
	string sendReceiveInfo = "";

	int currStateMasterClient = digitalRead(TEST_SYSTEM_PINS[mSystemNumber].Master_Client_Pin.WiringPi_Pin);

	if (currStateMasterClient == HIGH)
	{
		masterClientInfo = "Master";

		if (digitalRead(TEST_SYSTEM_PINS[mSystemNumber].Connection_Pin.WiringPi_Pin) == HIGH)
		{
			connectionInfo = "Client connected";
		}
		else
		{
			connectionInfo = "Not connected";
		}

		if (digitalRead(TEST_SYSTEM_PINS[mSystemNumber].Send_Receive_Pin.WiringPi_Pin) == HIGH)
		{
			sendReceiveInfo = "Send";
		}
		else
		{
			sendReceiveInfo = "Receive";
		}
	}
	else
	{
		masterClientInfo = "Client";

		if (digitalRead(TEST_SYSTEM_PINS[mSystemNumber].Connection_Pin.WiringPi_Pin) == HIGH)
		{
			connectionInfo = "Master connected";
		}
		else
		{
			connectionInfo = "Not connected";
		}

		if (digitalRead(TEST_SYSTEM_PINS[mSystemNumber].Send_Receive_Pin.WiringPi_Pin) == HIGH)
		{
			sendReceiveInfo = "Receive";
		}
		else
		{
			sendReceiveInfo = "Send";
		}
	}

	printInfo(masterClientInfo, connectionInfo, sendReceiveInfo);
}

void RPIUnderTest::printInfo(string const& masterClientInfo, string const& connectionInfo, string const& sendReceiveInfo)
{
	cout << "|" << setw(mColWidth) << left << mSystemNumber+1 << "|" << setw(mColWidth) << left << masterClientInfo << "|"
		<< setw(mColWidth) << left << connectionInfo << "|" << setw(mColWidth) << left << sendReceiveInfo << "|" << endl;
}