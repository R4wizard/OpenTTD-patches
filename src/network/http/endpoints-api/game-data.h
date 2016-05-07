/* $Id$ */

/**
 * @file http/endpoints-api/game-data.h put a better description here
 */

#ifdef ENABLE_NETWORK

void HandleEndpoint_API_GameData(struct mg_connection *nc, int ev, void *ev_data);
const char* NetworkLanguageToString(int lang);

#endif /* ENABLE_NETWORK */
