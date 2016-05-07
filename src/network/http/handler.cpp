/* $Id$ */

/**
 * @file http/http.h Basic functions to receive and send http server requests
 */

#ifdef ENABLE_NETWORK

#include <sstream>

#include "../../stdafx.h"
#include "../../debug.h"
#include "../../settings_type.h"
#include "../../map_func.h"
#include "../../date_func.h"
#include "../../string_func.h"
#include "../network_func.h"
#include "../network.h"
#include "../../company_base.h"
#include "../../newgrf_config.h"
#include "../../../3rdparty/mongoose/mongoose.h"
#include "handler.h"
#include "json.h"
#include "../../safeguards.h"

#define HTTP_API_SUCCESS "HTTP/1.0 200 OK\r\nContent-Type: application/json\r\n\r\n"
#define HTTP_API_FAILURE "HTTP/1.0 400 Bad Request\r\nContent-Type: application/json\r\n\r\n"

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

	_http_server_handler->HandleTick();
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

	mg_register_http_endpoint(connection, "/api/game-data", NetworkHTTPHandler::HandleEndpoint_API_GameData);
	mg_register_http_endpoint(connection, "/api/fail-data", NetworkHTTPHandler::HandleEndpoint_API_FailData);

	mg_set_protocol_http_websocket(connection);
	return connection->err == 0;
}

void NetworkHTTPHandler::Close()
{
	mg_mgr_free(manager);
	return;
}

NetworkHTTPHandler* NetworkHTTPHandler::GetSingleton(struct mg_connection *nc)
{
	/** This handler will get our instance via mg_connection::mgr::user_data. As the docs say:
		https://docs.cesanta.com/mongoose/dev/#/c-api/net.h/mg_mgr_init/ For C++ example
	**/
	return static_cast<NetworkHTTPHandler*>(nc->mgr->user_data);
}

void NetworkHTTPHandler::HandleEndpoint_API_GameData(struct mg_connection *nc, int ev, void *ev_data)
{
	NetworkHTTPHandler* handler = NetworkHTTPHandler::GetSingleton(nc);

	JSONWriter writer(HTTP_API_SUCCESS);
	writer.StartObject();
		writer.StartArray("newgrfs");
			const GRFConfig *c;
			for(c = _grfconfig; c != NULL; c = c->next) {
				if(!HasBit(c->flags, GCF_STATIC)) {
					writer.StartObject();
						writer.AddLong("id", c->ident.grfid);
						writer.AddLong("md5", c->ident.grfid);
					writer.EndObject();
				}
			}
		writer.EndArray();
		writer.AddBool("dedicated", _network_dedicated);
		writer.AddBool("password", !StrEmpty(_settings_client.network.server_password));
		writer.AddLong("language", _settings_client.network.server_lang);
		writer.StartObject("date");
			writer.AddLong("start", ConvertYMDToDate(_settings_game.game_creation.starting_year, 0, 1));
			writer.AddLong("current", _date);
		writer.EndObject();
		writer.StartObject("companies");
			writer.AddLong("current", Company::GetNumItems());
			writer.AddLong("maximum", _settings_client.network.max_companies);
		writer.EndObject();
		writer.StartObject("clients");
			writer.AddLong("current", _network_game_info.clients_on);
			writer.AddLong("maximum", _settings_client.network.max_clients);
		writer.EndObject();
		writer.StartObject("spectators");
			writer.AddLong("current", NetworkSpectatorCount());
			writer.AddLong("maximum", _settings_client.network.max_spectators);
		writer.EndObject();
		writer.StartObject("map");
			writer.AddLong("width", MapSizeX());
			writer.AddLong("height", MapSizeY());
			writer.AddLong("set", _settings_game.game_creation.landscape);
		writer.EndObject();
	writer.EndObject();
	handler->SendResponse(nc, writer.GetString());
}

void NetworkHTTPHandler::HandleEndpoint_API_FailData(struct mg_connection *nc, int ev, void *ev_data)
{
	NetworkHTTPHandler* handler = NetworkHTTPHandler::GetSingleton(nc);

	JSONWriter writer(HTTP_API_FAILURE);
	writer.StartObject();
	writer.EndObject();
	handler->SendResponse(nc, writer.GetString());
}

void NetworkHTTPHandler::HandleEvent(struct mg_connection *nc, int ev, void *ev_data)
{
	NetworkHTTPHandler* handler = NetworkHTTPHandler::GetSingleton(nc);

	if (ev == MG_EV_HTTP_REQUEST) {
		DEBUG(net, 1, "[http] got unhandled request");
	}
}

void NetworkHTTPHandler::HandleTick()
{
	mg_mgr_poll(manager, 0);
}

void NetworkHTTPHandler::SendResponse(struct mg_connection *nc, const char* response)
{
	mg_printf(nc, response);
	nc->flags |= MG_F_SEND_AND_CLOSE;
}

#endif /* ENABLE_NETWORK */
