// Sajan Cherukad, Stefan Wandl, Dominic Zopf
// DRS3 - Module Test Program

#include <iostream>
#include <thread>
#include <chrono>

#include "Message.h"
#include "UDPHandler.h"
#include "Testscenarios.h"

#include <chrono>

using namespace std;

/* Application Parameters */

int rxUDP_PORT = 12345;						// udp port for receive
int txUDP_PORT = 12345;						// udp port for send
constexpr int SYNC_INTERVAL_ms = 250;					// synchronisation interval
constexpr int MAX_TIMESTAMP_REQ_TRIES = 2;				// max tries of master to request timestamp from slave

const string testerID = "MyID"; // to do?


string getTimestamp();
void checkErrorCode(const boost::system::error_code& ec, int setTimeout, const std::string& errorMsg);
void checkReceivedMessage(const Message &msg, int testNodePriority, int expectedType, const std::string& errorMsg);
void checkIpAddress(const string& expected, const string& actual, const string& errorMsg);
void runTestScenario_1(int testNodePriority);
void runTestScenario_2(int testNodePriority);

const string usageErrorString = "Bad commandline arguments!\nArguments: [TestScenario(1 or 2)] [TestNodePriority(>1)] [-local (optional)]\ne.g.: ./SyncModuleTest(.exe) 1 4711 -local";

int main(int argc, char* argv[])
{
	try
	{
		if (argc != 3 && argc != 4)
		{
			throw std::exception(usageErrorString.c_str());
		}
		else
		{
			/* Parse commandline arguments */
			int testScenario = std::atoi(argv[1]);
			int testNodePriority = std::atoi(argv[2]);

			if((testScenario != 1 && testScenario != 2) || testNodePriority <= 1)
				throw std::exception(usageErrorString.c_str());

			if (argc == 4)
			{
				if (strcmp(argv[3], "-local") == 0)
				{
					rxUDP_PORT = 12345;
					txUDP_PORT = 12346;
				}
				else
				{
					throw std::exception(usageErrorString.c_str());
				}				
			}

			cout << "Testing scenario:        " << testScenario << endl;
			cout << "Priority of tested node: " << testNodePriority << endl << endl;
			
			if (testScenario == 1)
				runTestScenario_1(testNodePriority);
			else
				runTestScenario_2(testNodePriority);
		}
	}
	catch (const std::exception& e)
	{
		cerr << "Error:" << endl << e.what() << endl;
		return 1;
	}	

	return 0;
}

string getTimestamp()
{
	size_t unix_timestamp_us = std::chrono::microseconds(std::time(NULL)).count();

	string str = std::to_string(unix_timestamp_us) + "-" + std::to_string(unix_timestamp_us);

	return str;
}

void checkErrorCode(const boost::system::error_code& ec, int setTimeout, const std::string & errorMsg)
{
	if (ec.value() != boost::system::errc::success)
	{
		string exceptionMsg;

		if (ec.value() == 995)
		{
			exceptionMsg = "UDP receive timeout. No message in " + std::to_string(setTimeout) + " ms\n";
		}
		else
		{
			exceptionMsg = ec.message() + "\n" ;
		}

		exceptionMsg += errorMsg;
		throw exception(exceptionMsg.c_str());
	}
}

void checkReceivedMessage(const Message& msg, int testNodePriority, int expectedType, const std::string& errorMsg)
{
	string exceptionMsg;

	if (msg.isValid())
	{
		if (msg.priority() != testNodePriority)
		{
			exceptionMsg = "Priority in Message does not match configured priority!\n";
		}
		else if (msg.type() != expectedType)
		{
			exceptionMsg = "Wrong Message type, expected: " + std::to_string(expectedType) + ", received: " + std::to_string(msg.type()) + "!\n";
		}
		else if (msg.id().empty())
		{
			exceptionMsg = "Message has no ID!\n";
		}
		else if (msg.timestamp().empty())
		{
			exceptionMsg = "Message has no timestamp!\n";
		}
		else if (msg.address().empty())
		{
			exceptionMsg = "Message has no node address!\n";
		}
		else if ((msg.type() == 6 || msg.type() == 7) && msg.payload().empty())
		{
			exceptionMsg = "Message of type " + std::to_string(msg.type()) + " must not have an empty payload!\n";
		}
		else if (msg.type() != 6 && msg.type() != 7 && !msg.payload().empty())
		{
			exceptionMsg = "Message of type " + std::to_string(msg.type()) + " must not have a payload!\n";
		}
		else
			return; // msg is good
	}
	else
	{
		exceptionMsg = "Message Format not valid!\n";
	}

	exceptionMsg += errorMsg;
	throw exception(exceptionMsg.c_str());
}

