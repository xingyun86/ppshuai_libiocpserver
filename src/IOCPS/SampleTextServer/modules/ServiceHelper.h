#pragma once

#include <MacroDefine.h>

bool common_CreateJsonData(std::map<std::string, std::string> & odata, std::string &str)
{
	std::stringstream stream;
	try
	{
		ptree info;
		ptree data;
		info.put("weight", true);		
		BOOST_FOREACH(auto &v, odata)
		{
			ptree pt;
			std::stringstream sstream(v.second);
			read_json(sstream, pt);
			//ptree pt_item;
			//pt_item.put("phone", 123);
			data.push_back(std::make_pair(v.first, pt));
		}
		info.put_child("data", data);
		write_json(stream, info);
	}
	catch (ptree_error pt)
	{
		pt.what();
		return false;
	}
	str = stream.str();
	return true;
}
bool common_ParseJsonData(std::map<std::string, std::string> & odata, std::string &str)
{
	try
	{
		ptree pt;
		std::stringstream stream(str);
		read_json(stream, pt);
		BOOST_FOREACH(auto &v, odata)
		{
			v.second = pt.get<std::string>(v.first);
		}
	}
	catch (ptree_error pt)
	{
		pt.what();
		return false;
	}
	return true;
}

//static const std::string channelName = "unique-redis-channel-name-example";
//static const boost::posix_time::seconds timeout(1);

template <typename EndpointType>
class CMarketServer {

	// See https://wiki.mozilla.org/Security/Server_Side_TLS for more details about
	// the TLS modes. The code below demonstrates how to implement both the modern
	typedef enum tls_mode {
		MOZILLA_INTERMEDIATE = 1,//1024bits
		MOZILLA_MODERN = 2,//2048bits
	};

public:
	CMarketServer() {
		// Reset the log channels
		m_server.clear_access_channels(websocketpp::log::alevel::all);
		m_server.clear_error_channels(websocketpp::log::elevel::all);

		m_server.set_access_channels(websocketpp::log::alevel::all);
		m_server.set_error_channels(websocketpp::log::elevel::all);

		// Initialize Asio Transport
		m_server.init_asio();
		m_server.set_reuse_addr(true);

		// Register handler callbacks
		m_server.set_tls_init_handler(bind(&CMarketServer::on_tls_init_sslv23, this, ::_1));
		m_server.set_open_handler(bind(&CMarketServer::on_open, this, ::_1));
		m_server.set_close_handler(bind(&CMarketServer::on_close, this, ::_1));
		m_server.set_http_handler(bind(&CMarketServer::on_http, this, ::_1));
		m_server.set_message_handler(bind(&CMarketServer::on_message, this, ::_1, ::_2));
		m_server.set_fail_handler(bind(&CMarketServer::on_fail, this, ::_1));
	}
	CMarketServer(boost::asio::io_service & ios, 
		const std::string & channelName, 
		const boost::posix_time::milliseconds & timeout, 
		const boost::asio::ip::address & address, 
		unsigned short port, 
		CMTTickInstance * pTickInstance)
		: m_ios(ios), 
		m_connectSubscriberTimer(ios), 
		m_channelName(channelName), 
		m_timeout(timeout), 
		m_address(address), 
		m_port(port), 
		m_subscriber(ios), 
		m_pTickInstance(pTickInstance)
	{
		// Reset the log channels
		m_server.clear_access_channels(websocketpp::log::alevel::all);
		m_server.clear_error_channels(websocketpp::log::elevel::all);

		m_server.set_access_channels(websocketpp::log::alevel::all);
		m_server.set_error_channels(websocketpp::log::elevel::all);

		// initialize asio with our external io_service rather than an internal one

		// Initialize Asio Transport
		m_server.init_asio(&ios);
		m_server.set_reuse_addr(true);

		// Register handler callbacks
		//m_server.set_tls_init_handler(bind(&MarketServer::on_tls_init_tlsv1, this, ::_1));
		m_server.set_tls_init_handler(bind(&CMarketServer::on_tls_init_sslv23, this, ::_1));
		m_server.set_open_handler(bind(&CMarketServer::on_open, this, ::_1));
		m_server.set_close_handler(bind(&CMarketServer::on_close, this, ::_1));
		m_server.set_http_handler(bind(&CMarketServer::on_http, this, ::_1));
		m_server.set_message_handler(bind(&CMarketServer::on_message, this, ::_1, ::_2));
		m_server.set_fail_handler(bind(&CMarketServer::on_fail, this, ::_1));

		m_subscriber.installErrorHandler(std::bind(&CMarketServer::connectSubscriber, this));
	}

	static std::string get_password(
		std::size_t max_length,  // The maximum size for a password.
		context::password_purpose purpose // Whether password is for reading or writing.
	) {
		return "test";
		//return "123456";
	}

	static std::string get_password_callback() {
		return "test";
		//return "123456";
	}

	context_ptr on_tls_init_tlsv1(connection_hdl hdl) {
		std::cout << "on_tls_init called with hdl: " << hdl.lock().get() << std::endl;
		//context_ptr ctx(new context(context::tlsv1));
		context_ptr ctx = websocketpp::lib::make_shared<context>(context::tlsv1);
		try {
			ctx->set_options(context::default_workarounds |
				context::no_sslv2 |
				context::no_sslv3 |
				context::single_dh_use);
			ctx->set_password_callback(bind(&get_password, ::_1, ::_2));
			//ctx->set_password_callback(bind(&get_password_callback));
			ctx->use_certificate_chain_file("server.pem");
			ctx->use_private_key_file("server.pem", context::pem);
		}
		catch (std::exception& e) {
			std::cout << e.what() << std::endl;
		}
		return ctx;
	}

	context_ptr on_tls_init_sslv23(connection_hdl hdl) {
		//tls_mode mode = MOZILLA_MODERN; 
		tls_mode mode = MOZILLA_INTERMEDIATE;
		std::cout << "on_tls_init called with hdl: " << hdl.lock().get() << std::endl;
		std::cout << "using TLS mode: " << (mode == MOZILLA_MODERN ? "Mozilla Modern" : "Mozilla Intermediate") << std::endl;

		context_ptr ctx = websocketpp::lib::make_shared<context>(context::sslv23);

		try {
			if (mode == MOZILLA_MODERN) {
				// Modern disables TLSv1
				ctx->set_options(context::default_workarounds |
					context::no_sslv2 |
					context::no_sslv3 |
					context::no_tlsv1 |
					context::single_dh_use);
			}
			else {
				ctx->set_options(context::default_workarounds |
					context::no_sslv2 |
					context::no_sslv3 |
					context::single_dh_use);
			}
			//ctx->set_password_callback(bind(&get_password, ::_1, ::_2));
			ctx->set_password_callback(bind(&get_password_callback));
			ctx->use_certificate_chain_file("server.pem");
			ctx->use_private_key_file("server.pem", context::pem);

			// Example method of generating this file:
			// `openssl dhparam -out dh.pem 2048`
			// Mozilla Intermediate suggests 1024 as the minimum size to use
			// Mozilla Modern suggests 2048 as the minimum size to use.
			ctx->use_tmp_dh_file("dh.pem");

			std::string ciphers = "";

			if (mode == MOZILLA_MODERN) {
				ciphers = "ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-AES256-GCM-SHA384:DHE-RSA-AES128-GCM-SHA256:DHE-DSS-AES128-GCM-SHA256:kEDH+AESGCM:ECDHE-RSA-AES128-SHA256:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA:ECDHE-ECDSA-AES128-SHA:ECDHE-RSA-AES256-SHA384:ECDHE-ECDSA-AES256-SHA384:ECDHE-RSA-AES256-SHA:ECDHE-ECDSA-AES256-SHA:DHE-RSA-AES128-SHA256:DHE-RSA-AES128-SHA:DHE-DSS-AES128-SHA256:DHE-RSA-AES256-SHA256:DHE-DSS-AES256-SHA:DHE-RSA-AES256-SHA:!aNULL:!eNULL:!EXPORT:!DES:!RC4:!3DES:!MD5:!PSK";
			}
			else {
				ciphers = "ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-AES256-GCM-SHA384:DHE-RSA-AES128-GCM-SHA256:DHE-DSS-AES128-GCM-SHA256:kEDH+AESGCM:ECDHE-RSA-AES128-SHA256:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA:ECDHE-ECDSA-AES128-SHA:ECDHE-RSA-AES256-SHA384:ECDHE-ECDSA-AES256-SHA384:ECDHE-RSA-AES256-SHA:ECDHE-ECDSA-AES256-SHA:DHE-RSA-AES128-SHA256:DHE-RSA-AES128-SHA:DHE-DSS-AES128-SHA256:DHE-RSA-AES256-SHA256:DHE-DSS-AES256-SHA:DHE-RSA-AES256-SHA:AES128-GCM-SHA256:AES256-GCM-SHA384:AES128-SHA256:AES256-SHA256:AES128-SHA:AES256-SHA:AES:CAMELLIA:DES-CBC3-SHA:!aNULL:!eNULL:!EXPORT:!DES:!RC4:!MD5:!PSK:!aECDH:!EDH-DSS-DES-CBC3-SHA:!EDH-RSA-DES-CBC3-SHA:!KRB5-DES-CBC3-SHA";
			}

			if (SSL_CTX_set_cipher_list(ctx->native_handle(), ciphers.c_str()) != 1) {
				std::cout << "Error setting cipher list" << std::endl;
			}
		}
		catch (std::exception& e) {
			std::cout << "Exception: " << e.what() << std::endl;
		}
		return ctx;
	}
	
	void start_accept(uint16_t port) {

		// Connect to redis
		connectSubscriber();

		// listen on specified port
		m_server.listen(port);

		// Start the server accept loop
		m_server.start_accept();
	}
	
	void run(uint16_t port) {

		// listen on specified port
		// Start the server accept loop
		start_accept(port);

		// Start the ASIO io_service run loop
		try {
			m_server.run();
		}
		catch (const std::exception & e) {
			std::cout << e.what() << std::endl;
		}
	}

