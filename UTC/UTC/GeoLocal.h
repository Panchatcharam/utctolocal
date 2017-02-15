#pragma once
#include <iostream>
#include <time.h>
#include <string>
#include <curl/curl.h>
#include <vector>
#include <json/json.h>
#include <json/reader.h>
#include <json/writer.h>
#include <json/value.h>
#include <fstream>
#include <sstream>
#include <iomanip>

enum STATUS
{
	SUCCESS,
	FAILURE
};


class GeoLocal
{
public:
	GeoLocal();
	~GeoLocal();
	int ValidateInputAndPrintUsage(int argc, char* argv[]);
	int ProcessCSVRow(std::string input);
	void ParseTimeStamp(std::string input);
	static size_t writeCallback(char* buf, size_t size, size_t nmemb, void* up);
	int ParseTimeZone();
	std::string GetTimeZoneID() { return strTimeZoneID; }
	std::string GetLocDateTime() { return strLocDateTime; }
private:
	static std::string urlData;
	std::string strLatitude, strLongitude;
	std::string strTimeZoneID, strTimeZoneName;
	time_t timestamp;
	CURL *curl;
	long long llDstOffset;
	long long llRawOffset;
	time_t localTimeStamp;
	std::string strLocDateTime;
};