#include <iostream>
#include "options.h"
#include "log.h"
#include "fifo.h"
#include "httpserver.h"


int responseTest(HttpRequest * request, HttpResponse * response) {
	Log::logger->log("CNXTCP", DEBUG) << "Entering test callback" <<endl;
	string body="This is a test";
	char *buffer = new char[body.length() + 1]; // or

	std::strcpy(buffer, body.c_str());
	response->setBody(buffer, body.length());
	response->send();
	delete[] buffer;

}



int main(int argc, char **argv) {
	Log::logger->setLevel(NOTICE);
	Options options(argv[0], "1.0.0", "Epoll Ping Pong https://github.com/marc-despland/httpd");
	try {
		options.add('d', "debug", "Start on debug mode", false, false);
		options.add('p', "port", "Port to listen to", true, true);
	} catch(ExistingOptionException &e ) {
	}
	try {
		options.parse(argc, argv);
		if (options.get('d')->isAssign()) Log::logger->setLevel(DEBUG);
		HttpServer * server=new HttpServer(options.get("port")->asInt(),20);
		server->add(HTTP_GET, "\\/marc\\/mde", responseTest);
		server->run();
		
	} catch (OptionsStopException &e) {
	} catch (UnknownOptionException &e) {
		cout << " Request unknown option"<<endl;
	}

}