	void on_open(connection_hdl hdl) {
		{
			boost::lock_guard<boost::mutex> guard(m_action_lock);
			//std::cout << "on_open" << std::endl;
			m_actions.push_back(ServerAction<typename EndpointType>(SUBSCRIBE, hdl));
		}
		m_action_cond.notify_all();
	}

	void on_close(connection_hdl hdl) {
		{
			boost::lock_guard<boost::mutex> guard(m_action_lock);
			//std::cout << "on_close" << std::endl;
			m_actions.push_back(ServerAction<typename EndpointType>(UNSUBSCRIBE, hdl));
		}
		m_action_cond.notify_all();
	}

	void on_message(connection_hdl hdl, typename EndpointType::message_ptr msg) {
		// queue message up for sending by processing thread
		{
			boost::lock_guard<boost::mutex> guard(m_action_lock);
			if (msg->get_payload() == "hello") {
				return;
			}

			//std::cout << "on_message" << std::endl;
			m_actions.push_back(ServerAction<typename EndpointType>(MESSAGE, hdl, msg));
		}
		m_action_cond.notify_all();
	}

	void on_http(connection_hdl hdl) {
		typename EndpointType::connection_ptr con = m_server.get_con_from_hdl(hdl);

		std::string strResource = boost::to_lower_copy(con->get_resource());

		websocketpp::http::parser::request req = con->get_request();
		websocketpp::http::parser::response resp = con->get_response();

		std::string uri = req.get_uri();
		std::string raw = req.raw();
		std::string strHeader = req.get_header("host");
		std::string strBody = req.get_body();
		std::string strMethod = boost::to_lower_copy(req.get_method());
		std::string strVersion = req.get_version();
		std::string strActionParams = req.get_header("ActionParams");

		if (g_service_interface_apimap.find(strResource) != g_service_interface_apimap.end())
		{
			con->replace_header("Server", "PPSHUAI Server 2018");
			con->append_header("Access-Control-Allow-Credentials", "true");
			con->append_header("Access-Control-Allow-Methods", "POST, GET, OPTIONS, DELETE, PUT");
			con->append_header("Access-Control-Allow-Headers", "ACTIONPARAMS");
			con->append_header("Access-Control-Allow-Origin", "*");

			if (!strMethod.compare("options"))
			{
				con->set_body("options valid!");
				con->set_status(websocketpp::http::status_code::ok);
			}
			else if (!strMethod.compare("get") || !strMethod.compare("post"))
			{
				switch (g_service_interface_apimap.at(con->get_resource()))
				{
				case si_apitype_test:
				{
					con->set_body("This is a test!");
				}
				break;

				case si_apitype_market_kline://from to(暂不可用)
				{
					std::map<std::string, std::string> odata = { { "symbol","" },{ "from","" },{ "to","" }, };
					if (common_ParseJsonData(odata, strActionParams))
					{
						MTAPIRES mtapiRes = MT_RET_OK_NONE;
						MTChartBar * pMTChartBar = NULL;
						UINT uChartBars = 0L;
						//mtapiRes = g_manager->Chart(boost::locale::conv::utf_to_utf<wchar_t>(odata.at("symbol")).c_str(), std::stoull(odata.at("from")), std::stoull(odata.at("to")), pMTChartBar, uChartBars);
						// --- 
						if (mtapiRes == MT_RET_OK)
						{
							if (uChartBars)
							{
								//con->set_body("[ChartGet][count" + std::to_string(uChartBars) + "]");
								std::string str = "[ChartGet][count" + std::to_string(uChartBars) + "]";
								str.append("{\"" + odata.at("symbol") + "\":[");
								for (UINT uIdx = 0; uIdx < uChartBars; uIdx++)
								{
									if (uIdx)
									{
										str.append(",");
									}
									str.append("{");
									str.append("\"datetime\":\"").append(std::to_string(pMTChartBar[uIdx].datetime)).append("\",");
									str.append("\"open\":\"").append(std::to_string(pMTChartBar[uIdx].open)).append("\",");
									str.append("\"high\":\"").append(std::to_string(pMTChartBar[uIdx].high)).append("\",");
									str.append("\"low\":\"").append(std::to_string(pMTChartBar[uIdx].low)).append("\",");
									str.append("\"close\":\"").append(std::to_string(pMTChartBar[uIdx].close)).append("\",");
									str.append("\"tick_volume\":\"").append(std::to_string(pMTChartBar[uIdx].tick_volume)).append("\",");
									str.append("\"spread\":\"").append(std::to_string(pMTChartBar[uIdx].spread)).append("\",");
									str.append("\"volume\":\"").append(std::to_string(SMTMath::VolumeToDouble(pMTChartBar[uIdx].volume))).append("\"");
									str.append("}");
								}
								str.append("]}");
								con->set_body(str);
							}
							else
							{
								con->set_body("[ChartGet][count" + std::to_string(uChartBars) + "] is null");
							}
						}
						else
						{
							con->set_body("[ChartGet][error=" + std::to_string(mtapiRes) + "]");
						}
					}
					else
					{
						con->set_body("Action params invalid!");
					}
				}
				break;
				case si_apitype_market_kline_new://from distince(暂不可用)
				{
					std::map<std::string, std::string> odata = { { "symbol","" },{ "from","" },{ "distance","" }, };
					if (common_ParseJsonData(odata, strActionParams))
					{
						MTAPIRES mtapiRes = MT_RET_OK_NONE;
						MTChartBar * pMTChartBar = NULL;
						UINT uChartBars = 0L;
						INT64 nFromTime = 0LL;
						INT64 nCurrTime = 0LL;// g_manager->TimeCurrent();
						if (!odata.at("from").compare("now"))
						{
							nFromTime = nCurrTime - 60 * 60 * 24 * std::stoi(odata.at("distance"));
						}
						else
						{
							nFromTime = std::stoll(odata.at("from")) - 60 * 60 * 24 * std::stoi(odata.at("distance"));
						}

						//mtapiRes = g_manager->ChartGet(boost::locale::conv::utf_to_utf<wchar_t>(odata.at("symbol")).c_str(), nFromTime, nCurrTime, pMTChartBar, uChartBars);
						// --- 
						if (mtapiRes == MT_RET_OK)
						{
							if (uChartBars)
							{
								//con->set_body("[ChartGet][count" + std::to_string(uChartBars) + "]");
								std::string str = "[ChartGet][count" + std::to_string(uChartBars) + "]";
								str.append("{\"" + odata.at("symbol") + "\":[");
								for (UINT uIdx = 0; uIdx < uChartBars; uIdx++)
								{
									if (uIdx)
									{
										str.append(",");
									}
									str.append("{");
									str.append("\"datetime\":\"").append(std::to_string(pMTChartBar[uIdx].datetime)).append("\",");
									str.append("\"open\":\"").append(std::to_string(pMTChartBar[uIdx].open)).append("\",");
									str.append("\"high\":\"").append(std::to_string(pMTChartBar[uIdx].high)).append("\",");
									str.append("\"low\":\"").append(std::to_string(pMTChartBar[uIdx].low)).append("\",");
									str.append("\"close\":\"").append(std::to_string(pMTChartBar[uIdx].close)).append("\",");
									str.append("\"tick_volume\":\"").append(std::to_string(pMTChartBar[uIdx].tick_volume)).append("\",");
									str.append("\"spread\":\"").append(std::to_string(pMTChartBar[uIdx].spread)).append("\",");
									str.append("\"volume\":\"").append(std::to_string(SMTMath::VolumeToDouble(pMTChartBar[uIdx].volume))).append("\"");
									str.append("}");
								}
								str.append("]}");
								con->set_body(str);
							}
							else
							{
								con->set_body("[ChartGet][count" + std::to_string(uChartBars) + "] is null");
							}
							m_pTickInstance->getManagerApi()->Free(pMTChartBar);
							pMTChartBar = NULL;
						}
						else
						{
							con->set_body("[ChartGet][error=" + std::to_string(mtapiRes) + "]");
						}
					}
					else
					{
						con->set_body("Action params invalid!");
					}
				}
				break;

				case si_apitype_trade_position_open://市价/限价开仓
				{
					MTAPIRES mtapiRes = MT_RET_OK_NONE;
					IMTRequest * pIMTReq = NULL;

					///////////////////////////////////////////////////////////////////////////////////////////
					// 市价报单（开仓）
					
					//--- create request
					IMTRequest *request = m_pTickInstance->getManagerApi()->RequestCreate();
					IMTRequest *response = m_pTickInstance->getManagerApi()->RequestCreate();
					CDealerSink sink;
					UINT64      deal_id = 0;
					UINT        id = 0;
					if (request && response && sink.Initialize(response))
					{
						std::map<std::string, std::string> odata = { { "account","" },{ "method","" },{ "symbol","" },{ "price","" },{ "volume","" },{ "tp","" },{ "sl","" },{ "timeexpiration","" }, };
						if (common_ParseJsonData(odata, strActionParams))
						{
							//--- buy 1.00 EURUSD
							request->Clear();
							request->Login(std::stoull(odata.at("account")));
							request->Action(IMTRequest::TA_DEALER_POS_EXECUTE);
							request->Type(std::stoi(odata.at("method")));
							request->Volume(SMTMath::VolumeToInt(std::stod(odata.at("volume"))));
							request->Symbol(boost::locale::conv::utf_to_utf<wchar_t>(odata.at("symbol")).c_str());
							request->PriceOrder(std::stod(odata.at("price")));
							if (odata.at("sl").length())
							{
								request->PriceSL(std::stod(odata.at("sl")));
							}
							if (odata.at("tp").length())
							{
								request->PriceTP(std::stod(odata.at("tp")));
							}
							if (odata.at("timeexpiration").length())
							{
								request->TimeExpiration(std::stoull(odata.at("timeexpiration")));
							}
							mtapiRes = m_pTickInstance->getManagerApi()->DealerSend(request, &sink, id);
							if (mtapiRes == MT_RET_OK)
							{
								mtapiRes = sink.Wait(MT5_TIMEOUT_DEALER);
							}
							if (mtapiRes == MT_RET_REQUEST_DONE)
							{
								MTAPISTR strapi;
								response->Print(strapi);
								wprintf_s(L"%s\n", strapi);
								wprintf_s(L"Deal:  #%I64u\n", response->ResultDeal());
								wprintf_s(L"Order: #%I64u\n", response->ResultOrder());
								con->set_body("[Open]Success![error=" + std::to_string(mtapiRes) + "]--"
									"(" + "ResultDeal=" + std::to_string(response->ResultDeal()) + ","
									+ "ResultOrder=" + std::to_string(response->ResultOrder()) + ","
									+ "ResultVolume=" + std::to_string(SMTMath::VolumeToDouble(response->ResultVolume())) + ","
									+ "ResultPrice=" + std::to_string(response->ResultPrice()) + ")"
									+ "--from ppshuai![" + strActionParams + "]" + boost::locale::conv::utf_to_utf<char>(strapi));
							}
						}
						request->Release();
						response->Release();
					}
					// --- 
					/*if (pIMTReq == NULL)
					{
						con->set_body("[Open]Faliure![TradeRequestCreate error]--from ppshuai!");
					}
					else
					{
						pIMTReq->Clear();//清空内存
						pIMTReq->Action(std::stoul(cAction));//IMTRequest::EnTradeActions::TA_INSTANT //开单
						pIMTReq->Flags(std::stoul(cFlags));//IMTRequest::EnTradeActionFlags::TA_FLAG_MARKET //市价
						pIMTReq->Type(std::stoul(cType));//IMTOrder::EnOrderType::OP_BUY //开仓方向
						pIMTReq->Login(std::stoull(cLogin));//交易账号
						pIMTReq->Symbol(boost::locale::conv::utf_to_utf<wchar_t>(cSymbol).c_str());//交易品种
						pIMTReq->Volume(SMTMath::VolumeToInt(std::stod(cVolume)));//开仓量(手数)
						pIMTReq->PriceOrder(std::stod(cPriceOrder));//开仓价格
						//pIMTReq->Position(std::stoull(cPosition));//开仓单号
						pIMTReq->TypeFill(std::stoul(cTypeFill));//IMTOrder::EnOrderFilling::ORDER_FILL_FOK
						pIMTReq->PriceDeviation(std::stoull(cPriceDeviation));//价格偏差

						std::map<std::string, std::string> odata = { { "account","" },{ "method","" },{ "symbol","" },{ "price","" },{ "volume","" }, };
						if (common_ParseJsonData(odata, strActionParams))
						{
							pIMTReq->Type(std::stoul(odata.at("method")));//IMTOrder::EnOrderType::OP_BUY //报单方向
							pIMTReq->Login(std::stoull(odata.at("account")));//交易账号
							pIMTReq->Symbol(boost::locale::conv::utf_to_utf<wchar_t>(odata.at("symbol")).c_str());//交易品种
							pIMTReq->Volume(SMTMath::VolumeToInt(std::stod(odata.at("volume"))));//开仓量(手数)
							pIMTReq->PriceOrder(std::stod(odata.at("price")));//开仓价格
																			  //pIMTReq->PriceOrder(std::stod(odata.at("close_id")));//开仓单号
							mtapiRes = 0;// g_manager->TradeRequest(pIMTReq);
							// --- 
							if (mtapiRes == MT_RET_OK)
							{
								con->set_body("[Open]Success![error=" + std::to_string(mtapiRes) + "]--"
									"(" + "action=" + cAction + ","
									+ "flags=" + cFlags + ","
									+ "type=" + cType + ","
									+ "login=" + cLogin + ","
									+ "symbol=" + cSymbol + ","
									+ "volume=" + cVolume + ","
									+ "price_order=" + cPriceOrder + ","
									+ "TypeFill=" + cTypeFill + ","
									+ "PriceDeviation=" + cPriceDeviation + ")"
									+ "--from ppshuai![" + strActionParams + "]" + std::to_string(pIMTReq->ResultMarketLast()));
							}
							else
							{
								con->set_body("[Open]Failure![error=" + std::to_string(mtapiRes) + "]--"
									"(" + "action=" + cAction + ","
									+ "flags=" + cFlags + ","
									+ "type=" + cType + ","
									+ "login=" + cLogin + ","
									+ "symbol=" + cSymbol + ","
									+ "volume=" + cVolume + ","
									+ "price_order=" + cPriceOrder + ","
									+ "TypeFill=" + cTypeFill + ","
									+ "PriceDeviation=" + cPriceDeviation + ")"
									+ "--from ppshuai![" + strActionParams + "]" + std::to_string(pIMTReq->ResultMarketLast()));
							}
						}
						else
						{
							con->set_body("[Open]Failure!Parameters invalid!");
						}
						pIMTReq->Release();
					}*/
				}
				break;
				case si_apitype_trade_position_close://市价/限价平仓
				{
					MTAPIRES mtapiRes = MT_RET_OK_NONE;
					IMTRequest * pIMTReq = NULL;

					///////////////////////////////////////////////////////////////////////////////////////////
					// 市价报单（平仓）

					//--- create request
					IMTRequest *request = m_pTickInstance->getManagerApi()->RequestCreate();
					IMTRequest *response = m_pTickInstance->getManagerApi()->RequestCreate();
					CDealerSink sink;
					UINT64      deal_id = 0;
					UINT        id = 0;
					if (request && response && sink.Initialize(response))
					{
						std::map<std::string, std::string> odata = { { "account","" },{ "method","" },{ "symbol","" },{ "price","" },{ "volume","" }, { "position_id","" }, };
						if (common_ParseJsonData(odata, strActionParams))
						{
							//--- sell 1.00 EURUSD
							request->Login(std::stoull(odata.at("account")));
							request->Action(IMTRequest::TA_DEALER_POS_EXECUTE);
							request->Type(std::stoi(odata.at("method")));
							request->Volume(SMTMath::VolumeToInt(std::stod(odata.at("volume"))));
							request->Symbol(boost::locale::conv::utf_to_utf<wchar_t>(odata.at("symbol")).c_str());
							request->PriceOrder(std::stod(odata.at("price")));
							request->Position(std::stoull(odata.at("position_id")));
							mtapiRes = m_pTickInstance->getManagerApi()->DealerSend(request, &sink, id);
							if (mtapiRes == MT_RET_OK)
							{
								mtapiRes = sink.Wait(MT5_TIMEOUT_DEALER);
							}
							if (mtapiRes == MT_RET_REQUEST_DONE)
							{
								MTAPISTR strapi;
								response->Print(strapi);
								wprintf_s(L"%s\n", strapi);
								wprintf_s(L"Deal:  #%I64u\n", response->ResultDeal());
								wprintf_s(L"Order: #%I64u\n", response->ResultOrder());
								con->set_body("[Open]Success![error=" + std::to_string(mtapiRes) + "]--"
									"(" + "ResultDeal=" + std::to_string(response->ResultDeal()) + ","
									+ "ResultOrder=" + std::to_string(response->ResultOrder()) + ","
									+ "ResultVolume=" + std::to_string(SMTMath::VolumeToDouble(response->ResultVolume())) + ","
									+ "ResultPrice=" + std::to_string(response->ResultPrice()) + ")"
									+ "--from ppshuai![" + strActionParams + "]" + boost::locale::conv::utf_to_utf<char>(strapi));
							}
						}
						request->Release();
						response->Release();
					}
				}
				break;
				case si_apitype_trade_position_tpsl://持仓添加止盈止损
				{
					MTAPIRES mtapiRes = MT_RET_OK_NONE;

					///////////////////////////////////////////////////////////////////////////////////////////
					// 市价报单（添加止盈止损）
					
					std::map<std::string, std::string> odata = { { "account","" },{ "position_id","" },{ "tp","" },{ "sl","" }, };
					if (common_ParseJsonData(odata, strActionParams) && 
						(odata.at("sl").length() || odata.at("tp").length()))
					{
						//--- create request
						IMTRequest *request = m_pTickInstance->getManagerApi()->RequestCreate();
						IMTRequest *response = m_pTickInstance->getManagerApi()->RequestCreate();
						CDealerSink sink;
						UINT64      deal_id = 0;
						UINT        id = 0;
						if (request && response && sink.Initialize(response))
						{
							request->Clear();
							request->Login(std::stoull(odata.at("account")));
							request->Action(IMTRequest::TA_DEALER_POS_MODIFY);
							if (odata.at("sl").length())
							{
								request->PriceSL(std::stod(odata.at("sl")));
							}
							if (odata.at("tp").length())
							{
								request->PriceTP(std::stod(odata.at("tp")));
							}
							request->Position(std::stoull(odata.at("position_id")));
							mtapiRes = m_pTickInstance->getManagerApi()->DealerSend(request, &sink, id);
							if (mtapiRes == MT_RET_OK)
							{
								mtapiRes = sink.Wait(MT5_TIMEOUT_DEALER);
								if (mtapiRes == MT_RET_REQUEST_DONE)
								{
									MTAPISTR strapi;
									response->Print(strapi);
									wprintf_s(L"%s\n", strapi);
									wprintf_s(L"Deal:  #%I64u\n", response->ResultDeal());
									wprintf_s(L"Order: #%I64u\n", response->ResultOrder());
									con->set_body("[Open]Success![error=" + std::to_string(mtapiRes) + "]--"
										"(" + "ResultDeal=" + std::to_string(response->ResultDeal()) + ","
										+ "ResultOrder=" + std::to_string(response->ResultOrder()) + ","
										+ "ResultVolume=" + std::to_string(SMTMath::VolumeToDouble(response->ResultVolume())) + ","
										+ "ResultPrice=" + std::to_string(response->ResultPrice()) + ")"
										+ "--from ppshuai![" + strActionParams + "]" + boost::locale::conv::utf_to_utf<char>(strapi));
								}
								else
								{
									con->set_body(json_data_make(false, "", "[ModifyPosition]Failed!DealerSend Ok, But no result!"));
								}
							}							
							request->Release();
							response->Release();
						}
						else
						{
							con->set_body(json_data_make(false, "", "[ModifyPosition]Failed!Initialize failed!"));
						}
					}
					else
					{
						con->set_body(json_data_make(false, "", "[ModifyPosition]Failed!Params invalid!"));
					}
				}
				break;
				case si_apitype_trade_order_pending://创建委托单(限价单才可以更新，市价单都是即时成交)
				{
					MTAPIRES mtapiRes = MT_RET_OK_NONE;
					IMTRequest * pIMTReq = NULL;

					///////////////////////////////////////////////////////////////////////////////////////////
					// 委托报单

					//--- create request
					IMTRequest *request = m_pTickInstance->getManagerApi()->RequestCreate();
					IMTRequest *response = m_pTickInstance->getManagerApi()->RequestCreate();
					CDealerSink sink;
					UINT64      deal_id = 0;
					UINT        id = 0;
					if (request && response && sink.Initialize(response))
					{
						std::map<std::string, std::string> odata = { { "account","" },{ "method","" },{ "symbol","" },{ "price","" },{ "volume","" },{ "tp","" },{ "sl","" },{ "timeexpiration","" }, };
						if (common_ParseJsonData(odata, strActionParams))
						{
							//--- buy 1.00 EURUSD
							request->Clear();
							request->Login(std::stoull(odata.at("account")));
							request->Action(IMTRequest::TA_DEALER_ORD_PENDING);
							request->Type(std::stoi(odata.at("method")));
							request->Volume(SMTMath::VolumeToInt(std::stod(odata.at("volume"))));
							request->Symbol(boost::locale::conv::utf_to_utf<wchar_t>(odata.at("symbol")).c_str());
							request->PriceOrder(std::stod(odata.at("price")));
							if (odata.at("sl").length())
							{
								request->PriceSL(std::stod(odata.at("sl")));
							}
							if (odata.at("tp").length())
							{
								request->PriceTP(std::stod(odata.at("tp")));
							}
							if (odata.at("timeexpiration").length())
							{
								request->TimeExpiration(std::stoull(odata.at("timeexpiration")));
							}
							mtapiRes = m_pTickInstance->getManagerApi()->DealerSend(request, &sink, id);
							if (mtapiRes == MT_RET_OK)
							{
								mtapiRes = sink.Wait(MT5_TIMEOUT_DEALER);
							}
							if (mtapiRes == MT_RET_REQUEST_DONE)
							{
								MTAPISTR strapi;
								response->Print(strapi);
								wprintf_s(L"%s\n", strapi);
								wprintf_s(L"Deal:  #%I64u\n", response->ResultDeal());
								wprintf_s(L"Order: #%I64u\n", response->ResultOrder());
								con->set_body("[Open]Success![error=" + std::to_string(mtapiRes) + "]--"
									"(" + "ResultDeal=" + std::to_string(response->ResultDeal()) + ","
									+ "ResultOrder=" + std::to_string(response->ResultOrder()) + ","
									+ "ResultVolume=" + std::to_string(SMTMath::VolumeToDouble(response->ResultVolume())) + ","
									+ "ResultPrice=" + std::to_string(response->ResultPrice()) + ")"
									+ "--from ppshuai![" + strActionParams + "]" + boost::locale::conv::utf_to_utf<char>(strapi));
							}
						}
						request->Release();
						response->Release();
					}
				}
				break;
				case si_apitype_trade_order_update://更新委托单(限价单才可以更新，市价单都是即时成交)
				{
					MTAPIRES mtapiRes = MT_RET_OK_NONE;

					///////////////////////////////////////////////////////////////////////////////////////////
					// 更新委托单
					
					std::map<std::string, std::string> odata = { { "account","" },{ "order_id","" },{ "method","" },{ "symbol","" },{ "price","" },{ "volume","" },{ "tp","" },{ "sl","" },{ "timeexpiration","" }, };
					if (common_ParseJsonData(odata, strActionParams))
					{
						//--- create request
						IMTRequest *request = m_pTickInstance->getManagerApi()->RequestCreate();
						IMTRequest *response = m_pTickInstance->getManagerApi()->RequestCreate();
						CDealerSink sink;
						UINT64      deal_id = 0;
						UINT        id = 0;
						if (request && response && sink.Initialize(response))
						{
							request->Clear();
							request->Action(IMTRequest::TA_DEALER_ORD_MODIFY);
							request->Login(std::stoull(odata.at("account")));
							request->Order(std::stoull(odata.at("order_id")));
							request->Type(std::stoi(odata.at("method")));
							request->Volume(SMTMath::VolumeToInt(std::stod(odata.at("volume"))));
							request->Symbol(boost::locale::conv::utf_to_utf<wchar_t>(odata.at("symbol")).c_str());
							request->PriceOrder(std::stod(odata.at("price")));
							if (odata.at("sl").length())
							{
								request->PriceSL(std::stod(odata.at("sl")));
							}
							if (odata.at("tp").length())
							{
								request->PriceTP(std::stod(odata.at("tp")));
							}
							if (odata.at("timeexpiration").length())
							{
								request->TimeExpiration(std::stoull(odata.at("timeexpiration")));
							}
							mtapiRes = m_pTickInstance->getManagerApi()->DealerSend(request, &sink, id);
							if (mtapiRes == MT_RET_OK)
							{
								mtapiRes = sink.Wait(MT5_TIMEOUT_DEALER);
								if (mtapiRes == MT_RET_REQUEST_DONE)
								{
									MTAPISTR strapi;
									response->Print(strapi);
									wprintf_s(L"%s\n", strapi);
									wprintf_s(L"Deal:  #%I64u\n", response->ResultDeal());
									wprintf_s(L"Order: #%I64u\n", response->ResultOrder());
									con->set_body("[CancelOrder]Success![error=" + std::to_string(mtapiRes) + "]--"
										"(" + "ResultDeal=" + std::to_string(response->ResultDeal()) + ","
										+ "ResultOrder=" + std::to_string(response->ResultOrder()) + ","
										+ "ResultVolume=" + std::to_string(SMTMath::VolumeToDouble(response->ResultVolume())) + ","
										+ "ResultPrice=" + std::to_string(response->ResultPrice()) + ")"
										+ "--from ppshuai![" + strActionParams + "]" + boost::locale::conv::utf_to_utf<char>(strapi));
								}
								else
								{
									con->set_body(json_data_make(false, "", "[CancelOrder]Failed!DealerSend Ok, But no result!"));
								}
							}							
							request->Release();
							response->Release();
						}
						else
						{
							con->set_body(json_data_make(false, "", "[CancelOrder]Failed!Initialize failed!"));
						}
					}
					else
					{
						con->set_body(json_data_make(false, "", "[CancelOrder]Failed!Params invalid!"));
					}
				}
				break;
				case si_apitype_trade_order_cancel://取消委托单(限价单才可以取消，市价单都是即时成交)
				{
					MTAPIRES mtapiRes = MT_RET_OK_NONE;

					///////////////////////////////////////////////////////////////////////////////////////////
					// 取消委托单
					
					std::map<std::string, std::string> odata = { { "account","" },{ "order_id","" }, };
					if (common_ParseJsonData(odata, strActionParams))
					{
						//--- create request
						IMTRequest *request = m_pTickInstance->getManagerApi()->RequestCreate();
						IMTRequest *response = m_pTickInstance->getManagerApi()->RequestCreate();
						CDealerSink sink;
						UINT64      deal_id = 0;
						UINT        id = 0;
						if (request && response && sink.Initialize(response))
						{
							request->Clear();
							request->Login(std::stoull(odata.at("account")));
							request->Action(IMTRequest::TA_DEALER_ORD_REMOVE);
							request->Order(std::stoull(odata.at("order_id")));
							mtapiRes = m_pTickInstance->getManagerApi()->DealerSend(request, &sink, id);
							if (mtapiRes == MT_RET_OK)
							{
								mtapiRes = sink.Wait(MT5_TIMEOUT_DEALER);
								if (mtapiRes == MT_RET_REQUEST_DONE)
								{
									MTAPISTR strapi;
									response->Print(strapi);
									wprintf_s(L"%s\n", strapi);
									wprintf_s(L"Deal:  #%I64u\n", response->ResultDeal());
									wprintf_s(L"Order: #%I64u\n", response->ResultOrder());
									con->set_body("[CancelOrder]Success![error=" + std::to_string(mtapiRes) + "]--"
										"(" + "ResultDeal=" + std::to_string(response->ResultDeal()) + ","
										+ "ResultOrder=" + std::to_string(response->ResultOrder()) + ","
										+ "ResultVolume=" + std::to_string(SMTMath::VolumeToDouble(response->ResultVolume())) + ","
										+ "ResultPrice=" + std::to_string(response->ResultPrice()) + ")"
										+ "--from ppshuai![" + strActionParams + "]" + boost::locale::conv::utf_to_utf<char>(strapi));
								}
								else
								{
									con->set_body(json_data_make(false, "", "[CancelOrder]Failed!DealerSend Ok, But no result!"));
								}
							}							
							request->Release();
							response->Release();
						}
						else
						{
							con->set_body(json_data_make(false, "", "[CancelOrder]Failed!Initialize failed!"));
						}
					}
					else
					{
						con->set_body(json_data_make(false, "", "[CancelOrder]Failed!Params invalid!"));
					}
				}
				break;
				case si_apitype_trade_order_query://订单查询
				{
					std::map<std::string, std::string> odata = { { "login","" }, };
					if (common_ParseJsonData(odata, strActionParams))
					{
						MTAPIRES mtapiRes = MT_RET_OK_NONE;
						// -- - create array of orders
						IMTOrder* pIMTOrder = m_pTickInstance->getManagerApi()->OrderCreate();
						if (pIMTOrder != NULL)
						{
							mtapiRes = m_pTickInstance->getManagerApi()->OrderGet(std::stoll(odata.at("login")), pIMTOrder);
							if (mtapiRes == MT_RET_OK)
							{
								std::string str = "[OrderGet]";
								str.append("{\"" + odata.at("login") + "\":[");
								str.append("{");
								str.append("\"time_setup\":\"").append(std::to_string(pIMTOrder->TimeSetup())).append("\",");
								str.append("\"time_expiration\":\"").append(std::to_string(pIMTOrder->TimeExpiration())).append("\",");
								str.append("\"activation_time\":\"").append(std::to_string(pIMTOrder->ActivationTime())).append("\",");
								str.append("\"type_time\":\"").append(std::to_string(pIMTOrder->TypeTime())).append("\",");
								str.append("\"price_order\":\"").append(std::to_string(pIMTOrder->PriceOrder())).append("\",");
								str.append("\"price_current\":\"").append(std::to_string(pIMTOrder->PriceCurrent())).append("\",");
								str.append("\"price_trigger\":\"").append(std::to_string(pIMTOrder->PriceTrigger())).append("\",");
								str.append("\"price_sl\":\"").append(std::to_string(pIMTOrder->PriceSL())).append("\",");
								str.append("\"price_tp\":\"").append(std::to_string(pIMTOrder->PriceTP())).append("\",");
								str.append("\"activation_price\":\"").append(std::to_string(pIMTOrder->ActivationPrice())).append("\",");
								str.append("\"type\":\"").append(std::to_string(pIMTOrder->Type())).append("\",");
								str.append("\"state\":\"").append(std::to_string(pIMTOrder->State())).append("\",");
								str.append("\"symbol\":\"").append(boost::locale::conv::utf_to_utf<char>(pIMTOrder->Symbol()).c_str()).append("\",");
								str.append("\"volume_initial\":\"").append(std::to_string(SMTMath::VolumeToDouble(pIMTOrder->VolumeInitial()))).append("\",");
								str.append("\"volume_current\":\"").append(std::to_string(SMTMath::VolumeToDouble(pIMTOrder->VolumeCurrent()))).append("\"");
								str.append("}");								
								str.append("]}");
								con->set_body(str);
							}
							else
							{
								con->set_body(json_data_make(false, "", "[OrderGet][error=" + std::to_string(mtapiRes) + "]"));
							}
							//--- free mem
							m_pTickInstance->getManagerApi()->Free(pIMTOrder);
						}
						else
						{
							con->set_body(json_data_make(false, "", "[OrderGet][error=create array failed!]"));
						}
					}
					else
					{
						con->set_body(json_data_make(false, "", "[OrderGet]Action params invalid!"));
					}
				}
				break;
				case si_apitype_trade_position_query://持仓查询
				{
					std::map<std::string, std::string> odata = { { "login","" }, };
					if (common_ParseJsonData(odata, strActionParams))
					{
						MTAPIRES mtapiRes = MT_RET_OK_NONE;
						// -- - create array of position
						IMTPositionArray* pPositionArray = m_pTickInstance->getManagerApi()->PositionCreateArray();
						if (pPositionArray != NULL)
						{
							mtapiRes = m_pTickInstance->getManagerApi()->PositionGet(std::stoll(odata.at("login")), pPositionArray);
							if (mtapiRes == MT_RET_OK)
							{
								std::string str = "[PositionGet][count" + std::to_string(pPositionArray->Total()) + "]";
								str.append("{\"" + odata.at("login") + "\":[");
								for (UINT uIdx = 0; uIdx < pPositionArray->Total(); uIdx++)
								{
									IMTPosition * pIMTPosition = pPositionArray->Next(uIdx);
									if (uIdx)
									{
										str.append(",");
									}
									str.append("{");
									//str.append("\"time_setup\":\"").append(std::to_string(pIMTPosition->TimeSetup())).append("\",");
									//str.append("\"time_expiration\":\"").append(std::to_string(pIMTPosition->TimeExpiration())).append("\",");
									str.append("\"activation_time\":\"").append(std::to_string(pIMTPosition->ActivationTime())).append("\",");
									//str.append("\"type_time\":\"").append(std::to_string(pIMTPosition->TypeTime())).append("\",");
									str.append("\"price_current\":\"").append(std::to_string(pIMTPosition->PriceCurrent())).append("\",");
									str.append("\"price_current\":\"").append(std::to_string(pIMTPosition->PriceOpen())).append("\",");
									str.append("\"price_sl\":\"").append(std::to_string(pIMTPosition->PriceSL())).append("\",");
									str.append("\"price_tp\":\"").append(std::to_string(pIMTPosition->PriceTP())).append("\",");
									str.append("\"profit\":\"").append(std::to_string(pIMTPosition->Profit())).append("\",");
									str.append("\"action\":\"").append(std::to_string(pIMTPosition->Action())).append("\",");
									str.append("\"position\":\"").append(std::to_string(pIMTPosition->Position())).append("\",");
									str.append("\"rate_margin\":\"").append(std::to_string(pIMTPosition->RateMargin())).append("\",");
									str.append("\"rate_profit\":\"").append(std::to_string(pIMTPosition->RateProfit())).append("\",");
									str.append("\"time_create\":\"").append(std::to_string(pIMTPosition->TimeCreate())).append("\",");
									str.append("\"time_update\":\"").append(std::to_string(pIMTPosition->TimeUpdate())).append("\",");
									str.append("\"activation_price\":\"").append(std::to_string(pIMTPosition->ActivationPrice())).append("\",");
									//str.append("\"type\":\"").append(std::to_string(pIMTPosition->Type())).append("\",");
									//str.append("\"state\":\"").append(std::to_string(pIMTPosition->State())).append("\",");
									str.append("\"symbol\":\"").append(boost::locale::conv::utf_to_utf<char>(pIMTPosition->Symbol()).c_str()).append("\",");
									//str.append("\"volume_initial\":\"").append(std::to_string(SMTMath::VolumeToDouble(pIMTPosition->VolumeInitial()))).append("\",");
									str.append("\"volume\":\"").append(std::to_string(SMTMath::VolumeToDouble(pIMTPosition->Volume()))).append("\"");
									str.append("}");
									pIMTPosition->Clear();
								}
								str.append("]}");
								con->set_body(str);
							}
							else
							{
								con->set_body(json_data_make(false, "", "[PositionGet][error=" + std::to_string(mtapiRes) + "]"));
							}
							//--- free mem
							pPositionArray->Release();
						}
						else
						{
							con->set_body(json_data_make(false, "", "[PositionGet][error=create array failed!]"));
						}
					}
					else
					{
						con->set_body(json_data_make(false, "", "[PositionGet]Action params invalid!"));
					}
				}
				break;
				case si_apitype_trade_history_order_query://历史订单查询(暂不可用)
				{
					std::map<std::string, std::string> odata = { { "login","" },{ "from","" },{ "to","" }, };
					if (common_ParseJsonData(odata, strActionParams))
					{
						MTAPIRES mtapiRes = MT_RET_OK_NONE;
						// -- - create array of order
						IMTOrderArray* pIMTOrderArray = m_pTickInstance->getManagerApi()->OrderCreateArray();
						if (pIMTOrderArray != NULL)
						{
							//mtapiRes = g_manager->HistoryGet(std::stoll(odata.at("from")), std::stoll(odata.at("to")), std::stoll(odata.at("login")), pIMTOrderArray);
							if (mtapiRes == MT_RET_OK)
							{
								std::string str = "[Order-HistoryGet][count" + std::to_string(pIMTOrderArray->Total()) + "]";
								str.append("{\"" + odata.at("login") + "\":[");
								for (UINT uIdx = 0; uIdx < pIMTOrderArray->Total(); uIdx++)
								{
									IMTOrder * pIMTOrder = pIMTOrderArray->Next(uIdx);
									if (uIdx)
									{
										str.append(",");
									}
									str.append("{");
									str.append("\"time_setup\":\"").append(std::to_string(pIMTOrder->TimeSetup())).append("\",");
									str.append("\"time_expiration\":\"").append(std::to_string(pIMTOrder->TimeExpiration())).append("\",");
									str.append("\"activation_time\":\"").append(std::to_string(pIMTOrder->ActivationTime())).append("\",");
									str.append("\"type_time\":\"").append(std::to_string(pIMTOrder->TypeTime())).append("\",");
									str.append("\"price_order\":\"").append(std::to_string(pIMTOrder->PriceOrder())).append("\",");
									str.append("\"price_current\":\"").append(std::to_string(pIMTOrder->PriceCurrent())).append("\",");
									str.append("\"price_trigger\":\"").append(std::to_string(pIMTOrder->PriceTrigger())).append("\",");
									str.append("\"price_sl\":\"").append(std::to_string(pIMTOrder->PriceSL())).append("\",");
									str.append("\"price_tp\":\"").append(std::to_string(pIMTOrder->PriceTP())).append("\",");
									str.append("\"activation_price\":\"").append(std::to_string(pIMTOrder->ActivationPrice())).append("\",");
									str.append("\"type\":\"").append(std::to_string(pIMTOrder->Type())).append("\",");
									str.append("\"position_id\":\"").append(std::to_string(pIMTOrder->PositionID())).append("\",");
									str.append("\"state\":\"").append(std::to_string(pIMTOrder->State())).append("\",");
									str.append("\"symbol\":\"").append(boost::locale::conv::utf_to_utf<char>(pIMTOrder->Symbol()).c_str()).append("\",");
									str.append("\"volume_initial\":\"").append(std::to_string(SMTMath::VolumeToDouble(pIMTOrder->VolumeInitial()))).append("\",");
									str.append("\"volume_current\":\"").append(std::to_string(SMTMath::VolumeToDouble(pIMTOrder->VolumeCurrent()))).append("\"");
									str.append("}");
									pIMTOrder->Clear();
								}
								str.append("]}");
								con->set_body(str);
							}
							else
							{
								con->set_body(json_data_make(false, "", "[Order-HistoryGet][error=" + std::to_string(mtapiRes) + "]"));
							}
							//--- free mem
							pIMTOrderArray->Release();
						}
						else
						{
							con->set_body(json_data_make(false, "", "[Order-HistoryGet][error=create array failed!]"));
						}
					}
					else
					{
						con->set_body(json_data_make(false, "", "[Order-HistoryGet]Action params invalid!"));
					}
				}
				break;
				case si_apitype_trade_symbol_query://根据交易品种名称查询
				{
					std::map<std::string, std::string> odata = { { "symbol","" }, };
					if (common_ParseJsonData(odata, strActionParams))
					{
						MTAPIRES mtapiRes = MT_RET_OK_NONE;
						// -- - create array of order
						IMTConSymbol * pIMTConSymbol = m_pTickInstance->getManagerApi()->SymbolCreate();
						if (pIMTConSymbol != NULL)
						{
							mtapiRes = m_pTickInstance->getManagerApi()->SymbolGet(boost::locale::conv::utf_to_utf<wchar_t>(odata.at("symbol")).c_str(), pIMTConSymbol);
							if (mtapiRes == MT_RET_OK)
							{
								std::string str = "[SymbolGet][count" + boost::locale::conv::utf_to_utf<char>(pIMTConSymbol->Symbol()) + "]";
								str.append("{\"" + odata.at("symbol") + "\":[");
								str.append("{");
								str.append("\"calc_mode\":\"").append(std::to_string(pIMTConSymbol->CalcMode())).append("\",");
								str.append("\"chart_mode\":\"").append(std::to_string(pIMTConSymbol->ChartMode())).append("\",");
								str.append("\"time_start\":\"").append(std::to_string(pIMTConSymbol->TimeStart())).append("\",");
								str.append("\"time_expiration\":\"").append(std::to_string(pIMTConSymbol->TimeExpiration())).append("\",");
								str.append("\"accrued_interest\":\"").append(std::to_string(pIMTConSymbol->AccruedInterest())).append("\",");
								str.append("\"basis\":\"").append(boost::locale::conv::utf_to_utf<char>(pIMTConSymbol->Basis())).append("\",");
								//str.append("\"color\":\"").append(std::to_string(pIMTConSymbol->Color())).append("\",");
								//str.append("\"color_background\":\"").append(std::to_string(pIMTConSymbol->ColorBackground())).append("\",");
								str.append("\"currency_base\":\"").append(boost::locale::conv::utf_to_utf<char>(pIMTConSymbol->CurrencyBase())).append("\",");
								str.append("\"currency_base_digits\":\"").append(std::to_string(pIMTConSymbol->CurrencyBaseDigits())).append("\",");
								str.append("\"currency_margin\":\"").append(boost::locale::conv::utf_to_utf<char>(pIMTConSymbol->CurrencyMargin())).append("\",");
								str.append("\"currency_margin_digits\":\"").append(std::to_string(pIMTConSymbol->CurrencyMarginDigits())).append("\",");
								str.append("\"currency_profit\":\"").append(boost::locale::conv::utf_to_utf<char>(pIMTConSymbol->CurrencyProfit())).append("\",");
								str.append("\"currency_profit_digits\":\"").append(std::to_string(pIMTConSymbol->CurrencyProfitDigits()).c_str()).append("\",");
								str.append("\"digits\":\"").append(std::to_string(pIMTConSymbol->Digits())).append("\",");
								str.append("\"volume_min\":\"").append(std::to_string(SMTMath::VolumeToDouble(pIMTConSymbol->VolumeMin()))).append("\",");
								str.append("\"volume_max\":\"").append(std::to_string(SMTMath::VolumeToDouble(pIMTConSymbol->VolumeMax()))).append("\",");
								str.append("\"volume_limit\":\"").append(std::to_string(SMTMath::VolumeToDouble(pIMTConSymbol->VolumeLimit()))).append("\",");
								str.append("\"volume_step\":\"").append(std::to_string(SMTMath::VolumeToDouble(pIMTConSymbol->VolumeStep()))).append("\",");
								str.append("\"face_value\":\"").append(std::to_string(pIMTConSymbol->FaceValue())).append("\",");
								str.append("\"exec_mode\":\"").append(std::to_string(pIMTConSymbol->ExecMode())).append("\"");
								str.append("}");
								str.append("]}");
								con->set_body(str);
							}
							else
							{
								con->set_body(json_data_make(false, "", "[SymbolGet][error=" + std::to_string(mtapiRes) + "]"));
							}
							//--- free mem
							pIMTConSymbol->Release();
						}
						else
						{
							con->set_body(json_data_make(false, "", "[SymbolGet][error=create array failed!]"));
						}
					}
					else
					{
						con->set_body(json_data_make(false, "", "[SymbolGet]Action params invalid!"));
					}
				}
				break;
				case si_apitype_account_balance_query://根据登陆ID查询个人账户信息
				{
					std::map<std::string, std::string> odata = { { "account","" }, };
					if (common_ParseJsonData(odata, strActionParams))
					{
						MTAPIRES mtapiRes = MT_RET_OK_NONE;
						// -- - create IMTUser instance
						IMTUser * pIMTUser = m_pTickInstance->getManagerApi()->UserCreate();
						if (pIMTUser != NULL)
						{
							// -- - create IMTAccount instance
							IMTAccount * pIMTAccount = m_pTickInstance->getManagerApi()->UserCreateAccount();
							if (pIMTAccount != NULL)
							{
								mtapiRes = m_pTickInstance->getManagerApi()->UserAccountGet(std::stoll(odata.at("account")), pIMTAccount);
								if (mtapiRes == MT_RET_OK)
								{
								}
							}
							mtapiRes = m_pTickInstance->getManagerApi()->UserGet(std::stoll(odata.at("account")), pIMTUser);
							if (mtapiRes == MT_RET_OK)
							{
								std::string str = "";
								// "[UserAccountGet][count" + boost::locale::conv::utf_to_utf<char>(pIMTConSymbol->Symbol()) + "]";
								std::string strData = json_start();
								//账号名称
								json_append(strData, "account", std::to_string(pIMTUser->Login()));
								//账号信息
								json_append(strData, "accountName", boost::locale::conv::utf_to_utf<char>(pIMTUser->Name()));
								//结余
								json_append(strData, "balance", pIMTUser->Balance());
								//可用信贷金额
								json_append(strData, "credit", pIMTUser->Credit());
								//前一个交易日的账户余额(暂时为0)
								// https://support.metaquotes.net/zh/docs/mt5/platform/administration/admin_reports/daily_report
								// 每日报告生成必须在服务器上针对包含必要账户的组启用，以生成该类报告。
								json_append(strData, "previousBalance", pIMTUser->BalancePrevDay());

								//账户可用资金(=结余)
								json_append(strData, "cashAvailable", pIMTUser->Balance());
								//杠杆比例
								json_append(strData, "leverage", pIMTUser->Leverage());
								//净值
								json_append(strData, "equity", pIMTAccount->Equity());
								//账户浮动盈亏
								json_append(strData, "floatingProfit", pIMTAccount->Floating());
								//可用保证金
								json_append(strData, "marginAvailable", pIMTAccount->MarginFree());
								//已用保证金
								json_append(strData, "marginUsed", pIMTAccount->Margin());
								//保证金占用比例%
								json_append(strData, "marginUtilisation", pIMTAccount->MarginLevel());

								//入金币种(默认美元)
								json_append(strData, "depositCurrency", "", false);
								json_end(strData);
								con->set_body(json_data_make(true, strData));
							}
							else
							{
								con->set_body(json_data_make(false, "", "[UserAccountGet][error=" + std::to_string(mtapiRes) + "]"));
							}
							//--- free mem
							pIMTAccount->Release();
						}
						else
						{
							con->set_body(json_data_make(false, "", "[UserAccountGet][error=create array failed!]"));
						}
					}
					else
					{
						con->set_body(json_data_make(false, "", "[UserAccountGet]Action params invalid!"));
					}
				}
				break;
				case si_apitype_account_register://注册新用户(模拟组)
				{
					MTAPIRES res = MT_RET_OK_NONE;
					std::map<std::string, std::string> odata = { { "name","" },{ "master_password","" },{ "investor_password","" }, };
					if (common_ParseJsonData(odata, strActionParams))
					{
						//--- create user
						IMTUser *user = m_pTickInstance->getManagerApi()->UserCreate();
						if (user)
						{
							wprintf_s(L"User interface has been created\n");
							//--- create new user
							user->Clear();
							user->Name(boost::locale::conv::utf_to_utf<wchar_t>(odata.at("name")).c_str());
							user->Rights(IMTUser::USER_RIGHT_DEFAULT);
							user->Group(L"demo\\demoforex");
							user->Leverage(100);
							res = m_pTickInstance->getManagerApi()->UserAdd(user, 
								boost::locale::conv::utf_to_utf<wchar_t>(odata.at("master_password")).c_str(), 
								boost::locale::conv::utf_to_utf<wchar_t>(odata.at("investor_password")).c_str());
							if (res == MT_RET_OK)
							{
								std::string strData = json_start();
								json_append(strData, "account", user->Login());
								con->set_body(json_data_make(true, strData));
							}
							else
							{
								con->set_body(json_data_make(false, "", "[Register]Failure!" + std::to_string(res)));
							}
						}
						else
						{
							con->set_body(json_data_make(false, "", "[Register]UserCreate Failure!"));
						}
					}
					else
					{
						con->set_body(json_data_make(false, "", "[Register]Failure!Parameters invalid!"));
					}
				}
				break;
				case si_apitype_account_name_update://更新账户名称
				{
					MTAPIRES res = MT_RET_OK_NONE;
					std::map<std::string, std::string> odata = { { "name","" },{ "account","" }, };
					if (common_ParseJsonData(odata, strActionParams))
					{
						//--- create user
						IMTUser *user = m_pTickInstance->getManagerApi()->UserCreate();
						if (user)
						{
							res = m_pTickInstance->getManagerApi()->UserGet(std::stoull(odata.at("account")), user);
							if (res == MT_RET_OK)
							{
								//--- update user name
								//user->Login(std::stoull(odata.at("account")));
								user->Name(boost::locale::conv::utf_to_utf<wchar_t>(odata.at("name")).c_str());
								m_pTickInstance->getManagerApi()->UserUpdate(user);
								if (res == MT_RET_OK)
								{
									std::string strData = json_start();
									json_append(strData, "account", user->Login());
									json_append(strData, "name", boost::locale::conv::utf_to_utf<char>(user->Name()));
									con->set_body(json_data_make(true, strData));
								}
								else
								{
									con->set_body(json_data_make(false, "", "[UserUpdate]Failure!" + std::to_string(res)));
								}
							}
							else
							{
								con->set_body(json_data_make(false, "", "[UserUpdate]UserGet Failure!" + std::to_string(res)));
							}
						}
						else
						{
							con->set_body(json_data_make(false, "", "[UserUpdate]UserCreate Failure!"));
						}
					}
					else
					{
						con->set_body(json_data_make(false, "", "[UserUpdate]Failure!Parameters invalid!"));
					}
				}
				break;
				case si_apitype_account_password_check://账户密码检查(只检查Investor_Password)
				{
					MTAPIRES res = MT_RET_OK_NONE;
					std::map<std::string, std::string> odata = { { "account","" }, { "password","" }, };
					if (common_ParseJsonData(odata, strActionParams))
					{
						//--- create user
						IMTUser *user = m_pTickInstance->getManagerApi()->UserCreate();
						if (user)
						{
							res = m_pTickInstance->getManagerApi()->UserGet(std::stoull(odata.at("account")), user);
							if (res == MT_RET_OK)
							{
								res = m_pTickInstance->getManagerApi()->UserPasswordCheck(IMTUser::USER_PASS_INVESTOR, user->Login(), boost::locale::conv::utf_to_utf<wchar_t>(odata.at("password")).c_str());
								if (res == MT_RET_OK)
								{
									//检测通过
									std::string strData = json_start();
									json_append(strData, "account", user->Login());
									con->set_body(json_data_make(true, strData));
								}
								else
								{
									//检测不通过
									std::string strData = json_start();
									json_append(strData, "account", user->Login());
									con->set_body(json_data_make(false, strData, std::to_string(res)));
								}
							}
							else
							{
								con->set_body(json_data_make(false, "", "[UserPasswordCheck]UserGet Failure!" + std::to_string(res)));
							}
						}
						else
						{
							con->set_body(json_data_make(false, "", "[UserPasswordCheck]UserCreate Failure!"));
						}
					}
					else
					{
						con->set_body(json_data_make(false, "", "[UserPasswordCheck]Failure!Parameters invalid!"));
					}
				}
				break;
				case si_apitype_account_password_update://更新账户密码(只更改Investor_Password)
				{
					MTAPIRES res = MT_RET_OK_NONE;
					std::map<std::string, std::string> odata = { { "account","" },{ "password","" }, };
					if (common_ParseJsonData(odata, strActionParams))
					{
						//--- create user
						IMTUser *user = m_pTickInstance->getManagerApi()->UserCreate();
						if (user)
						{
							res = m_pTickInstance->getManagerApi()->UserGet(std::stoull(odata.at("account")), user);
							if (res == MT_RET_OK)
							{
								//--- update user password
								res = m_pTickInstance->getManagerApi()->UserPasswordChange(IMTUser::USER_PASS_INVESTOR, user->Login(), boost::locale::conv::utf_to_utf<wchar_t>(odata.at("password")).c_str());
								if (res == MT_RET_OK)
								{
									std::string strData = json_start();
									json_append(strData, "account", user->Login());
									con->set_body(json_data_make(true, strData));
								}
								else
								{
									con->set_body(json_data_make(false, "", "[UserPasswordChange]Failure!" + std::to_string(res)));
								}
							}
							else
							{
								con->set_body(json_data_make(false, "", "[UserPasswordChange]UserGet Failure!" + std::to_string(res)));
							}
						}
						else
						{
							con->set_body(json_data_make(false, "", "[UserPasswordChange]UserCreate Failure!"));
						}
					}
					else
					{
						con->set_body(json_data_make(false, "", "[UserPasswordChange]Failure!Parameters invalid!"));
					}
				}
				break;
				case si_apitype_account_balance_deposit://账户入金
				{
					MTAPIRES res = MT_RET_OK_NONE;
					std::map<std::string, std::string> odata = { { "account","" },{ "balance","" }, { "comment","" }, };
					if (common_ParseJsonData(odata, strActionParams))
					{
						MTAPIRES mtapiRes = MT_RET_OK_NONE;
						UINT64   deal_id = 0;
						if(odata.at("comment").length() <= 0)
						{
							odata.at("comment") = ("Deposit");
						}
						//--- deposit $100000.0
						res = m_pTickInstance->getManagerApi()->DealerBalance(std::stoll(odata.at("account")), std::stod(odata.at("balance")), IMTDeal::DEAL_BALANCE, boost::locale::conv::utf_to_utf<wchar_t>(odata.at("comment")).c_str(), deal_id);
						if (res == MT_RET_OK)
						{
							std::string strData = json_start();
							json_append(strData, "account", odata.at("account"));
							json_append(strData, "deal_id", deal_id);
							json_append(strData, "balance", odata.at("balance"));
							con->set_body(json_data_make(true, strData));
						}
						else
						{
							con->set_body(json_data_make(false, "", "[Deposit]Failure!" + std::to_string(res)));
						}
					}
					else
					{
						con->set_body(json_data_make(false, "", "[Deposit]Action params invalid!"));
					}
				}
				break;
				case si_apitype_account_balance_withdrawal://账户出金
				{
					MTAPIRES res = MT_RET_OK_NONE;
					std::map<std::string, std::string> odata = { { "account","" },{ "balance","" },{ "comment","" }, };
					if (common_ParseJsonData(odata, strActionParams))
					{
						MTAPIRES mtapiRes = MT_RET_OK_NONE;
						UINT64   deal_id = 0;
						if (odata.at("comment").length() <= 0)
						{
							odata.at("comment") = ("Withdrawal");
						}
						//--- withdrawal $100000.0
						res = m_pTickInstance->getManagerApi()->DealerBalance(std::stoll(odata.at("account")), -fabs(std::stod(odata.at("balance"))), IMTDeal::DEAL_BALANCE, boost::locale::conv::utf_to_utf<wchar_t>(odata.at("comment")).c_str(), deal_id);
						if (res == MT_RET_OK)
						{
							std::string strData = json_start();
							json_append(strData, "account", odata.at("account"));
							json_append(strData, "deal_id", deal_id);
							json_append(strData, "balance", odata.at("balance"));
							con->set_body(json_data_make(true, strData));
						}
						else
						{
							con->set_body(json_data_make(false, "", "[Withdrawal]Failure!" + std::to_string(res)));
						}
					}
					else
					{
						con->set_body(json_data_make(false, "", "[Withdrawal]Action params invalid!"));
					}
				}
				break;
				case si_apitype_mt5_kcharts:
				{
					con->set_body("mt5 kcharts!");
				}
				break;
				default:
					break;
				}

				con->set_status(websocketpp::http::status_code::ok);
			}
		}
	}

