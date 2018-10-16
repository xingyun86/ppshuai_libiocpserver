#pragma once

#include <MacroDefine.h>

class CKlineData {
public:
	CKlineData(double _tms,  // 时间戳
		double _open,// 开仓价
		double _high,// 最高价
		double _low,// 最低价
		double _close,// 品仓价
		double _vol// 成交
	) 
	{
		this->tms = _tms;
		this->open = _open;
		this->high = _high;
		this->low = _low;
		this->close = _close;
		this->vol = _vol;
	}
public:
	double tms;  // 时间戳
	double open;// 开仓价
	double high;// 最高价
	double low;// 最低价
	double close;// 品仓价
	double vol;// 成交手数
};

class CBaseData {
public:
	bool ret;
	std::string msg;
	std::string data;
	CBaseData(bool _ret, std::string _msg)
	{
		this->ret = _ret;
		this->msg = _msg;
	}
public:
	std::string make_json(bool _ret, std::string _msg, std::string _data)
	{
		ptree pt;
		std::stringstream stream;
		try
		{
			pt.put("ret", _ret);
			pt.put("msg", _msg);
			pt.put("data", _data);
			write_json(stream, pt);
		}
		catch (ptree_error err)
		{
			err.what();
			return ("");
		}
		return stream.str();
	}
	std::string make_json_kline(std::vector<CKlineData> & kdv)
	{
		std::stringstream stream;
		ptree pt_root, pt_item, pt_data;;
		
		try
		{
			pt_root.put("ret", ret);
			pt_root.put("msg", msg);
			
			BOOST_FOREACH(auto v, kdv)
			{
				pt_item.clear();
				pt_item.put("tms", v.tms);
				pt_item.put("open", v.open);
				pt_item.put("high", v.high);
				pt_item.put("low", v.low);
				pt_item.put("close", v.close);
				pt_item.put("vol", v.vol);
				pt_data.push_back(std::make_pair("", pt_item));
			}
			pt_root.put_child("data", pt_data);
			write_json(stream, pt_root);
		}
		catch (ptree_error err)
		{
			err.what();
			return ("");
		}
		return stream.str();
	}
};

bool CreateJson(std::string &wstr)
{
	std::stringstream stream;
	try
	{
		ptree pt;
		pt.put("name", "jim");
		ptree info;
		info.put("weight", true);
		ptree phone, phone_item1, phone_item2;
		phone_item1.put("phone", 123);
		phone_item2.put("phone", "123");
		phone.push_back(std::make_pair("", phone_item1));
		phone.push_back(std::make_pair("", phone_item2));
		info.put_child("all_phone", phone);
		pt.push_back(std::make_pair("info", info));
		write_json(stream, pt);
	}
	catch (ptree_error pt)
	{
		pt.what();
		return false;
	}
	wstr = stream.str();
	return true;
}
bool ParseJson(std::string &str)
{
	try
	{
		ptree pt;
		std::stringstream stream(str);
		read_json(stream, pt);
		std::string wstrName = pt.get<std::string>("name");
		ptree info = pt.get_child("info");
		std::string weight = info.get<std::string>("weight");
		int w = 0;
		w = info.get<int>("weight");
		ptree phones = info.get_child("all_phone");
		std::vector<std::string> vcPhone;
		BOOST_FOREACH(ptree::value_type &v, phones)
		{
			vcPhone.push_back(v.second.get<std::string>("phone"));
		}
	}
	catch (ptree_error pt)
	{
		pt.what();
		return false;
	}
	return true;
}

std::string map2json(const std::map<std::string, std::string>& map) {
	ptree pt;
	for (auto& entry : map)
		pt.put(entry.first, entry.second);
	std::ostringstream buf;
	write_json(buf, pt, false);
	return buf.str();
}

int test_json()
{
	std::string str;
	CreateJson(str);
	ParseJson(str);
	return 0;
}