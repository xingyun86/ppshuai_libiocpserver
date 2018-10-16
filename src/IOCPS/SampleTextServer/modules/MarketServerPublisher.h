#include <string>
#include <iostream>
#include <boost/asio/ip/address.hpp>
#include <boost/format.hpp>
#include <boost/locale.hpp>
#include <boost/asio/deadline_timer.hpp>

#include <redisclient/redisasyncclient.h>

using namespace redisclient;

static const std::string channelName = "unique-redis-channel-name-realtick";
static const boost::posix_time::milliseconds timeout(300);

class CRedisPublisher
{
public:
	CRedisPublisher(boost::asio::io_service &ioService,
		const boost::asio::ip::address &address,
		unsigned short port)
		: ioService(ioService), 
		publishTimer(ioService),
		connectPublisherTimer(ioService),
		address(address), port(port),
		publisher(ioService)
	{
		publisher.installErrorHandler(std::bind(&CRedisPublisher::connectPublisher, this));
	}

	void publish(const std::string & channel_name, const std::string &str)
	{
		publisher.publish(channel_name, str);
	}
	void publish(const std::string &str)
	{
		publisher.publish(channelName, str);
	}

	void start()
	{
		connectPublisher();
	}
	void stop()
	{
		std::cout << "disconnectPublisher\n";

		publisher.disconnect();
		publishTimer.cancel();
	}
protected:
	void errorPubProxy(const std::string &)
	{
		publishTimer.cancel();
		connectPublisher();
	}

	void connectPublisher()
	{
		std::cerr << "connectPublisher\n";

		if (publisher.state() == RedisAsyncClient::State::Connected)
		{
			std::cerr << "disconnectPublisher\n";

			publisher.disconnect();
			publishTimer.cancel();
		}

		boost::asio::ip::tcp::endpoint endpoint(address, port);
		publisher.connect(endpoint,
			std::bind(&CRedisPublisher::onPublisherConnected, this, std::placeholders::_1));
	}

	void callLater(boost::asio::deadline_timer &timer,
		void(CRedisPublisher::*callback)())
	{
		std::cerr << "callLater\n";
		timer.expires_from_now(timeout);
		timer.async_wait([callback, this](const boost::system::error_code &ec) {
			if (!ec)
			{
				(this->*callback)();
			}
		});
	}

	void onPublishTimeout()
	{
		//static size_t counter = 0;
		//std::string msg = str(boost::format("message %1%") % counter++);
		/*std::string sGBK = "{\"code\":200,\"data\":{\"snapshot\":{\"XAUUSD\":[\"»Æ½ð\",1269.85,2.67,0.21,1271.09,1266.83,1267.1799999999998,1267.1799999999998,1529660173,0,\"TRADE\",\"commodity\",2,1366.13,1204.95],\"fields\":[\"prod_name\",\"last_px\",\"px_change\",\"px_change_rate\",\"high_px\",\"low_px\",\"open_px\",\"preclose_px\",\"update_time\",\"business_amount\",\"trade_status\",\"securities_type\",\"price_precision\",\"week_52_high\",\"week_52_low\"]}}}";
		std::string sUTF8 = boost::locale::conv::between(sGBK, "UTF-8", "GBK");
		msg = sUTF8;*/
		if (publisher.state() == RedisAsyncClient::State::Connected)
		{
			//std::cerr << "pub " << msg << "\n";
			//publish(msg);
		}

		callLater(publishTimer, &CRedisPublisher::onPublishTimeout);
	}

	void onPublisherConnected(boost::system::error_code ec)
	{
		if (ec)
		{
			std::cerr << "onPublisherConnected: can't connect to redis: " << ec.message() << "\n";
			callLater(connectPublisherTimer, &CRedisPublisher::connectPublisher);
		}
		else
		{
			std::cerr << "onPublisherConnected ok\n";
			callLater(publishTimer, &CRedisPublisher::onPublishTimeout);
		}
	}

	void onMessage(const std::vector<char> &buf)
	{
		std::string s(buf.begin(), buf.end());
		std::cout << "onMessage: " << s << "\n";
	}

private:
	boost::asio::io_service &ioService;
	boost::asio::deadline_timer publishTimer;
	boost::asio::deadline_timer connectPublisherTimer;
	const boost::asio::ip::address address;
	const unsigned short port;

	RedisAsyncClient publisher;
};
#include <redisclient\redissyncclient.h>
__inline static BOOL sync_test(char * pKey, char * pTestData = "")
{
	//putenv("REDIS_HOST=192.168.31.86");
	const char * host = getenv("REDIS_HOST");
	boost::asio::ip::address address = boost::asio::ip::address::from_string(host ? host : "127.0.0.1");
	//boost::asio::ip::address address = boost::asio::ip::address::from_string("127.0.0.1");
	const unsigned short port = 6379;
	boost::asio::ip::tcp::endpoint endpoint(address, port);

	boost::asio::io_service ioService;
	redisclient::RedisSyncClient redis(ioService);
	boost::system::error_code ec;

	redis.connect(endpoint, ec);

	if (ec)
	{
		std::cerr << "Can't connect to redis: " << ec.message() << std::endl;
		return EXIT_FAILURE;
	}

	redisclient::RedisValue result;

	result = redis.command("SET", { "key", pTestData });

	if (result.isError())
	{
		std::cerr << "SET error: " << result.toString() << "\n";
		return EXIT_FAILURE;
	}

	result = redis.command("GET", { "key" });

	if (result.isOk())
	{
		std::cout << "GET: " << result.toString() << "\n";
		return EXIT_SUCCESS;
	}
	else
	{
		std::cerr << "GET error: " << result.toString() << "\n";
		return EXIT_FAILURE;
	}
}
__inline static int test_main(int argc, char ** argv)
{
	putenv("REDIS_HOST=192.168.31.86");
	const char * host = getenv("REDIS_HOST");
	boost::asio::ip::address address = boost::asio::ip::address::from_string(host ? host : "127.0.0.1");
	//boost::asio::ip::address address = boost::asio::ip::address::from_string("127.0.0.1");
	const unsigned short port = 6379;

	boost::asio::io_service ioService;

	CRedisPublisher client(ioService, address, port);

	client.start();
	ioService.run();

	std::cerr << "done\n";

	return 0;
}