	void on_fail(connection_hdl hdl) {
		typename EndpointType::connection_ptr con = m_server.get_con_from_hdl(hdl);

		// Print as much information as possible
		m_server.get_elog().write(websocketpp::log::elevel::warn,
			"Fail handler: " + std::to_string(con->get_state()) + " " +
			std::to_string(con->get_local_close_code()) + " " + con->get_local_close_reason() + " " + std::to_string(con->get_remote_close_code()) + " " +
			con->get_remote_close_reason() + " " + std::to_string(con->get_ec().value()) + " - " + con->get_ec().message() + "\n");
	}

protected:
	void errorSubProxy(const std::string &)
	{
		connectSubscriber();
	}

	void connectSubscriber()
	{
		std::cerr << "connectSubscriber\n";

		if (m_subscriber.state() == redisclient::RedisAsyncClient::State::Connected ||
			m_subscriber.state() == redisclient::RedisAsyncClient::State::Subscribed)
		{
			std::cerr << "disconnectSubscriber\n";
			m_subscriber.disconnect();
		}

		boost::asio::ip::tcp::endpoint endpoint(m_address, m_port);
		m_subscriber.connect(endpoint, std::bind(&CMarketServer::onSubscriberConnected, this, std::placeholders::_1));
	}

	void onSubscriberConnected(boost::system::error_code ec)
	{
		if (ec)
		{
			std::cerr << "onSubscriberConnected: can't connect to redis: " << ec.message() << "\n";
			callLater(m_connectSubscriberTimer, &CMarketServer::connectSubscriber);
		}
		else
		{
			std::cerr << "onSubscriberConnected ok\n";
			m_subscriber.subscribe(m_channelName,
				std::bind(&CMarketServer::onSubScriberMessage, this, std::placeholders::_1));
			m_subscriber.psubscribe(m_channelName,//"*",
				std::bind(&CMarketServer::onSubScriberMessage, this, std::placeholders::_1));
		}
	}
	
