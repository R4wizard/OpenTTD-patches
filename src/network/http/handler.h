/* $Id$ */

/**
 * @file web/web.h Basic functions to receive and send web server requests
 */

#ifndef NETWORK_HTTP_HANDLER_H
#define NETWORK_HTTP_HANDLER_H

#ifdef ENABLE_NETWORK

void NetworkHTTPInitialize();
void NetworkHTTPClose();

/** Base handler for all web server requests */
class NetworkHTTPHandler {
protected:

	uint16 port;

public:
	NetworkHTTPHandler(uint16 port);

	/** On destructing of this class, the socket needs to be closed */
	virtual ~NetworkHTTPHandler() { this->Close(); }

	bool Listen();
	void Close();

	/** This handler will get our instance via mg_connection::mgr::user_data. As the docs say:
		https://docs.cesanta.com/mongoose/dev/#/c-api/net.h/mg_mgr_init/ For C++ example

		Then we pass it to the non-static copy
	**/
	static void HandleEvent_Callback(struct mg_connection *nc, int ev, void *ev_data);
	void HandleEvent(struct mg_connection *nc, int ev, void *ev_data);
};

#endif /* ENABLE_NETWORK */

#endif /* NETWORK_HTTP_HANDLER_H */
