#ifndef UDPHANDLER_H
#define UDPHANDLER_H

#include <string>
#include <memory>
#include <boost/asio.hpp>

class UDPHandler
{
public:
	UDPHandler() = delete;
	UDPHandler(int UDPPort);
	~UDPHandler();

	int getUDPPort() const;
	void send(std::string const& msg, std::string const& remoteIpAddress, int const& remotePort);
	boost::asio::ip::udp::endpoint receive(std::string& recvMsg, int const& timeoutMs, boost::system::error_code& errorCodeTimeout);

private:
	int mPort;

	std::unique_ptr<boost::asio::io_service> mPIOService;
	std::unique_ptr<boost::asio::ip::udp::socket> mPSocket;
	std::unique_ptr<boost::asio::deadline_timer> mPDeadlineTimerReceive;

	std::size_t receiveTimeout(const boost::asio::mutable_buffer& buffer,
		boost::posix_time::time_duration timeout, boost::system::error_code& ec,
		boost::asio::ip::udp::endpoint& senderEndpoint);
};

#endif