	void onSubScriberMessage(const std::vector<char> &buf)
	{
		std::string r("");
		std::string s(buf.begin(), buf.end());
		
		boost::lock_guard<boost::mutex> guard(m_action_quote_lock);
		if (s.find("AUDJPY") != std::string::npos || s.find("XAGUSD") != std::string::npos || s.find("XAUUSD") != std::string::npos)
		{
			std::cout << "onMessage: " << s << "\n";
			m_actions_realtimequote.push_front(ServerAction<typename EndpointType>(MESSAGE, s));
			//m_actions_realtimequote.push_front(ServerAction<typename EndpointType>(MESSAGE, boost::locale::conv::between(s, "UTF-8", "GBK")));
			m_action_cond_realtimequote.notify_all();
		}
		//if (s.find("XAUUSD") != std::string::npos)
		{
			//std::cout << "onMessage: " << s << "\n";
			//std::map<std::string, std::string> osymbol = { { "symbol","" }, };
			/*if (common_ParseJsonData(osymbol, s))
			{
				if (m_ssmap.find(osymbol.at("symbol")) == m_ssmap.end())
				{
					m_ssmap.insert(std::map<std::string, std::string>::value_type(osymbol.at("symbol"), s));
				}
				else
				{
					m_ssmap.at(osymbol.at("symbol")).assign(s);
					//if (s.find("XAUUSD") != std::string::npos)
					{
						//std::cout << "onMessage: " << m_ssmap.at(osymbol.at("symbol")) << "\n";
					}
				}
				common_CreateJsonData(m_ssmap, r);
				m_actions_realtimequote.push_front(ServerAction<typename EndpointType>(MESSAGE, r));
				//m_actions_realtimequote.push_front(ServerAction<typename EndpointType>(MESSAGE, boost::locale::conv::between(s, "UTF-8", "GBK")));
				m_action_cond_realtimequote.notify_all();
			}*/
		}
	}

