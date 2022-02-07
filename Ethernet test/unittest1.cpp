#include "stdafx.h"
#include "CppUnitTest.h"
#include "BoostUDP.h"
//#include "boost/asio.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Ethernettest
{		
	TEST_CLASS(UnitTest1)
	{
	public:
		
		TEST_METHOD(TestMethod1)
		{
			// TODO: Your test code here
			//boost::asio::io_service io_service;
			//boost::asio::ip::udp::socket socket(io_service);
			//boost::asio::ip::udp::endpoint remote_endpoint;

			//socket.open(boost::asio::ip::udp::v4());

			//remote_endpoint = boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 1000);

			//boost::system::error_code err;
			//socket.send_to(boost::asio::buffer("Hello from chimera.", 19), remote_endpoint, 0, err);

			//socket.close();

			BoostUDP gmoog("127.0.0.1", 13999);
			//std::vector<unsigned char> dat{0xff, 0xff , 0xff , 0xff, 0xff , 0xff};
			std::vector<int> dat(99999, 0x1f);
			gmoog.write(dat);
		}

	};
}