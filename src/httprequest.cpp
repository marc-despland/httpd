#include "httprequest.h"


HttpRequest::HttpRequest() {
	this->cr1=false;
	this->cr2=false;
	this->lf1=false;
	this->lf2=false;
}

bool HttpRequest::finished(char * buffer,unsigned int length) {
	if (this->cr1) {
		if (this->lf1) {
			if (this->cr2) {
				if (this->lf2) {
					return true;
				} else {
					if (length==1 && buffer[0]=='\n') {
						this->lf2=true;
						return true;
					} else {
						return newEnd(buffer, length);
					}
				}
			} else {
				if (length==2 && buffer[0]=='\r' && buffer[1]=='\n') {
					this->lf2=true;
					this->cr2=true;
					return true;
				} else {
					if (length==1 && buffer[0]=='\r') {
						this->cr2=true;
						return false;
					} else {
						return newEnd(buffer, length);
					}
				}
			}
		} else {
			if (length==3 && buffer[0]=='\n' && buffer[1]=='\r' && buffer[3]=='\n') {
					this->lf1=true;
					this->lf2=true;
					this->cr2=true;
					return true;
				} else {
					if (length==2 && buffer[0]=='\n' && buffer[1]=='\r') {
						this->lf1=true;
						this->cr2=true;
						return false;
					} else {
						if (length==1 && buffer[0]=='\n') {
							this->lf1=true;
							return false;
						} else {
							return newEnd(buffer, length);
						}
					}
				}
			}
	} else {
		return newEnd(buffer, length);
	}
}

bool HttpRequest::newEnd(char * buffer,unsigned int length) {
	this->cr1=false;
	this->cr2=false;
	this->lf1=false;
	this->lf2=false;
	int i=length-1;	
	while (i>=0 && (buffer[i]=='\r' || buffer[i]=='\n')) i--;
	i++;
	if (i<length) {
		int size=length-i;
		switch (size) {
			case 1:
				if (buffer[i]=='\r') this->cr1=true;
			case 2:
				if (buffer[i]=='\r' && buffer[i+1]=='\n') {
					this->cr1=true;
					this->lf1=true;
				}
				break;
			case 3:
				if (buffer[i]=='\r' && buffer[i+1]=='\n' && buffer[i+2]=='\r') {
					this->cr1=true;
					this->lf1=true;
					this->cr2=true;
				}
				break;
			default:
				if (buffer[length-4]=='\r' && buffer[length-3]=='\n' && buffer[length-2]=='\r' && buffer[length-1]=='\n') {
					this->cr1=true;
					this->lf1=true;
					this->cr2=true;
					this->lf2=true;
				}
		}
	}
	return (this->cr1 && this->cr2 && this->lf1 && this->lf2);
}

void HttpRequest::init() {
	this->cr1=false;
	this->cr2=false;
	this->lf1=false;
	this->lf2=false;
}

std::string HttpRequest::request(std::string host, unsigned int port, std::string path) {
	return "GET / HTTP/1.1\r\nHost: "+host+":"+std::to_string(port)+"\r\nAccept: */*\r\nConnection: keep-alive\r\n\r\n";
}