	void callLater(boost::asio::deadline_timer &timer,
		void(CMarketServer::*callback)())
	{
		std::cerr << "callLater\n";
		timer.expires_from_now(m_timeout);
		timer.async_wait([callback, this](const boost::system::error_code &ec) {
			if (!ec)
			{
				(this->*callback)();
			}
		});
	}

public:

	void process_messages() {
		
		while (1) {
			boost::unique_lock<boost::mutex> lock(m_action_lock);

			while (m_actions.empty()) {
				m_action_cond.wait(lock);
			}

			ServerAction<typename EndpointType> a = m_actions.front();
			m_actions.pop_front();

			lock.unlock();

			if (a.type == SUBSCRIBE) {
				boost::lock_guard<boost::mutex> guard(m_connection_lock);
				m_connections.insert(a.hdl);
			}
			else if (a.type == UNSUBSCRIBE) {
				boost::lock_guard<boost::mutex> guard(m_connection_lock);
				m_connections.erase(a.hdl);
			}
			else if (a.type == MESSAGE) {
				/*boost::lock_guard<boost::mutex> guard(m_connection_lock);

				std::set<connection_hdl, std::owner_less<connection_hdl>>::iterator it;
				for (it = m_connections.begin(); it != m_connections.end();) {
					try {
						//m_server.send(*it,a.msg);
						if (a.data.find("XAUUSD") != std::string::npos)
						{
							std::cout << "a.data: " << a.data << "\n";
						}
						m_server.send(*it, a.data, websocketpp::frame::opcode::text);
						it++;
					}
					catch (const std::exception & e) {
						std::cout << e.what() << std::endl;
						it = m_connections.erase(it);
					}
					boost::this_thread::sleep_for(boost::chrono::milliseconds(300));
				}*/
			}
			else {
				// undefined.
			}
		}
	}

