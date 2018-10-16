#pragma once

#include <MacroDefine.h>

std::shared_ptr<redisclient::RedisSyncClient> create_sync_client(std::shared_ptr<boost::asio::io_service> & ioservice, std::shared_ptr<boost::asio::ip::tcp::endpoint> & endpoint)
{
	const char * host = getenv("REDIS_HOST");
	//boost::asio::io_service ioservice;
	ioservice = std::shared_ptr<boost::asio::io_service>(new boost::asio::io_service);
	std::shared_ptr<redisclient::RedisSyncClient> redissyncclient = std::shared_ptr<redisclient::RedisSyncClient>(new redisclient::RedisSyncClient(*ioservice));
	boost::asio::ip::address address = boost::asio::ip::address::from_string(host ? host : "127.0.0.1");
	//boost::asio::ip::address address = boost::asio::ip::address::from_string("127.0.0.1");
	const unsigned short port = 6379;
	//boost::asio::ip::tcp::endpoint endpoint(address, port);
	endpoint = std::shared_ptr<boost::asio::ip::tcp::endpoint>(new boost::asio::ip::tcp::endpoint(address, port));

	boost::system::error_code ec;

	redissyncclient->connect(*endpoint, ec);
	if (ec)
	{
		std::cerr << "Can't connect to redis: " << ec.message() << std::endl;
		redissyncclient = nullptr;
	}
	
	return redissyncclient;
}
int redisclient_main(int, char **)
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

	result = redis.command("SET", { "key", "value" });

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
class CCRedisClient
{
private:
	CCRedisClient() {}
	~CCRedisClient() {}

public:
	void InitEnv()
	{
		m_redissyncclient = create_sync_client(m_ioservice, m_endpoint);
		//const char * host = getenv("REDIS_HOST");
		//m_ioservice = std::shared_ptr<boost::asio::io_service>(new boost::asio::io_service);
		//m_redissyncclient = std::shared_ptr<redisclient::RedisSyncClient>(new redisclient::RedisSyncClient(*m_ioservice));
		//boost::asio::ip::address address = boost::asio::ip::address::from_string(host ? host : "127.0.0.1");
		//const unsigned short port = 6379;
		//boost::asio::ip::tcp::endpoint endpoint(address, port);
		//m_endpoint = std::shared_ptr<boost::asio::ip::tcp::endpoint>(new boost::asio::ip::tcp::endpoint(address, port));
		//boost::system::error_code ec;
		//m_redissyncclient->connect(*m_endpoint, ec);
		//if (ec)
		//{
		//	std::cerr << "Can't connect to redis: " << ec.message() << std::endl;
		//	m_redissyncclient = nullptr;
		//}
	}
public:
	std::shared_ptr<redisclient::RedisSyncClient> m_redissyncclient;
private:
	std::shared_ptr<boost::asio::io_service> m_ioservice;
	std::shared_ptr<boost::asio::ip::tcp::endpoint> m_endpoint;
};

template <class T>
class CSingleton
{
protected:    
	CSingleton() {};
	~CSingleton() {};

private:
	CSingleton(const CSingleton&) {};//½ûÖ¹¿½±´
	CSingleton& operator=(const CSingleton&) {};//½ûÖ¹¸³Öµ
	static T* m_instance;

public:
	static T* GetInstance()
	{
		return m_instance;
	}
};
template <class T>
T* CSingleton<T>::m_instance = new T();

class CRedisClient
{
private:
	static const CRedisClient* m_instance;
	CRedisClient() {}
	~CRedisClient() {}
	
public:
	void InitEnv()
	{
		m_redissyncclient = create_sync_client(m_ioservice, m_endpoint);
		//const char * host = getenv("REDIS_HOST");
		//m_ioservice = std::shared_ptr<boost::asio::io_service>(new boost::asio::io_service);
		//m_redissyncclient = std::shared_ptr<redisclient::RedisSyncClient>(new redisclient::RedisSyncClient(*m_ioservice));
		//boost::asio::ip::address address = boost::asio::ip::address::from_string(host ? host : "127.0.0.1");
		//const unsigned short port = 6379;
		//boost::asio::ip::tcp::endpoint endpoint(address, port);
		//m_endpoint = std::shared_ptr<boost::asio::ip::tcp::endpoint>(new boost::asio::ip::tcp::endpoint(address, port));
		//boost::system::error_code ec;
		//m_redissyncclient->connect(*m_endpoint, ec);
		//if (ec)
		//{
		//	std::cerr << "Can't connect to redis: " << ec.message() << std::endl;
		//	m_redissyncclient = nullptr;
		//}
	}
	static const CRedisClient* getInstance()
	{
		//if (!m_instance->m_redissyncclient)
		{
			//m_instance->InitEnv();
		}
		return m_instance;
	}
public:
	std::shared_ptr<boost::asio::io_service> m_ioservice;
	std::shared_ptr<boost::asio::ip::tcp::endpoint> m_endpoint;
	std::shared_ptr<redisclient::RedisSyncClient> m_redissyncclient;
};

//Íâ²¿³õÊ¼»¯ before invoke main
const CRedisClient* CRedisClient::m_instance = new CRedisClient();
