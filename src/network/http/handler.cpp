/* $Id$ */

/**
 * @file http/http.h Basic functions to receive and send http server requests
 */

#ifdef ENABLE_NETWORK

#include <sstream>

#include "../../stdafx.h"
#include "../../debug.h"
#include "../../settings_type.h"
#include "../../../3rdparty/mongoose/mongoose.h"
#include "handler.h"
#include "../../safeguards.h"

NetworkHTTPHandler *_http_server_handler = NULL;
struct mg_mgr *_http_server_mg_manager = NULL;

/** Initialize the whole web bit. */
void NetworkHTTPInitialize()
{
	/* If not closed, then do it. */
	if (_http_server_handler != NULL) NetworkHTTPClose();

	DEBUG(net, 1, "[http] initializing listeners");
	assert(_http_server_handler == NULL && _http_server_mg_manager == NULL);

	// server_http_port gives me 0 so I set 9000 manually for now.
	_http_server_handler = new NetworkHTTPHandler(9000); //_settings_client.network.server_http_port);
	_http_server_mg_manager = new mg_mgr;
	mg_mgr_init(_http_server_mg_manager, _http_server_handler);
}

void NetworkHTTPClose()
{
	if(_http_server_handler != NULL)
		_http_server_handler->Close();

	mg_mgr_free(_http_server_mg_manager); // I think this is good enough to omit the `delete`
	delete _http_server_handler;

	_http_server_mg_manager = NULL;
	_http_server_handler = NULL;
	DEBUG(net, 1, "[http] closed listeners");
}

/**
 * Create an http server but don't listen yet.
 * @param port the port to bind to.
 */
NetworkHTTPHandler::NetworkHTTPHandler(uint16 port)
{
	this->port = port;
}

bool NetworkHTTPHandler::Listen()
{
	std::stringstream ss;
	ss << ":" << port;
	DEBUG(net, 1, "[http] listening on port %s", ss.str().c_str());

	// Can we make the return conditional?
	mg_set_protocol_http_websocket(mg_bind(_http_server_mg_manager, ss.str().c_str(), NetworkHTTPHandler::HandleEvent_Callback));
	return true;
}

void NetworkHTTPHandler::Close()
{
	return;
}

void NetworkHTTPHandler::HandleEvent(mg_connection *nc, int ev, void *ev_data)
{
	mg_serve_http_opts s_http_server_opts;
	s_http_server_opts.document_root = "./http/";  // Serve current directory
	s_http_server_opts.enable_directory_listing = "no";
	s_http_server_opts.index_files = "index.lp,index.htm,index.html,index.js";

	if (ev == MG_EV_HTTP_REQUEST) {
		mg_serve_http(nc, static_cast<http_message*>(ev_data), s_http_server_opts);
	}
}

void NetworkHTTPHandler::HandleEvent_Callback(struct mg_connection *nc, int ev, void *ev_data)
{
	DEBUG(net, 1, "[http] received event");

	// What we should do is grab the NetworkHTTPHandler instance via nc::mgr::user_data pointer
	// Once we have the instance, we call it's non-static handler with the parameters again.
	NetworkHTTPHandler* instance = static_cast<NetworkHTTPHandler*>(nc->mgr->user_data);
	instance->HandleEvent(nc, ev, ev_data);
}

#endif /* ENABLE_NETWORK */
