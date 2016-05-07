/* $Id$ */

/**
 * @file http/endpoints-api/game-data.cpp put a better description here
 */

#ifdef ENABLE_NETWORK

#include "../../../stdafx.h"
#include "../../../debug.h"

#include "../../../settings_type.h"
#include "../../../map_func.h"
#include "../../../date_func.h"
#include "../../../string_func.h"
#include "../../network_internal.h"
#include "../../network_func.h"
#include "../../network.h"
#include "../../../company_base.h"
#include "../../../newgrf_config.h"

#include "../../../3rdparty/mongoose/mongoose.h"
#include "../handler.h"
#include "../json.h"

#include "game-data.h"

void HandleEndpoint_API_GameData(struct mg_connection *nc, int ev, void *ev_data)
{
	NetworkHTTPHandler* handler = NetworkHTTPHandler::GetSingleton(nc);

	JSONWriter writer(HTTP_API_SUCCESS);
	writer.StartObject();
		writer.StartObject("server");
			writer.AddString("version", "");
			writer.AddString("host", "");
			writer.StartObject("ports");
				writer.AddLong("game", _settings_client.network.server_port);
				writer.AddLong("http", _settings_client.network.server_http_port);
				writer.AddLong("admin", _settings_client.network.server_admin_port);
			writer.EndObject();
			writer.AddBool("dedicated", _network_dedicated);
			writer.AddBool("password", !StrEmpty(_settings_client.network.server_password));
			writer.AddString("language", NetworkLanguageToString(_settings_client.network.server_lang));
			writer.AddLong("maximum_companies", _settings_client.network.max_companies);
			writer.AddLong("maximum_clients", _settings_client.network.max_clients);
			writer.AddLong("maximum_spectators", _settings_client.network.max_spectators);
		writer.EndObject();
		writer.StartObject("date");
			writer.AddLong("start", ConvertYMDToDate(_settings_game.game_creation.starting_year, 0, 1));
			writer.AddLong("current", _date);
		writer.EndObject();
		writer.StartArray("companies");
			for(int i = 0; i < Company::GetNumItems(); i++)
				writer.AddBool(true);
		writer.EndArray();
		writer.StartArray("clients");
			for(int i = 0; i < _network_game_info.clients_on; i++)
				writer.AddBool(true);
		writer.EndArray();
		writer.StartArray("spectators");
			for(int i = 0; i < NetworkSpectatorCount(); i++)
				writer.AddBool(true);
		writer.EndArray();
		writer.StartObject("map");
			writer.AddLong("width", MapSizeX());
			writer.AddLong("height", MapSizeY());
			switch (_settings_game.game_creation.landscape) {
				case LT_TEMPERATE: writer.AddString("set", "temperate"); break;
				case LT_ARCTIC:    writer.AddString("set", "arctic");    break;
				case LT_TROPIC:    writer.AddString("set", "tropical");  break;
				case LT_TOYLAND:   writer.AddString("set", "toyland");   break;
				default: writer.AddLong("set", _settings_game.game_creation.landscape); break;
			}
			writer.StartArray("newgrfs");
				const GRFConfig *c;
				for(c = _grfconfig; c != NULL; c = c->next) {
					if(!HasBit(c->flags, GCF_STATIC)) {
						writer.StartObject();
							char idbuf[256];
							seprintf(idbuf, lastof(idbuf), "%08X", BSWAP32(c->ident.grfid));
							writer.AddString("id", idbuf);

							char md5buf[sizeof(c->ident.md5sum) * 2 + 1];
							md5sumToString(md5buf, lastof(md5buf), c->ident.md5sum);
							writer.AddString("md5sum", md5buf);

							const char* name = c->GetName();
							if(name != NULL)
								writer.AddString("name", name);

							const char* url = c->GetURL();
							if(url != NULL)
								writer.AddString("url", url);

							const char* desc = c->GetDescription();
							if(desc != NULL && strcmp(desc, name) != 0)
								writer.AddString("description", desc);

							writer.AddLong("version", c->version);
						writer.EndObject();
					}
				}
			writer.EndArray();
		writer.EndObject();
	writer.EndObject();
	handler->Send(nc, writer.GetString());
}

const char* NetworkLanguageToString(int lang)
{
	switch(lang) {
		case NETLANG_ANY:        return "any";
		case NETLANG_ENGLISH:    return "en";
		case NETLANG_GERMAN:     return "de";
		case NETLANG_FRENCH:     return "fr";
		case NETLANG_BRAZILIAN:  return "pt-BR";
		case NETLANG_BULGARIAN:  return "bg";
		case NETLANG_CHINESE:    return "zh";
		case NETLANG_CZECH:      return "cs";
		case NETLANG_DANISH:     return "da";
		case NETLANG_DUTCH:      return "nl";
		case NETLANG_ESPERANTO:  return "eo";
		case NETLANG_FINNISH:    return "fi";
		case NETLANG_HUNGARIAN:  return "hu";
		case NETLANG_ICELANDIC:  return "is";
		case NETLANG_ITALIAN:    return "it";
		case NETLANG_JAPANESE:   return "ja";
		case NETLANG_KOREAN:     return "ko";
		case NETLANG_LITHUANIAN: return "lt";
		case NETLANG_NORWEGIAN:  return "no";
		case NETLANG_POLISH:     return "pl";
		case NETLANG_PORTUGUESE: return "pt";
		case NETLANG_ROMANIAN:   return "ro";
		case NETLANG_RUSSIAN:    return "ru";
		case NETLANG_SLOVAK:     return "sk";
		case NETLANG_SLOVENIAN:  return "sl";
		case NETLANG_SPANISH:    return "es";
		case NETLANG_SWEDISH:    return "sv";
		case NETLANG_TURKISH:    return "tr";
		case NETLANG_UKRAINIAN:  return "uk";
		case NETLANG_AFRIKAANS:  return "af";
		case NETLANG_CROATIAN:   return "hr";
		case NETLANG_CATALAN:    return "ca";
		case NETLANG_ESTONIAN:   return "et";
		case NETLANG_GALICIAN:   return "gl";
		case NETLANG_GREEK:      return "el";
		case NETLANG_LATVIAN:    return "lv";
	}

	DEBUG(net, 1, "[http] [warning] attempted to convert unknown NetworkLanguage to string.");
	DEBUG(net, 1, "[http] [warning] this typically means another language has been added without updating the switch.");
	return "other";
}

#endif /* ENABLE_NETWORK */
