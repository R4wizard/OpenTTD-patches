/* $Id$ */

/**
 * @file http/http.h Basic functions to receive and send http server requests
 */

#ifdef ENABLE_NETWORK

#include "../../stdafx.h"
#include "../../debug.h"
#include "../../settings_type.h"
#include "handler.h"

#include "../../../3rdparty/mongoose/mongoose.h"

#include "../../safeguards.h"

NetworkHTTPHandler *_http_server_handler = NULL;  ///< HTTP server handler

/** Initialize the whole web bit. */
void NetworkHTTPInitialize()
{
	/* If not closed, then do it. */
	if (_http_server_handler != NULL) NetworkHTTPClose();

	DEBUG(net, 1, "[http] initializing listeners");
	assert(_http_server_handler == NULL);

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

/*static struct mg_serve_http_opts s_http_server_opts;
static void ev_handler(struct mg_connection *nc, int ev, void *p) {
	if (ev == MG_EV_HTTP_REQUEST) {
		mg_serve_http(nc, (struct http_message *) p, s_http_server_opts);
	}
}*/

/**
 * Create an http server but don't listen yet.
 * @param bind the addresses to bind to.
 */
NetworkHTTPHandler::NetworkHTTPHandler(uint16 port)
{
	this->port = port;
/*	const char *cport = (char*)port;
	struct mg_mgr mgr;

	mg_mgr_init(&mgr, NULL);
	mg_set_protocol_http_websocket(mg_bind(&mgr, cport, ev_handler));
	s_http_server_opts.document_root = "./http/";  // Serve current directory
    s_http_server_opts.enable_directory_listing = "no";


    for (;;) { mg_mgr_poll(&mgr, 1000); }
    mg_mgr_free(&mgr);*/
}

bool NetworkHTTPHandler::Listen()
{
	DEBUG(net, 1, "[http] listening on port %d", this->port);
	return true;
}

void NetworkHTTPHandler::Close()
{
	return;
}

#endif /* ENABLE_NETWORK */
