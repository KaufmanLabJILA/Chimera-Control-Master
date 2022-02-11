#pragma once

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <iostream>

class BoostUDP
{
public:
	BoostUDP(std::string IPAddress, int port);
	~BoostUDP();

	void setReadCallback(const boost::function<void(int)> &read_callback);

	void write(std::vector<unsigned char>);
	void write(std::vector<int>);
	void writeVector(std::vector<std::vector<unsigned char>>);
	void writeVector(std::vector<std::vector<int>>);
private:
	boost::asio::io_service io_service_;
	std::unique_ptr<boost::asio::ip::udp::socket> socket_;
	boost::asio::ip::udp::endpoint remote_endpoint;
	boost::system::error_code err;

	void read();
	void run();
	
	std::array<unsigned char, 1024> readbuffer;

	void readhandler(
		const boost::system::error_code& error,
		std::size_t bytes_transferred
	);

	boost::thread io_thread;
	boost::mutex mutex_;

	boost::function<void(uint8_t)> read_callback_;

	std::unique_ptr<boost::asio::io_service::work> work;	
};

