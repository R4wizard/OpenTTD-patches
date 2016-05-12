/* $Id$ */

/**
 * @file http/handler.h Basic functions to receive and send http server requests
 */

#ifdef ENABLE_NETWORK

#include <sstream>

#include "../../stdafx.h"
#include "../../debug.h"
#include "../../settings_type.h"
#include "../../../3rdparty/mongoose/mongoose.h"
#include "handler.h"
#include "json.h"
#include "../../safeguards.h"

#include "endpoints-api/fail-data.h"
#include "endpoints-api/game-data.h"

NetworkHTTPHandler *_http_server_handler = NULL;

/** Initialize the whole web bit. */
void NetworkHTTPInitialize()
{
	/* If not closed, then do it. */
	if (_http_server_handler != NULL) NetworkHTTPClose();

	DEBUG(net, 1, "[http] initializing listeners");
	assert(_http_server_handler == NULL);

	// server_http_port gives me 0 so I set 9000 manually for now.
	_http_server_handler = new NetworkHTTPHandler(_settings_client.network.server_http_port);
}

void NetworkHTTPClose()
{
	if(_http_server_handler != NULL)
		_http_server_handler->Close();

	delete _http_server_handler;

	_http_server_handler = NULL;
	DEBUG(net, 1, "[http] closed listeners");
}

void NetworkHTTPTick()
{
	if(_http_server_handler == NULL)
		return;

	_http_server_handler->Tick();
}

/**
 * Create an http server but don't listen yet.
 * @param port the port to bind to.
 */
NetworkHTTPHandler::NetworkHTTPHandler(uint16 port)
{
	this->port = port;

	manager = new mg_mgr;
	mg_mgr_init(manager, _http_server_handler);
}

bool NetworkHTTPHandler::Listen()
{
	std::stringstream ss;
	ss << ":" << port;
	DEBUG(net, 1, "[http] listening on port %s", ss.str().c_str());

	connection = mg_bind(manager, ss.str().c_str(), NetworkHTTPHandler::HandleEvent);

	mg_register_http_endpoint(connection, "/api/game-data", HandleEndpoint_API_GameData);
	mg_register_http_endpoint(connection, "/api/fail-data", HandleEndpoint_API_FailData);

	mg_set_protocol_http_websocket(connection);
	return connection->err == 0;
}

void NetworkHTTPHandler::Close()
{
	mg_mgr_free(manager);
}

void NetworkHTTPHandler::Tick()
{
	mg_mgr_poll(manager, 0);
}

void NetworkHTTPHandler::Send(struct mg_connection *nc, const char* response)
{
	DEBUG(net, 1, "%s", response);
	mg_printf(nc, response);
	nc->flags |= MG_F_SEND_AND_CLOSE;
}

void NetworkHTTPHandler::HandleEvent(struct mg_connection *nc, int ev, void *ev_data)
{
	NetworkHTTPHandler* handler = NetworkHTTPHandler::GetSingleton(nc);

	if (ev == MG_EV_HTTP_REQUEST) {
		DEBUG(net, 1, "[http] got unhandled request");
	}
}

NetworkHTTPHandler* NetworkHTTPHandler::GetSingleton(struct mg_connection *nc)
{
	/** This handler will get our instance via mg_connection::mgr::user_data. As the docs say:
		https://docs.cesanta.com/mongoose/dev/#/c-api/net.h/mg_mgr_init/ For C++ example
	**/
	return static_cast<NetworkHTTPHandler*>(nc->mgr->user_data);
}

#endif /* ENABLE_NETWORK */
