#include "stdafx.h"
#include "BoostAsyncSerial.h"

BoostAsyncSerial::BoostAsyncSerial(std::string portID, int baudrate)
{
	if (!GIGAMOOG_SAFEMODE) {
		port_ = std::make_unique<boost::asio::serial_port>(io_service_);

		port_->open(portID);
		port_->set_option(boost::asio::serial_port_base::baud_rate(baudrate));
		port_->set_option(boost::asio::serial_port_base::character_size(8));
		port_->set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one));
		port_->set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none));
		port_->set_option(boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none));

		io_thread = boost::thread(boost::bind(&BoostAsyncSerial::run, this));
		read();
	}

}

BoostAsyncSerial::BoostAsyncSerial(
	std::string portID,
	int baudrate,
	int character_size,
	boost::asio::serial_port_base::stop_bits::type stop_bits,
	boost::asio::serial_port_base::parity::type parity,
	boost::asio::serial_port_base::flow_control::type flow_control
)
{
	port_ = std::make_unique<boost::asio::serial_port>(io_service_);

	port_->open(portID);
	port_->set_option(boost::asio::serial_port_base::baud_rate(baudrate));
	port_->set_option(boost::asio::serial_port_base::character_size(character_size));
	port_->set_option(boost::asio::serial_port_base::stop_bits(stop_bits));
	port_->set_option(boost::asio::serial_port_base::parity(parity));
	port_->set_option(boost::asio::serial_port_base::flow_control(flow_control));

	io_thread = boost::thread(boost::bind(&BoostAsyncSerial::run, this));
	read();
}

BoostAsyncSerial::~BoostAsyncSerial()
{
	io_service_.stop();

	if (port_) {
		port_->cancel();
		port_->close();
	}
	
	io_thread.join();
}

void BoostAsyncSerial::setReadCallback(const boost::function<void(int)>& read_callback)
{
	read_callback_ = read_callback;
}

void BoostAsyncSerial::readhandler(const boost::system::error_code & error, std::size_t bytes_transferred)
{
	boost::mutex::scoped_lock look(mutex_);

	if (error) {
		thrower("Error reading from serial");
	}
	
	port_->async_read_some(boost::asio::buffer(readbuffer), boost::bind(&BoostAsyncSerial::readhandler, this,
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred
	));

	int c;
	for (int idx = 0; idx < bytes_transferred; idx++) {
		c = static_cast<int>(readbuffer[idx]) & 0xFF;
		if (!read_callback_.empty()) {
			read_callback_(c);
		}
	}
		
}

void BoostAsyncSerial::write(std::vector<unsigned char> data)
{
	if (!port_->is_open()) {
		thrower("Serial port has not been opened");
	}
	boost::asio::write(*port_, boost::asio::buffer(data));
}

void BoostAsyncSerial::write(std::vector<int> data)
{
	std::vector<unsigned char> converted(data.size());
	for (int idx = 0; idx < data.size(); idx++) {
		if(data[idx] < 0 || data[idx] >255){
			thrower("Byte value needs to be in range 0-255");
		}
		converted[idx] = data[idx];
	}

	std::cout << "Serial is writing: ";
	for (auto& byte : data)
		std::cout << (int)byte << " ";
	std::cout << "\n";

	boost::asio::write(*port_, boost::asio::buffer(converted));
}

void BoostAsyncSerial::read()
{
	if (!port_->is_open()) {
		thrower("Serial port has not been opened");
	}
	port_->async_read_some(boost::asio::buffer(readbuffer), boost::bind(&BoostAsyncSerial::readhandler,
		this,
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred
	));
}

void BoostAsyncSerial::run()
{
	work = std::make_unique<boost::asio::io_service::work>(io_service_);
	io_service_.run();
}
