#ifndef _HTTPREQUEST_H
#define _HTTPREQUEST_H
#include <string>

class HttpRequest {
public:
	HttpRequest();
	bool finished(char * buffer,unsigned int length);
	void init();
	static std::string request(std::string host, unsigned int port, std::string path);
protected:
	bool newEnd(char * buffer,unsigned int length);
	bool cr1;
	bool cr2;
	bool lf1;
	bool lf2;

};

#endif