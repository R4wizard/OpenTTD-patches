/* $Id$ */

/**
 * @file http/endpoints-api/fail-data.cpp put a better description here
 */

#ifdef ENABLE_NETWORK

#include "../../../stdafx.h"
#include "../../../debug.h"
#include "../../../3rdparty/mongoose/mongoose.h"
#include "../handler.h"
#include "../json.h"
#include "../../../safeguards.h"

#include "fail-data.h"

void HandleEndpoint_API_FailData(struct mg_connection *nc, int ev, void *ev_data)
{
	NetworkHTTPHandler* handler = NetworkHTTPHandler::GetSingleton(nc);

	JSONWriter writer(HTTP_API_FAILURE);
	writer.StartObject();
	writer.EndObject();
	handler->Send(nc, writer.GetString());
}

#endif /* ENABLE_NETWORK */