void checkIpAddress(const string& expected, const string & actual, const string & errorMsg)
{
	if (actual != expected)
	{
		string errMsg = errorMsg + "\nMessage from unknown ip address " + actual + " received, expected message from: " + expected;
		throw exception(errMsg.c_str());
	}
}

void runTestScenario_1(int testNodePriority)
{
	UDPHandler udpHandler{ rxUDP_PORT };
	boost::asio::ip::udp::endpoint remoteEndpoint;

	string testObject_IP;
	string rxData;
	string txData;
	boost::system::error_code errorCode;
	Message rxMsg;
	Message txMsg(testerID, "timestamp", "myAddress", testNodePriority + 1, MsgType::SlaveRequest, "");

	/*
	1) Integration zu aktiven höher prioren Master

		A wartet auf Msg - Type1 auf Port 12345 (Broadcast)

					B sendet seine Priorität über Broadcast, Msg - Type1

		A registriert(speichert) B als Slave und sendet ACK(Msg - Type2)
	*/
	cout << "##### Testing: Integration of slave (testobject) to active master (higher priority) #####" << endl << endl;

	{
		remoteEndpoint = udpHandler.receive(rxData, 20000, errorCode);

		checkErrorCode(errorCode, 20000, "Error while waiting for initial message from slave!");

		rxMsg = Message::fromString(rxData);
		checkReceivedMessage(rxMsg, testNodePriority, MsgType::SlaveRequest, "Expected Message: Slave request (type=1) from slave");

		testObject_IP = remoteEndpoint.address().to_string();

		cout << "Received slave request message from slave (" << testObject_IP << ") with priority " << rxMsg.priority() << endl;

		txMsg.setType(MsgType::MasterACK);
		txMsg.setPriority(testNodePriority + 1);
		txMsg.setPayload("");
		txData = txMsg.toString();
		udpHandler.send(txData, testObject_IP, txUDP_PORT);

		cout << "Saving slave information and sending back ACK message." << endl << endl;		
	}

	/*
	2) Synchronisation mit Master (3x durchlaufen)

		A schickt Anfrage Msg-Type5 an Slave

				B antwortet mit Zeitstempel, Msg-Type6

		A sendet Abweichung zurück, Msg-Type7
	*/
	cout << "##### Testing: Synchronisation (3x) #####" << endl << endl;

	{
		for (size_t i = 0; i < 3; i++)
		{
			cout << "Waiting " << 1 * SYNC_INTERVAL_ms << " ms..." << endl;
			remoteEndpoint = udpHandler.receive(rxData, 1 * SYNC_INTERVAL_ms, errorCode);

			if (errorCode.value() == boost::system::errc::success)
			{
				rxMsg = Message::fromString(rxData);
				string errMsg = "Received unexpected message (type=" + std::to_string(rxMsg.type()) + ") inbetween update interval!";
				throw exception(errMsg.c_str());
			}

			txMsg.setType(MsgType::RequestTimestamp);
			txMsg.setPayload("");
			txData = txMsg.toString();
			cout << "Requesting timestamp from slave..." << endl;
			udpHandler.send(txData, testObject_IP, txUDP_PORT);

			remoteEndpoint = udpHandler.receive(rxData, 200, errorCode);

			checkErrorCode(errorCode, 200, "Error while waiting for timestamp message from slave!");

			checkIpAddress(testObject_IP, remoteEndpoint.address().to_string(), "Error while waiting for timestamp message");

			rxMsg = Message::fromString(rxData);
			checkReceivedMessage(rxMsg, testNodePriority, MsgType::Timestamp, "Expected Message: Timestamp (type=6) from slave");

			cout << "Received timestamp message from slave." << endl;

			txMsg.setType(MsgType::Deviation);
			txMsg.setPayload("0"); // ?
			txData = txMsg.toString();

			cout << "Sending back deviation message." << endl;
			udpHandler.send(txData, testObject_IP, txUDP_PORT);
		}

		cout << endl;
	}

	/*

	3) Ausfall Master, A wird wieder Master

		A wartet max. 3 Sync-Intervalle

					B sendet Priorität über Broadcast aus, Msg-Type1

		A sendet (höhere) Priorität über Broadcast aus, Msg-Type1

		A wartet 1 Sekunde

		A sendet über Broadcast aus, dass er jetzt Master ist, Msg-Type3

					B antwortet mit ACK, Msg-Type4
	*/

	cout << "##### Testing: Master failure and reselecting new master (again this node) #####" << endl << endl;

	{
		cout << "Waiting " << 2 * SYNC_INTERVAL_ms << " ms..." << endl;
		remoteEndpoint = udpHandler.receive(rxData, 2 * SYNC_INTERVAL_ms - 100, errorCode);

		if (errorCode.value() == boost::system::errc::success)
		{
			rxMsg = Message::fromString(rxData);
			string errMsg = "Received unexpected message (type=" + std::to_string(rxMsg.type()) + ") in time where no message should be received!";
			throw exception(errMsg.c_str());
		}

		remoteEndpoint = udpHandler.receive(rxData, 1 * SYNC_INTERVAL_ms, errorCode);

		checkErrorCode(errorCode, 1 * SYNC_INTERVAL_ms, "Error while waiting for slave request message (type=1)!");

		checkIpAddress(testObject_IP, remoteEndpoint.address().to_string(), "Error while waiting for slave request message (type=1)!");

		rxMsg = Message::fromString(rxData);
		checkReceivedMessage(rxMsg, testNodePriority, MsgType::SlaveRequest, "Expected Message: SlaveRequest (type=1) from slave");

		cout << "Received slave request message from slave (" << testObject_IP << ") with priority " << rxMsg.priority() << endl;

		txMsg.setType(MsgType::SlaveRequest);
		txMsg.setPayload("");
		txData = txMsg.toString();

		cout << "Sending out own slave request broadcast message with priority " << txMsg.priority() << " ..." << endl;
		udpHandler.send(txData, "255.255.255.255", txUDP_PORT);

		cout << "Waiting " << 1000 << " ms..." << endl;
		remoteEndpoint = udpHandler.receive(rxData, 1000, errorCode);

		if (errorCode.value() == boost::system::errc::success)
		{
			rxMsg = Message::fromString(rxData);
			string errMsg = "Received unexpected message (type=" + std::to_string(rxMsg.type()) + ") in time where no message should be received!";
			throw exception(errMsg.c_str());
		}

		txMsg.setType(MsgType::NewMaster);
		txMsg.setPayload("");
		txData = txMsg.toString();

		cout << "Sending out broadcast that I am master now..." << endl;
		udpHandler.send(txData, "255.255.255.255", txUDP_PORT);


		remoteEndpoint = udpHandler.receive(rxData, 500, errorCode);

		checkErrorCode(errorCode, 500, "Error while waiting for new master ACK message from slave (type=4)!");

		checkIpAddress(testObject_IP, remoteEndpoint.address().to_string(), "Error while waiting for new master ACK message from slave (type=4)!");

		rxMsg = Message::fromString(rxData);
		checkReceivedMessage(rxMsg, testNodePriority, MsgType::NewMasterACK, "Expected Message: New Master ACK (type=4) from slave");

		cout << "Received new master ACK message from slave " << endl;

		cout << endl;
	}

	/*
		4) Synchronisation mit Master (3x durchlaufen)

		A schickt Anfrage Msg-Type5 an Slave

					B antwortet mit Zeitstempel, Msg-Type6

		A sendet Abweichung zurück, Msg-Type7
		*/

	cout << "##### Testing: Synchronisation (3x) #####" << endl << endl;

	{
		for (size_t i = 0; i < 3; i++)
		{
			cout << "Waiting " << 1 * SYNC_INTERVAL_ms << " ms..." << endl;
			remoteEndpoint = udpHandler.receive(rxData, 1 * SYNC_INTERVAL_ms, errorCode);

			if (errorCode.value() == boost::system::errc::success)
			{
				rxMsg = Message::fromString(rxData);
				string errMsg = "Received unexpected message (type=" + std::to_string(rxMsg.type()) + ") inbetween update interval!";
				throw exception(errMsg.c_str());
			}

			txMsg.setType(MsgType::RequestTimestamp);
			txMsg.setPayload("");
			txData = txMsg.toString();
			cout << "Requesting timestamp from slave..." << endl;
			udpHandler.send(txData, testObject_IP, txUDP_PORT);

			remoteEndpoint = udpHandler.receive(rxData, 200, errorCode);

			checkErrorCode(errorCode, 200, "Error while waiting for timestamp message from slave!");

			checkIpAddress(testObject_IP, remoteEndpoint.address().to_string(), "Error while waiting for timestamp message");

			rxMsg = Message::fromString(rxData);
			checkReceivedMessage(rxMsg, testNodePriority, MsgType::Timestamp, "Expected Message: Timestamp (type=6) from slave");

			cout << "Received timestamp message from slave." << endl;

			txMsg.setType(MsgType::Deviation);
			txMsg.setPayload("0"); // ?
			txData = txMsg.toString();

			cout << "Sending back deviation message." << endl;
			udpHandler.send(txData, testObject_IP, txUDP_PORT);
		}

		cout << endl;

	}

	/*
	5) Ausfall Master, B wird jetzt Master

		A wartet max. 3 Sync-Intervalle

				B sendet Priorität über Broadcast aus, Msg-Type1

		A sendet (niedrigere) Priorität über Broadcast aus, Msg-Type1

				B wartet ca. 1 Sekunde

				B sendet über Broadcast aus, dass er jetzt Master ist, Msg-Type3

		A antwortet mit ACK, Msg-Type4
	*/

	cout << "##### Testing: Master failure and reselecting new master (testobject will be master) #####" << endl << endl;

	{
		cout << "Waiting " << 2 * SYNC_INTERVAL_ms << " ms..." << endl;
		remoteEndpoint = udpHandler.receive(rxData, 2 * SYNC_INTERVAL_ms - 100, errorCode);

		if (errorCode.value() == boost::system::errc::success)
		{
			rxMsg = Message::fromString(rxData);
			string errMsg = "Received unexpected message (type=" + std::to_string(rxMsg.type()) + ") in time where no message should be received!";
			throw exception(errMsg.c_str());
		}

		remoteEndpoint = udpHandler.receive(rxData, 1 * SYNC_INTERVAL_ms, errorCode);

		checkErrorCode(errorCode, 1 * SYNC_INTERVAL_ms, "Error while waiting for slave request message (type=1)!");

		checkIpAddress(testObject_IP, remoteEndpoint.address().to_string(), "Error while waiting for slave request message (type=1)!");

		rxMsg = Message::fromString(rxData);
		checkReceivedMessage(rxMsg, testNodePriority, MsgType::SlaveRequest, "Expected Message: SlaveRequest (type=1) from slave");

		cout << "Received slave request message from slave (" << testObject_IP << ") with priority " << rxMsg.priority() << endl;

		txMsg.setType(MsgType::SlaveRequest);
		txMsg.setPriority(testNodePriority - 1);
		txMsg.setPayload("");
		txData = txMsg.toString();

		cout << "Sending out own slave request broadcast message with priority " << txMsg.priority() << " ..." << endl;
		udpHandler.send(txData, "255.255.255.255", txUDP_PORT);

		remoteEndpoint = udpHandler.receive(rxData, 900, errorCode);

		if (errorCode.value() == boost::system::errc::success)
		{
			rxMsg = Message::fromString(rxData);
			string errMsg = "Received unexpected message (type=" + std::to_string(rxMsg.type()) + ") in time where no message should be received!\n";
			errMsg += "Testobject did not wait 1 sec before sending new master message!";
			throw exception(errMsg.c_str());
		}

		remoteEndpoint = udpHandler.receive(rxData, 2 * SYNC_INTERVAL_ms, errorCode);

		checkErrorCode(errorCode, 2 * SYNC_INTERVAL_ms, "Error while waiting for new master message from testobject (type=3)!");

		checkIpAddress(testObject_IP, remoteEndpoint.address().to_string(), "Error while waiting for new master message from testobject (type=3)!");

		rxMsg = Message::fromString(rxData);
		checkReceivedMessage(rxMsg, testNodePriority, MsgType::NewMaster, "Expected Message: NewMaster (type=3) from testobject");

		cout << "Received new master message from testobject" << endl;

		txMsg.setType(MsgType::NewMasterACK);
		txMsg.setPayload("");
		txData = txMsg.toString();

		cout << "Sending back new master ACK message to new master (=testobject)" << endl;
		udpHandler.send(txData, testObject_IP, txUDP_PORT);

		cout << endl;
	}

	

	/*
	6) Synchronisation als Master (3x durchlaufen)

					B schickt Anfrage Msg-Type5 an A

		A antwortet mit Zeitstempel, Msg-Type6

					B sendet Abweichung zurück, Msg-Type7
		*/

	cout << "##### Testing: Synchronisation (3x) with testobject as master #####" << endl << endl;

	{
		for (size_t i = 0; i < 3; i++)
		{
			remoteEndpoint = udpHandler.receive(rxData, 2 * SYNC_INTERVAL_ms, errorCode);

			checkErrorCode(errorCode, 2 * SYNC_INTERVAL_ms, "Error while waiting for timestamp request message from master (type=5)!");

			checkIpAddress(testObject_IP, remoteEndpoint.address().to_string(), "Error while waiting for timestamp request message from master (type=5)!");

			rxMsg = Message::fromString(rxData);
			checkReceivedMessage(rxMsg, testNodePriority, MsgType::RequestTimestamp, "Expected Message: RequestTimestamp (type=5) from master");

			cout << "Received timestamp request message from master" << endl;


			txMsg.setType(MsgType::Timestamp);
			txMsg.setPayload(getTimestamp());
			txData = txMsg.toString();

			cout << "Sending back timestamp message to master" << endl;
			udpHandler.send(txData, testObject_IP, txUDP_PORT);


			remoteEndpoint = udpHandler.receive(rxData, 200, errorCode);

			checkErrorCode(errorCode, 200, "Error while waiting for deviation message from master (type=7)!");

			checkIpAddress(testObject_IP, remoteEndpoint.address().to_string(), "Error while waiting for deviation message from master (type=7)!");

			rxMsg = Message::fromString(rxData);
			checkReceivedMessage(rxMsg, testNodePriority, MsgType::Deviation, "Expected Message: Deviation (type=7) from master");

			cout << "Received deviation message from master" << endl;
		}
		
		cout << endl;
	}

	

	/*
	7) Slave Ausfall, A antwortet nicht mehr

		A antwortet nicht mehr

					B sendet Timestamp Anfrage (Msg-Type5) 2x und entfernt Slave (A) danach aus Liste
	*/
	
	cout << "##### Testing: Slave failure #####" << endl << endl;

	{
		for (size_t i = 0; i < MAX_TIMESTAMP_REQ_TRIES; i++)
		{
			remoteEndpoint = udpHandler.receive(rxData, 2 * SYNC_INTERVAL_ms, errorCode);

			checkErrorCode(errorCode, 2 * SYNC_INTERVAL_ms, "Error while waiting for timestamp request message from master (type=5)!");

			checkIpAddress(testObject_IP, remoteEndpoint.address().to_string(), "Error while waiting for timestamp request message from master (type=5)!");

			rxMsg = Message::fromString(rxData);
			checkReceivedMessage(rxMsg, testNodePriority, MsgType::RequestTimestamp, "Expected Message: RequestTimestamp (type=5) from master");

			cout << "Received timestamp request message from master" << endl;
		}

		cout << "Master should remove us from its client list..." << endl;

		remoteEndpoint = udpHandler.receive(rxData, 1000, errorCode);

		if (errorCode.value() == boost::system::errc::success)
		{
			rxMsg = Message::fromString(rxData);
			string errMsg = "Received unexpected message (type=" + std::to_string(rxMsg.type()) + ") in time where no message should be received!";
			throw exception(errMsg.c_str());
		}

		cout << "No timestamp request for " << 1000 << "ms. Looks good!" << endl;
	}


	cout << endl << "##### Test Scenario 1 successful #####" << endl;
	
}


