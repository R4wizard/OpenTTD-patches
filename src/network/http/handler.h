/* $Id$ */

/**
 * @file http/handler.h Basic functions to receive and send web server requests
 */

#ifndef NETWORK_HTTP_HANDLER_H
#define NETWORK_HTTP_HANDLER_H

#ifdef ENABLE_NETWORK

void NetworkHTTPInitialize();
void NetworkHTTPClose();
void NetworkHTTPTick();

#define HTTP_API_SUCCESS "HTTP/1.0 200 OK\r\nContent-Type: application/json\r\n\r\n"
#define HTTP_API_FAILURE "HTTP/1.0 400 Bad Request\r\nContent-Type: application/json\r\n\r\n"
#define HTTP_SUCCESS "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n"
#define HTTP_FAILURE "HTTP/1.0 400 Bad Request\r\nContent-Type: text/html\r\n\r\n"

/** Base handler for all web server requests */
class NetworkHTTPHandler {
protected:

	struct mg_connection *connection;
	struct mg_mgr *manager;
	uint16 port;

public:
	NetworkHTTPHandler(uint16 port);

	/** On destructing of this class, the socket needs to be closed */
	virtual ~NetworkHTTPHandler() { this->Close(); }

	bool Listen();
	void Close();
	void Send(struct mg_connection *nc, const char* response);
	void Tick();

	static NetworkHTTPHandler* GetSingleton(struct mg_connection *nc);
	static void HandleEvent(struct mg_connection *nc, int ev, void *ev_data);
};

#endif /* ENABLE_NETWORK */

#endif /* NETWORK_HTTP_HANDLER_H */
