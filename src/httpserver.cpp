#include "httpserver.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>
#include <arpa/inet.h>
#include "log.h"
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>



HttpServer::HttpServer(int port, unsigned int size, bool keepalive):Runnable() {
	this->port=port;
	this->size=size;
	this->count=0;
	this->wait=false;
	this->keepalive=keepalive;
}


void HttpServer::waitBeforeClosing() {
	this->wait=true;
}

void HttpServer::makeSocketNonBlocking(int socket) throw(MakeSocketNonBlockingException){
	int flags, s;
	flags = fcntl (socket, F_GETFL, 0);
	if (flags == -1) {
     throw MakeSocketNonBlockingException("Can't set to F_GETFL");
    }

	flags |= O_NONBLOCK;
	s = fcntl (socket, F_SETFL, flags);
	if (s == -1) {
      throw MakeSocketNonBlockingException("Can't set to F_SETFL");
    }
}

void HttpServer::listen() throw(ConnectionListenException){
	struct sockaddr_in server;
	this->socketfd=::socket(AF_INET,SOCK_STREAM,0);

	bzero(&server,sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr=htonl(INADDR_ANY);
	server.sin_port=htons(this->port);
	if (::bind(this->socketfd,(struct sockaddr *)&server,sizeof(server))<0) {
		Log::logger->log("CNXTCP", DEBUG) << "Failed to bind the socket " << this->socketfd << " try to bind to " << this->port<<endl;
		throw ConnectionListenException("Can't bind socket");
	}
	try {
		HttpServer::makeSocketNonBlocking(this->socketfd);
	} catch(MakeSocketNonBlockingException &e) {
		Log::logger->log("CNXTCP", DEBUG) << "Failed to make listen socket non blocking " << this->socketfd << " bind to " << this->port<<endl;
		throw ConnectionListenException("Can't make socket non blocking");
	}
	if (::listen(this->socketfd,this->size)<0) {
		Log::logger->log("CNXTCP", DEBUG) << "Failed to listen on socket " << this->socketfd << " bind to " << this->port<<endl;
		throw ConnectionListenException("Can't listen on socket");
	}
	this->pool=new ConnectionPool();
	this->pool->add(this->socketfd);
	Log::logger->log("CNXTCP", NOTICE) << "Listening on port " << this->port <<endl;
}


void HttpServer::accept() throw(MakeSocketNonBlockingException) {
	Log::logger->log("CNXTCP", DEBUG) << "Wait for a new client" <<endl;
	int clientfd;
	struct sockaddr_in client;
	socklen_t clientsize;
	bzero(&client,sizeof(client));
	clientsize=sizeof(client);
	try {
		do {
			Log::logger->log("CNXTCP",DEBUG) << "ACCEPT LOOP START" <<endl;
			bzero(&client,sizeof(client));
			clientsize=sizeof(client);
			clientfd = ::accept(this->socketfd,(struct sockaddr *)&client,&clientsize);
			Log::logger->log("CNXTCP",DEBUG) << "accept " << clientfd << " " << errno<<endl;
			if (clientfd>=0) {
				HttpServer::makeSocketNonBlocking(clientfd);
				this->pool->add(clientfd);
				this->requests[clientfd].init();
				this->count++;
				Log::logger->log("CNXTCP", DEBUG) << "New Connection " << this->count <<endl;
			} else {
				Log::logger->log("CNXTCP",DEBUG) << "Accept socket: " << clientfd << " error : " << errno << " " << strerror(errno) <<endl;
			}
			Log::logger->log("CNXTCP",DEBUG) << "ACCEPT LOOP END " << clientfd << " " << errno<<endl;
		} while ((clientfd>0) || (errno==EINTR)) ;
		//((clientfd<0) && ((errno==EINTR) || (errno!=EAGAIN)));
	} catch(ConnectionPoolException &e) {
		Log::logger->log("CNXTCP", ERROR) << "A connection pool error occurs " <<endl;
	}
}



void HttpServer::run() {
	this->listen();
	this->Runnable::start();
	while (this->Runnable::running()) {
		vector<int> * ready=this->pool->poll();
		Log::logger->log("CNXTCP",DEBUG) << "poll size "<<  ready->size() <<endl;
		for (unsigned int i=0; i<ready->size(); i++ ) {
			if (ready->at(i)==this->socketfd) {
				Log::logger->log("CNXTCP",DEBUG) << "Socket is a new connection "<<  ready->at(i) <<endl;
				this->accept();
			} else {
				int readerror=0;
				do {
					readerror=0;
					ssize_t nbb;
	                char buf[512];
	                Log::logger->log("CNXTCP",DEBUG) << "Trying to read in socket "<< ready->at(i)<<endl;
	                nbb = read (ready->at(i),  buf, sizeof buf);
	               	readerror=errno;
	                /*if ((nbb<0) && (readerror==EBADF)) {
	                	this->requests.erase(ready->at(i));
	                	::close(ready->at(i));
	                	Log::logger->log("CNXTCP",DEBUG) << "Closing socket "<< ready->at(i)<<endl;
	                } */
	                if (nbb>0) {
	                	Log::logger->log("CNXTCP",DEBUG) << "Socket is a client one with something to read "<<  ready->at(i) << " => " <<nbb << " bytes"<<endl;
	                	if (this->requests[ready->at(i)].finished(buf,nbb)) {
	                		Log::logger->log("CNXTCP",DEBUG) << "Have read full request "<<  ready->at(i) <<endl;
	                		time_t rawtime;
	  						struct tm * timeinfo;
	  						char buffer [80]; 
	  						time (&rawtime);
	  						timeinfo = localtime (&rawtime);
	  						strftime (buffer,80,"Date: %a, %d %b %Y %X %Z",timeinfo);
	  						std::string response= "HTTP/1.1 200 OK\r\n"+ string(buffer) +"\r\nServer: fast\r\nContent-Type: application/json\r\nConnection: close\r\nContent-Length: 13\r\n\r\n{ 'event':0 }";
	  						if (this->keepalive) {
	  							response= "HTTP/1.1 200 OK\r\n"+ string(buffer) +"\r\nServer: fast\r\nContent-Type: application/json\r\nConnection: keep-alive\r\nContent-Length: 13\r\n\r\n{ 'event':0 }";
	  						}
	  						int ws=write(ready->at(i), response.c_str(), response.length());
	                		if (ws<0) {
	                			Log::logger->log("CNXTCP",ERROR) << "Can't write on socket: " << ready->at(i) <<endl;
	                		}
	                		if (ws != response.length()) Log::logger->log("CNXTCP",ERROR) << "We still have to write data " << ready->at(i) <<endl;
	                		//::shutdown(ready->at(i), SHUT_RD);
	                		
	                		if (this->wait) sleep(2);
	                		if (!this->keepalive) {
	                			Log::logger->log("CNXTCP",DEBUG) << "Closing socket "<<  ready->at(i) <<endl;
	                			::close(ready->at(i));
	                			this->requests[ready->at(i)].init();
	                		}
	                	}	
	                }
	                Log::logger->log("CNXTCP",DEBUG) << "Read loop "<< readerror<<endl;
	            } while (readerror==0);
			}
		}
	}

}