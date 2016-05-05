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
};

#endif /* ENABLE_NETWORK */

#endif /* NETWORK_HTTP_HANDLER_H */