	void process_realtimequote_messages() {

		while (1) {
			boost::unique_lock<boost::mutex> lock(m_action_quote_lock);

			while (m_actions_realtimequote.empty()) {
				m_action_cond_realtimequote.wait(lock);
			}
			ServerAction<typename EndpointType> a = m_actions_realtimequote.front();
			m_actions_realtimequote.pop_front();
			lock.unlock();

			boost::lock_guard<boost::mutex> guard(m_connection_lock);

			std::set<connection_hdl, std::owner_less<connection_hdl>>::iterator it;
			for (it = m_connections.begin(); it != m_connections.end();) {
				try {
					m_server.send(*it, a.data, websocketpp::frame::opcode::text);
					it++;
				}
				catch (const std::exception & e) {
					std::cout << e.what() << std::endl;
					it = m_connections.erase(it);
				}
				boost::this_thread::sleep_for(boost::chrono::milliseconds(300));
			}
		}
	}
private:
	boost::asio::io_service &m_ios;

	typename EndpointType m_server;
	std::set<connection_hdl, std::owner_less<connection_hdl>> m_connections;
	std::deque<ServerAction<typename EndpointType>> m_actions;
	std::map<std::string, std::string> m_ssmap;
	std::deque<ServerAction<typename EndpointType>> m_actions_realtimequote;

