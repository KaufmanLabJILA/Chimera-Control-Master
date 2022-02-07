#include "stdafx.h"
#include "BoostUDP.h"


BoostUDP::BoostUDP(std::string IPAddress, int port)
{
	if (port == -1) {
		return;
		//handle AWG safemode.
	}

	socket_ = std::make_unique<boost::asio::ip::udp::socket>(io_service_);
	remote_endpoint = boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string(IPAddress), port);

	socket_->open(boost::asio::ip::udp::v4());

	io_thread = boost::thread(boost::bind(&BoostUDP::run, this));
	read();
}

BoostUDP::~BoostUDP()
{
	io_service_.stop();

	if (socket_) {
		socket_->cancel();
		socket_->close();
	}
	
	io_thread.join();
}

void BoostUDP::setReadCallback(const boost::function<void(int)>& read_callback)
{
	read_callback_ = read_callback;
}

void BoostUDP::readhandler(const boost::system::error_code & error, std::size_t bytes_transferred)
{
	boost::mutex::scoped_lock look(mutex_);

	if (error) {
		//thrower("Error reading UDP message.");
	}

	int c;
	for (int idx = 0; idx < bytes_transferred; idx++) {
		c = static_cast<int>(readbuffer[idx]) & 0xFF;
		if (!read_callback_.empty()) {
			read_callback_(c);
		}
	}
		
}

void BoostUDP::read()
{
	/// Invokes the readhandler() function with two arguments: a value of type boost::system::error_code indicating whether the operation succeeded or failed, and a size_t value bytes_transferred specifying the number of bytes received.
	if (!socket_->is_open()) {
		//thrower("Serial port has not been opened");
	}
	socket_->async_receive_from(boost::asio::buffer(readbuffer),
		remote_endpoint,
		boost::bind(&BoostUDP::readhandler,
		this,
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred
	));
}

void BoostUDP::write(std::vector<unsigned char> data)
{
	if (!socket_->is_open()) {
		thrower("UDP socket has not been opened");
	}

	while (data.size()>0)
	{
		if (data.size() < 1024) //maximum UDP packet size for ethernet (overly conservative)
		{
			socket_->send_to(boost::asio::buffer(data), remote_endpoint, 0, err);
			data.clear();
		}
		else
		{
			std::vector<unsigned char> dataSubset(data.begin(), data.begin() + 1024);
			socket_->send_to(boost::asio::buffer(dataSubset), remote_endpoint, 0, err);
			data.erase(data.begin(), data.begin() + 1024);
		}
	}
}

void BoostUDP::write(std::vector<int> data)
{
	std::vector<unsigned char> converted(data.size());
	for (int idx = 0; idx < data.size(); idx++) {
		if (data[idx] < 0 || data[idx] >255) {
			thrower("Byte value needs to be in range 0-255");
		}
		converted[idx] = data[idx];
	}

	write(converted);
}

void BoostUDP::run()
{
	work = std::make_unique<boost::asio::io_service::work>(io_service_);
	io_service_.run();
}