void runTestScenario_2(int testNodePriority)
{
	UDPHandler udpHandler{ rxUDP_PORT };
	boost::asio::ip::udp::endpoint remoteEndpoint;

	string testObject_IP;
	string rxData;
	string txData;
	boost::system::error_code errorCode;
	Message rxMsg;
	Message txMsg(testerID, "timestamp", "myAddress", testNodePriority - 1, MsgType::SlaveRequest, "");

	/*
	1) Integration zu aktiven Master, welcher eine niedrigere Priorität hat

		A wartet auf Msg-Type1 auf Port 12345 (Broadcast)

						B sendet seine Priorität über Broadcast, Msg-Type1

		A sendet ACK (Msg-Type2)

						B erkennt, dass eigene Priorität höher ist als die von A
						--> B benachrichtigt alle Knoten über Broadcast, dass er jetzt Master ist, Msg-Type3

		A antwortet darauf mit Ack, Msg-Type4
	*/

	cout << "##### Testing: Integration of slave (testobject, higher priority) to active master (lower priority) #####" << endl << endl;

	{
		remoteEndpoint = udpHandler.receive(rxData, 20000, errorCode);

		checkErrorCode(errorCode, 20000, "Error while waiting for initial message from slave!");

		rxMsg = Message::fromString(rxData);
		checkReceivedMessage(rxMsg, testNodePriority, 1, "Expected Message: Slave request (type=1) from slave");

		testObject_IP = remoteEndpoint.address().to_string();

		cout << "Received slave request message from slave (" << testObject_IP << ") with priority " << rxMsg.priority() << endl;

		txMsg.setType(MsgType::MasterACK);
		txMsg.setPriority(testNodePriority - 1);
		txMsg.setPayload("");
		txData = txMsg.toString();
		cout << "Sending back ACK message with priority: " << txMsg.priority() << endl;
		cout << "Waiting for testobject to claim master role..." << endl;
		udpHandler.send(txData, testObject_IP, txUDP_PORT);


		remoteEndpoint = udpHandler.receive(rxData, 500, errorCode);

		checkErrorCode(errorCode, 500, "Error while waiting for new master message from testobject!");

		rxMsg = Message::fromString(rxData);
		checkReceivedMessage(rxMsg, testNodePriority, MsgType::NewMaster, "Expected Message: New Master (type=3) from testobject");

		cout << "Received new master broadcast message from " << testObject_IP << " with priority " << rxMsg.priority() << endl;

		txMsg.setType(MsgType::NewMasterACK);
		txMsg.setPayload("");
		txData = txMsg.toString();
		cout << "Sending back new master ACK message..." << endl;
		udpHandler.send(txData, testObject_IP, txUDP_PORT);

		cout << endl;
	}

	/*
	2) Synchronisation als Master (3x durchlaufen)

					B schickt Anfrage Msg-Type5 an A

		A antwortet mit Zeitstempel, Msg-Type6

					B sendet Abweichung zurück, Msg-Type7
	*/

	cout << "##### Testing: Synchronisation (3x) with testobject as master #####" << endl << endl;

	{
		for (size_t i = 0; i < 3; i++)
		{
			remoteEndpoint = udpHandler.receive(rxData, 2 * SYNC_INTERVAL_ms, errorCode);

			checkErrorCode(errorCode, 2 * SYNC_INTERVAL_ms, "Error while waiting for timestamp request message from master (type=5)!");
			checkIpAddress(testObject_IP, remoteEndpoint.address().to_string(), "Error while waiting for timestamp request message from master (type=5)!");

			rxMsg = Message::fromString(rxData);
			checkReceivedMessage(rxMsg, testNodePriority, MsgType::RequestTimestamp, "Expected Message: RequestTimestamp (type=5) from master");

			cout << "Received timestamp request message from master" << endl;


			txMsg.setType(MsgType::Timestamp);
			txMsg.setPayload(getTimestamp());
			txData = txMsg.toString();

			cout << "Sending back timestamp message to master" << endl;
			udpHandler.send(txData, testObject_IP, txUDP_PORT);


			remoteEndpoint = udpHandler.receive(rxData, 200, errorCode);

			checkErrorCode(errorCode, 200, "Error while waiting for deviation message from master (type=7)!");
			checkIpAddress(testObject_IP, remoteEndpoint.address().to_string(), "Error while waiting for deviation message from master (type=7)!");

			rxMsg = Message::fromString(rxData);
			checkReceivedMessage(rxMsg, testNodePriority, MsgType::Deviation, "Expected Message: Deviation (type=7) from master");

			cout << "Received deviation message from master" << endl;
		}

		cout << endl;
	}


	/*
	3) Slave Ausfall, A antwortet nicht mehr

		A antwortet nicht mehr

				B sendet Timestamp Anfrage (Msg-Type5) 2x und entfernt Slave (A) danach aus Liste
	*/

	cout << "##### Testing: Slave failure #####" << endl << endl;

	{
		for (size_t i = 0; i < MAX_TIMESTAMP_REQ_TRIES; i++)
		{
			remoteEndpoint = udpHandler.receive(rxData, 2 * SYNC_INTERVAL_ms, errorCode);

			checkErrorCode(errorCode, 2 * SYNC_INTERVAL_ms, "Error while waiting for timestamp request message from master (type=5)!");

			checkIpAddress(testObject_IP, remoteEndpoint.address().to_string(), "Error while waiting for timestamp request message from master (type=5)!");

			rxMsg = Message::fromString(rxData);
			checkReceivedMessage(rxMsg, testNodePriority, MsgType::RequestTimestamp, "Expected Message: RequestTimestamp (type=5) from master");

			cout << "Received timestamp request message from master" << endl;
		}

		cout << "Master should remove us from its client list..." << endl;

		remoteEndpoint = udpHandler.receive(rxData, 1000, errorCode);

		if (errorCode.value() == boost::system::errc::success)
		{
			rxMsg = Message::fromString(rxData);
			string errMsg = "Received unexpected message (type=" + std::to_string(rxMsg.type()) + ") in time where no message should be received!";
			throw exception(errMsg.c_str());
		}

		cout << "No timestamp request for " << 1000 << "ms. Looks good!" << endl << endl;
	}

	/*
	4) A reintegriert sich mit höherer Priorität, übernimmt Master-Rolle von B

		A sendet seine Priorität über Broadcast, Msg-Type1

						B sendet ACK (Msg-Type2)

		A erkennt, dass eigene Priorität höher ist als die von B
		--> A benachrichtigt alle Knoten über Broadcast, dass er jetzt Master ist, Msg-Type3

						B antwortet darauf mit Ack, Msg-Type4
	*/
	
	cout << "##### Testing: Integration of node (this) with higher priority than master (testobject) #####" << endl << endl;

	{
		txMsg.setType(MsgType::SlaveRequest);
		txMsg.setPriority(testNodePriority + 1);
		txMsg.setPayload("");
		txData = txMsg.toString();

		cout << "Sending out own slave request broadcast message with priority " << txMsg.priority() << " ..." << endl;
		udpHandler.send(txData, "255.255.255.255", txUDP_PORT);

		remoteEndpoint = udpHandler.receive(rxData, 500, errorCode);

		checkErrorCode(errorCode, 500, "Error while waiting for master ACK message (type=2)!");

		checkIpAddress(testObject_IP, remoteEndpoint.address().to_string(), "Error while waiting for master ACK message (type=2)!");

		rxMsg = Message::fromString(rxData);
		checkReceivedMessage(rxMsg, testNodePriority, MsgType::MasterACK, "Expected Message: Master ACK (type=2) from slave");

		cout << "Received master ACK message from master (" << testObject_IP << ") with priority " << rxMsg.priority() << endl;

		cout << "I have a higher priority --> claim master role..." << endl;

		
		txMsg.setType(MsgType::NewMaster);
		txMsg.setPriority(testNodePriority + 1);
		txMsg.setPayload("");
		txData = txMsg.toString();

		cout << "Sending out new master broadcast message with priority..." << endl;
		udpHandler.send(txData, "255.255.255.255", txUDP_PORT);

		remoteEndpoint = udpHandler.receive(rxData, 500, errorCode);

		checkErrorCode(errorCode, 500, "Error while waiting for new master ACK message(type=4)!");

		checkIpAddress(testObject_IP, remoteEndpoint.address().to_string(), "Error while waiting for new master ACK message (type=4)!");

		rxMsg = Message::fromString(rxData);
		checkReceivedMessage(rxMsg, testNodePriority, MsgType::NewMasterACK, "Expected Message: NewMaster ACK (type=4) from slave");

		cout << "Received new master ACK message from slave (" << testObject_IP << ") with priority " << rxMsg.priority() << endl;
		cout << endl;
	}

	/*
	5) Synchronisation mit Master (3x durchlaufen)

		A schickt Anfrage Msg-Type5 an B

						B antwortet mit Zeitstempel, Msg-Type6

		A sendet Abweichung zurück, Msg-Type7
	*/
	
	cout << "##### Testing: Synchronisation (3x) #####" << endl << endl;

	{
		for (size_t i = 0; i < 3; i++)
		{
			cout << "Waiting " << 1 * SYNC_INTERVAL_ms << " ms..." << endl;
			remoteEndpoint = udpHandler.receive(rxData, 1 * SYNC_INTERVAL_ms, errorCode);

			if (errorCode.value() == boost::system::errc::success)
			{
				rxMsg = Message::fromString(rxData);
				string errMsg = "Received unexpected message (type=" + std::to_string(rxMsg.type()) + ") inbetween update interval!";
				throw exception(errMsg.c_str());
			}

			txMsg.setType(MsgType::RequestTimestamp);
			txMsg.setPayload("");
			txData = txMsg.toString();
			cout << "Requesting timestamp from slave..." << endl;
			udpHandler.send(txData, testObject_IP, txUDP_PORT);

			remoteEndpoint = udpHandler.receive(rxData, 200, errorCode);

			checkErrorCode(errorCode, 200, "Error while waiting for timestamp message from slave!");

			checkIpAddress(testObject_IP, remoteEndpoint.address().to_string(), "Error while waiting for timestamp message");

			rxMsg = Message::fromString(rxData);
			checkReceivedMessage(rxMsg, testNodePriority, MsgType::Timestamp, "Expected Message: Timestamp (type=6) from slave");

			cout << "Received timestamp message from slave." << endl;

			txMsg.setType(MsgType::Deviation);
			txMsg.setPayload("0");
			txData = txMsg.toString();

			cout << "Sending back deviation message." << endl;
			udpHandler.send(txData, testObject_IP, txUDP_PORT);
		}

		cout << endl;

	}

	/*
	6) Master (A) fällt aus, B übernimmt Master Rolle

		A wartet max. 3 Sync-Intervalle

						B sendet Priorität über Broadcast aus, Msg-Type1

						B wartet ca. 1 Sekunde

						B sendet über Broadcast aus, dass er jetzt Master ist, Msg-Type3
	*/

	cout << "##### Testing: Master failure, testobject now alone --> new master #####" << endl << endl;

	{
		cout << "Waiting " << 2 * SYNC_INTERVAL_ms << " ms..." << endl;
		remoteEndpoint = udpHandler.receive(rxData, 2 * SYNC_INTERVAL_ms - 100, errorCode);

		if (errorCode.value() == boost::system::errc::success)
		{
			rxMsg = Message::fromString(rxData);
			string errMsg = "Received unexpected message (type=" + std::to_string(rxMsg.type()) + ") in time where no message should be received!";
			throw exception(errMsg.c_str());
		}

		remoteEndpoint = udpHandler.receive(rxData, 1 * SYNC_INTERVAL_ms, errorCode);

		checkErrorCode(errorCode, 1 * SYNC_INTERVAL_ms, "Error while waiting for slave request message (type=1)!");

		checkIpAddress(testObject_IP, remoteEndpoint.address().to_string(), "Error while waiting for slave request message (type=1)!");

		rxMsg = Message::fromString(rxData);
		checkReceivedMessage(rxMsg, testNodePriority, MsgType::SlaveRequest, "Expected Message: SlaveRequest (type=1) from slave");

		cout << "Received slave request message from slave (" << testObject_IP << ") with priority " << rxMsg.priority() << endl;

		cout << "Waiting for testobject to claim master role..." << endl;

		remoteEndpoint = udpHandler.receive(rxData, 2000, errorCode);

		checkErrorCode(errorCode, 1 * SYNC_INTERVAL_ms, "Error while waiting for new master message from testobject (type=3)!");

		checkIpAddress(testObject_IP, remoteEndpoint.address().to_string(), "Error while waiting for new master message from testobject (type=3)!");

		rxMsg = Message::fromString(rxData);
		checkReceivedMessage(rxMsg, testNodePriority, MsgType::NewMaster, "Expected Message: NewMaster (type=3) from testobject");

		cout << "Received new master message from testobject" << endl;

		cout << endl;
	}


	cout << endl << "##### Test Scenario 2 successful #####" << endl;
}