	boost::mutex m_action_lock;
	boost::mutex m_action_quote_lock;
	boost::mutex m_connection_lock;
	boost::condition_variable m_action_cond;
	boost::condition_variable m_action_cond_realtimequote;

	const boost::posix_time::milliseconds m_timeout;
	const std::string m_channelName;
	const boost::asio::ip::address m_address;
	const unsigned short m_port;
	boost::asio::deadline_timer m_connectSubscriberTimer;
	redisclient::RedisAsyncClient m_subscriber;

	CMTTickInstance * m_pTickInstance;
};


__inline static int start_main_multithread(CMTTickInstance * pTickInstance) {
	try {
		// set up an external io_service to run both endpoints on. This is not
		// strictly necessary, but simplifies thread management a bit.
		boost::asio::io_service ios;
		static const std::string channelName = "unique-redis-channel-name-realorder";
		static const boost::posix_time::milliseconds timeout(300);
		
		const char * host = getenv("REDIS_HOST");
		boost::asio::ip::address address = boost::asio::ip::address::from_string(host ? host : "127.0.0.1");
		//boost::asio::ip::address address = boost::asio::ip::address::from_string("127.0.0.1");
		const unsigned short port = 6379;

		CMarketServer<server_no_tls> server_no_tls_instance(ios, channelName, timeout, address, port, pTickInstance);
		CMarketServer<server_tls> server_tls_instance(ios, channelName, timeout, address, port, pTickInstance);

		// Start a thread to run the processing loop
		boost::thread t_notls(bind(&CMarketServer<server_no_tls>::process_messages, &server_no_tls_instance));

		// Start a thread to run the processing loop
		boost::thread t_notls_realtimequote(bind(&CMarketServer<server_no_tls>::process_realtimequote_messages, &server_no_tls_instance));
		// Start a thread to run the processing loop
		boost::thread t_tls(bind(&CMarketServer<server_tls>::process_messages, &server_tls_instance));
		boost::thread t_tls_realtimequote(bind(&CMarketServer<server_tls>::process_realtimequote_messages, &server_tls_instance));

		// Run the asio loop with the main thread
		server_no_tls_instance.start_accept(9003);

		// Run the asio loop with the main thread
		server_tls_instance.start_accept(9002);

		t_notls.detach();
		t_notls_realtimequote.detach();

		t_tls.detach();
		t_tls_realtimequote.detach();

		ios.run();
	}
	catch (websocketpp::exception const & e) {
		std::cout << e.what() << std::endl;
	}
	return 0;
}
__inline static int start_main_original(CMTTickInstance * pTickInstance) {
	try {
		// set up an external io_service to run both endpoints on. This is not
		// strictly necessary, but simplifies thread management a bit.
		boost::asio::io_service ios;
		static const std::string channelName = "unique-redis-channel-name-example";
		static const boost::posix_time::milliseconds timeout(1000);
		boost::asio::ip::address address = boost::asio::ip::address::from_string("127.0.0.1");
		const unsigned short port = 6379;

		CMarketServer<server_no_tls> server_no_tls_instance(ios, channelName, timeout, address, port, pTickInstance);
		CMarketServer<server_tls> server_tls_instance(ios, channelName, timeout, address, port, pTickInstance);

		// Run the asio loop with the main thread
		server_no_tls_instance.start_accept(9003);
		server_tls_instance.start_accept(9002);

		ios.run();
	}
	catch (websocketpp::exception const & e) {
		std::cout << e.what() << std::endl;
	}
	return 0;
}
__inline static int start_tick_order(CMTTickInstance * pTickInstance)
{
	{
		std::string strData = "";
		std::map<std::string, std::string> ssv = { { "aa", "{\"a\":\"aa\", \"b\":\"456\"}" },{ "bb", "{\"a\":\"bb\", \"b\":\"456\"}" }, };
		common_CreateJsonData(ssv, strData);
		strData = "";
	}
	try {
		// set up an external io_service to run both endpoints on. This is not
		// strictly necessary, but simplifies thread management a bit.
		boost::asio::io_service ios;
		static const std::string channelTickName = "unique-redis-channel-name-realtick";
		static const std::string channelOrderName = "unique-redis-channel-name-realorder";
		static const boost::posix_time::milliseconds timeout(300);

		const char * host = getenv("REDIS_HOST");
		boost::asio::ip::address address = boost::asio::ip::address::from_string(host ? host : "127.0.0.1");
		//boost::asio::ip::address address = boost::asio::ip::address::from_string("127.0.0.1");
		const unsigned short port = 6379;

		CMarketServer<server_no_tls> tick_server_no_tls_instance(ios, channelTickName, timeout, address, port, pTickInstance);

		// Start a thread to run the processing loop
		boost::thread t_tick_notls(bind(&CMarketServer<server_no_tls>::process_messages, &tick_server_no_tls_instance));

		// Start a thread to run the processing loop
		boost::thread t_tick_notls_realtimequote(bind(&CMarketServer<server_no_tls>::process_realtimequote_messages, &tick_server_no_tls_instance));
		
		CMarketServer<server_no_tls> order_server_no_tls_instance(ios, channelOrderName, timeout, address, port, pTickInstance);

		// Start a thread to run the processing loop
		boost::thread t_order_notls(bind(&CMarketServer<server_no_tls>::process_messages, &order_server_no_tls_instance));

		// Start a thread to run the processing loop
		boost::thread t_order_notls_realtimequote(bind(&CMarketServer<server_no_tls>::process_realtimequote_messages, &order_server_no_tls_instance));
		
		// Run the asio loop with the main thread
		tick_server_no_tls_instance.start_accept(9003);
		
		// Run the asio loop with the main thread
		order_server_no_tls_instance.start_accept(9001);

		t_order_notls.detach();
		t_order_notls_realtimequote.detach();
		
		t_tick_notls.detach();
		t_tick_notls_realtimequote.detach();

		ios.run();
	}
	catch (websocketpp::exception const & e) {
		std::cout << e.what() << std::endl;
	}
	return 0;
}