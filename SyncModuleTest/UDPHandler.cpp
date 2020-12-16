#include "UDPHandler.h"
#include <boost/array.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

using boost::asio::io_service;
using boost::asio::ip::udp;
using boost::asio::ip::address_v4;
using boost::asio::ip::address;
using boost::asio::deadline_timer;

// Receive timeout wait
void checkDeadline(deadline_timer* deadlineTimerReceive, udp::socket* socket)
{
	// Check whether the deadline has passed. We compare the deadline against
	// the current time since a new asynchronous operation may have moved the
	// deadline before this actor had a chance to run.
	if (deadlineTimerReceive->expires_at() <= deadline_timer::traits_type::now())
	{
		// deadline reached -> timeout
		socket->cancel();

		// There is no longer an active deadline. The expiry is set to positive
		// infinity so that the actor takes no action until a new deadline is set.
		deadlineTimerReceive->expires_at(boost::posix_time::pos_infin);
	}

	// Put the actor back to sleep.
	deadlineTimerReceive->async_wait(boost::bind(checkDeadline, deadlineTimerReceive, socket));
}

void handleReceive(const boost::system::error_code& ec, std::size_t length,
	boost::system::error_code* out_ec, std::size_t* out_length)
{
	*out_ec = ec;
	*out_length = length;
}

UDPHandler::UDPHandler(int UDPPort) : mPort{UDPPort}
{
	mPIOService = std::make_unique<io_service>();
	mPSocket = std::make_unique<udp::socket>(*mPIOService, udp::endpoint(address_v4::any(), UDPPort));
	mPSocket->set_option(udp::socket::socket_base::broadcast(true));

	// Setup recive timeout
	mPDeadlineTimerReceive = std::make_unique<deadline_timer>(*mPIOService);
	// Start setting timout not active
	mPDeadlineTimerReceive->expires_at(boost::posix_time::pos_infin);

	// TODO: remove parameters
	checkDeadline(mPDeadlineTimerReceive.get(), mPSocket.get());
}

UDPHandler::~UDPHandler()
{
	mPSocket->close();
}

int UDPHandler::getUDPPort() const
{
	return mPort;
}

void UDPHandler::send(std::string const& msg, std::string const& remoteIpAddress, int const& remotePort)
{
	udp::endpoint remoteEndpoint = udp::endpoint(address_v4::from_string(remoteIpAddress), remotePort);

	mPSocket->send_to(boost::asio::buffer(msg), remoteEndpoint);
}

// Receive with timeout
std::size_t UDPHandler::receiveTimeout(const boost::asio::mutable_buffer& buffer,
	boost::posix_time::time_duration timeout, boost::system::error_code& ec, 
	udp::endpoint & senderEndpoint)
{
	// Set a deadline for the asynchronous operation.
	mPDeadlineTimerReceive->expires_from_now(timeout);

	// Set up the variables that receive the result of the asynchronous
	// operation. The error code is set to would_block to signal that the
	// operation is incomplete. Asio guarantees that its asynchronous
	// operations will never fail with would_block, so any other value in
	// ec indicates completion.
	ec = boost::asio::error::would_block;
	std::size_t length = 0;

	// Start the asynchronous operation itself. The handleReceive function
	// used as a callback will update the ec and length variables.
	mPSocket->async_receive_from(
		boost::asio::buffer(buffer), senderEndpoint,
		boost::bind(&handleReceive, _1, _2, &ec, &length));

	// Block until the asynchronous operation has completed.
	do {
		mPIOService->run_one();
	} while (ec == boost::asio::error::would_block);

	return length;
}

udp::endpoint UDPHandler::receive(std::string & recvMsg, int const& timeoutMs, boost::system::error_code & errorCodeTimeout)
{
	udp::endpoint senderEndpoint;

	boost::array<char, 4096> recvBuffer;	

	//size_t lenRecived = mPSocket->receive_from(boost::asio::buffer(recvBuffer), senderEndpoint);
	size_t lenRecived = receiveTimeout(boost::asio::buffer(recvBuffer), boost::posix_time::milliseconds(timeoutMs), errorCodeTimeout, senderEndpoint);

	recvMsg = std::string(recvBuffer.data(), lenRecived);

	return senderEndpoint;
}
