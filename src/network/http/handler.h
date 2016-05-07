/* $Id$ */

/**
 * @file web/web.h Basic functions to receive and send web server requests
 */

#ifndef NETWORK_HTTP_HANDLER_H
#define NETWORK_HTTP_HANDLER_H

#ifdef ENABLE_NETWORK

void NetworkHTTPInitialize();
void NetworkHTTPClose();
void NetworkHTTPTick();

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

	static NetworkHTTPHandler* GetSingleton(struct mg_connection *nc);
	static void HandleEvent(struct mg_connection *nc, int ev, void *ev_data);
	static void HandleEndpoint_API_GameData(struct mg_connection *nc, int ev, void *ev_data);
	static void HandleEndpoint_API_FailData(struct mg_connection *nc, int ev, void *ev_data);

	static char* ConcatString(const char* a, const char* b);

	void SendResponse(struct mg_connection *nc, const char* response);
	void HandleTick();
};

#endif /* ENABLE_NETWORK */

#endif /* NETWORK_HTTP_HANDLER_H */
