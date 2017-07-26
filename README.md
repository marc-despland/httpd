# httpd

A simple HTTP server using (http_parser)[https://github.com/nodejs/http-parser] to parse the http request.
This server is designed to be embedded in other project to add REST API capabilities for example.

To create a server :

```
		HttpServer * server=new HttpServer(port,pool_size, manage_cors);
		server->add(HTTP_GET, "/marc/mde", responseTest);
		server->run();
```

**/marc/mde** is an (ECMAScript regex)[http://www.cplusplus.com/reference/regex/ECMAScript/]

**responseTest** is a callback matching 
```
typedef int (*http_callback) (HttpRequest * req, HttpResponse * res);
```

For example :
```
int HttpServer::response404Callback(HttpRequest * request, HttpResponse * response) {
	Log::logger->log("CNXTCP", DEBUG) << "Entering default callback" <<endl;
	response->setStatusCode(404);
	response->setStatusMessage("NOT FOUND");
	response->send();
	delete response;
	delete request;
}

```

You have to delete **response** and **request** after the **send** calls.
