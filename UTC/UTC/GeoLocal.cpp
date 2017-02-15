// UTC.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "GeoLocal.h"
#include <memory>
#include <windows.h>

using namespace std;

// Constant and static data intialization
string GeoLocal::urlData = "";
const string url = "https://maps.googleapis.com/maps/api/timezone/json?";
const string key = "&key=AIzaSyC7P6yhPn0Ub_4vQVnYa5rAxQFY_v3wqhY";

int main(int argc, char* argv[])
{
	int uiProgCount = 0;
	// Create unique instance of GeoLocal
	unique_ptr<GeoLocal> geoLoc = make_unique<GeoLocal>();

	// Validate Input data
	if (geoLoc->ValidateInputAndPrintUsage(argc, argv) == SUCCESS)
	{
		//Open file
		std::ifstream ifs(argv[1]);

		// Construct output file name
		string outputfile = string(argv[1]);
		size_t pos = outputfile.find(".csv");
		outputfile.replace(pos,11,"_output.csv");
		string strCSVRow("");
		if (ifs.is_open())
		{
			std::ofstream ofs(outputfile.c_str());
			// Read csv file row by row
			while (getline(ifs, strCSVRow))
			{
				switch (++uiProgCount)
				{
				case 1:
					cout<<"Parsing CSV.\r";
					break;
				case 2:
					cout << "Parsing CSV..\r";
					break;
				case 3:
					cout << "Parsing CSV...\r";
					break;
				default:
					cout << "Parsing CSV\r";
					uiProgCount = 0;
					break;
				}

				// Proces the line
				if (geoLoc->ProcessCSVRow(strCSVRow) == SUCCESS)
				{
					// Write constructed data to output file
					ofs << strCSVRow << "," << geoLoc->GetTimeZoneID() << "," << geoLoc->GetLocDateTime() << "\n";
				}
				strCSVRow.clear();
			}
			ofs.close();
			ifs.close();
			cout << "Parsing complete!!" << endl;
			cout << "The output file is " << outputfile << endl;
			cout << "Press any Key to Exit..." << endl;
		}
		else
		{
			ifs.close();
			std::cout << "\n Unable to open input csv, press any key to exit!!!" << std::endl;
			std::cin.get();
			exit(EXIT_FAILURE);
		}
	}
	cin.get();
	return 0;
}
GeoLocal::GeoLocal():llDstOffset(0)
					, llRawOffset(0)
					, localTimeStamp(0)
					, strLocDateTime("")
{
	// Intialize curl upon construction
	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &GeoLocal::writeCallback);
}

GeoLocal::~GeoLocal()
{
	// Cleanup curl
	curl_easy_cleanup(curl);
}

int GeoLocal::ValidateInputAndPrintUsage(int argc, char* argv[])
{
	int retVal = SUCCESS;
	if ((argc < 2) || (argc > 3)) 
	{
		cout << "\n Usage: " << argv[0] <<" <csv file path>" << endl;
		retVal = FAILURE;
	}
	
	return retVal;
}

int GeoLocal::ProcessCSVRow(std::string input)
{
	int retVal = FAILURE;
	CURLcode res;
	
	// Reset data
	strLatitude.clear();
	strLongitude.clear();
	strLocDateTime.clear();
	timestamp = 0;

	ParseTimeStamp(input);
	string param = url + "location=" + strLatitude + "," + strLongitude +"&timestamp=" + to_string(timestamp) + key;

	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, param.c_str());
		res = curl_easy_perform(curl);
		if (res == CURLE_OK)
		{
			retVal = ParseTimeZone();
		}
	}

	return retVal;
}

int GeoLocal::ParseTimeZone()
{
	int retVal = FAILURE;
	// Use jsonCPP to parse json data
	Json::Value root;
	Json::Reader reader;
	if (!urlData.empty())
	{
		if (!reader.parse(urlData.c_str(), root))
		{
			std::cout << "\n Unable to parse json data, press any key to exit!!!" << std::endl;
			std::cin.get();
			exit(EXIT_FAILURE);
		}

		// Retrive data
		llDstOffset = root.get("dstOffset", "Default").asInt();
		llRawOffset = root.get("rawOffset", "Default").asInt();
		strTimeZoneID = root.get("timeZoneId", "Default").asString();
		strTimeZoneName = root.get("timeZoneName", "Default").asString();

		// Calculate local time stamp
		localTimeStamp = timestamp + llDstOffset + llRawOffset;

		struct tm * timeinfo = nullptr;
		char buffer[80];
		timeinfo = localtime(&localTimeStamp);
		//Format time as follows 2013-07-10T14:52:49
		strftime(buffer, 80, "%FT%T", timeinfo);
		strLocDateTime = string(buffer);
		retVal = SUCCESS;
	}

	return retVal;
}

// callback method to handle curl response
size_t GeoLocal::writeCallback(char* buf, size_t size, size_t nmemb, void* up)
{ 
	urlData = string(buf);
	return size*nmemb;
}

// Simple lmbda method for tokenization
auto Tokenize = [] (string & input, vector<string> & token, const char sep)
{
	token.clear();
	for (size_t p = 0, q = 0; p != input.npos; p = q)
	{
		token.push_back(input.substr(p + (p != 0), (q = input.find(sep, p + 1)) - p - (p != 0)));
	}
};

void GeoLocal::ParseTimeStamp(string input)
{
	// Parse the CSV data
	//"2013-07-10 02:52:49,-44.490947,171.220966"
	size_t pos = input.find_first_of(" ");
	string date = input.substr(0, pos);
	input.erase(0,pos);
	pos = input.find_first_of(",");
	string timeStamp = input.substr(0, pos);
	input.erase(0, pos+1);
	pos = input.find_first_of(",");
	strLatitude = input.substr(0, pos);
	input.erase(0, pos+1);
	strLongitude = input.substr(0, pos);

	// Tokenize the parsed data
	std::vector<std::string> token;
	Tokenize(date,token, '-');
	int year(std::stoi(token[0])), month(std::stoi(token[1])), day(std::stoi(token[2]));
	Tokenize(timeStamp, token, ':');
	int hour(std::stoi(token[0])), mins(std::stoi(token[1])), secs(std::stoi(token[2]));
	
	// Calculate time stamp
	time_t rawtime;
	time(&rawtime);
	struct tm * tmobj = localtime(&rawtime);
	tmobj->tm_hour = hour;
	tmobj->tm_min = mins;
	tmobj->tm_sec = secs;
	tmobj->tm_mon = month-1;
	tmobj->tm_mday = day;
	tmobj->tm_year = year-1900;
	timestamp = mktime(tmobj);
}
