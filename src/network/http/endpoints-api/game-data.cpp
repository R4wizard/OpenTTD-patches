/* $Id$ */

/**
 * @file http/endpoints-api/game-data.cpp put a better description here
 */

#ifdef ENABLE_NETWORK

#include "../../../stdafx.h"
#include "../../../debug.h"

#include "../../../settings_type.h"
#include "../../../company_func.h"
#include "../../../company_base.h"
#include "../../../map_func.h"
#include "../../../date_func.h"
#include "../../../string_func.h"
#include "../../../strings_func.h"
#include "../../network_internal.h"
#include "../../network_admin.h"
#include "../../network_func.h"
#include "../../network_base.h"
#include "../../network.h"
#include "../../../error.h"
#include "../../../rev.h"
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
		/***** SERVER INFORMATION *****/
		writer.StartObject("Server");
			writer.AddString("Name", _settings_client.network.server_name);
			writer.AddString("Version", _openttd_revision);
			writer.StartObject("Ports");
				writer.AddLong("Game", _settings_client.network.server_port);
				writer.AddLong("HTTP", _settings_client.network.server_http_port);
				writer.AddLong("Admin", _settings_client.network.server_admin_port);
			writer.EndObject();
			writer.AddBool("IsDedicated", _network_dedicated);
			writer.AddBool("IsPassworded", !StrEmpty(_settings_client.network.server_password));
			writer.AddString("Language", NetworkLanguageToString(_settings_client.network.server_lang));
			writer.AddLong("MaximumCompanies", _settings_client.network.max_companies);
			writer.AddLong("MaximumClients", _settings_client.network.max_clients);
			writer.AddLong("MaximumSpectators", _settings_client.network.max_spectators);
		writer.EndObject();

		/***** DATE INFORMATION *****/
		writer.StartObject("Date");
			writer.AddLong("Start", ConvertYMDToDate(_settings_game.game_creation.starting_year, 0, 1));
			writer.AddLong("Current", _date);
		writer.EndObject();

		/***** MAP INFORMATION *****/
		writer.StartObject("Map");
			writer.AddString("Name", _network_game_info.map_name);
			writer.AddLong("Width", MapSizeX());
			writer.AddLong("Height", MapSizeY());
			switch (_settings_game.game_creation.landscape) {
				case LT_TEMPERATE: writer.AddString("Set", "Temperate"); break;
				case LT_ARCTIC:    writer.AddString("Set", "Arctic");    break;
				case LT_TROPIC:    writer.AddString("Set", "Tropical");  break;
				case LT_TOYLAND:   writer.AddString("Set", "Toyland");   break;
				default: writer.AddLong("Set", _settings_game.game_creation.landscape); break;
			}
		writer.EndObject();

		/***** NEWGRFS INFORMATION *****/
		writer.StartArray("NewGRFs");
			const GRFConfig *c;
			for(c = _grfconfig; c != NULL; c = c->next) {
				if(!HasBit(c->flags, GCF_STATIC)) {
					writer.StartObject();
						char idbuf[256];
						seprintf(idbuf, lastof(idbuf), "%08X", BSWAP32(c->ident.grfid));
						writer.AddString("ID", idbuf);

						char md5buf[sizeof(c->ident.md5sum) * 2 + 1];
						md5sumToString(md5buf, lastof(md5buf), c->ident.md5sum);
						writer.AddString("MD5Sum", md5buf);

						const char* name = c->GetName();
						if(name != NULL)
							writer.AddString("Name", name);

						const char* url = c->GetURL();
						if(url != NULL)
							writer.AddString("URL", url);

						const char* desc = c->GetDescription();
						if(desc != NULL && strcmp(desc, name) != 0)
							writer.AddString("Description", desc);

						writer.AddLong("Version", c->version);
					writer.EndObject();
				}
			}
		writer.EndArray();

		/***** COMAPNY INFORMATION *****/
		const Company *company;
		writer.StartArray("Companies");
			FOR_ALL_COMPANIES(company) {
				writer.StartObject();
					NetworkCompanyStats company_stats[MAX_COMPANIES];
					NetworkPopulateCompanyStats(company_stats);

					writer.AddLong("ID", company->index);

					char company_name[NETWORK_COMPANY_NAME_LENGTH];
					SetDParam(0, company->index);
					GetString(company_name, STR_COMPANY_NAME, lastof(company_name));
					writer.AddString("Name", company_name);

					char president_name[MAX_LENGTH_PRESIDENT_NAME_CHARS];
					SetDParam(0, company->index);
					GetString(president_name, STR_PRESIDENT_NAME, lastof(president_name));
					writer.AddString("President", president_name);

					writer.AddBool("IsAI", company->is_ai);
					writer.AddBool("IsPassworded", !StrEmpty(_network_company_states[company->index].password));

					char colour[512];
					GetString(colour, STR_COLOUR_DARK_BLUE + _company_colours[company->index], lastof(colour));
					writer.AddString("Colour", colour);

					writer.AddLong("InauguratedYear", company->inaugurated_year);
					writer.StartObject("Finances");
						char moneybuf[256];
						seprintf(moneybuf, lastof(moneybuf), OTTD_PRINTF64, (int64)company->money);
						writer.AddString("Money",  moneybuf);

						char loanbuf[256];
						seprintf(loanbuf, lastof(loanbuf), OTTD_PRINTF64, (int64)company->current_loan);
						writer.AddString("Loan", loanbuf);

						char valuebuf[256];
						seprintf(valuebuf, lastof(valuebuf), OTTD_PRINTF64, (int64)CalculateCompanyValue(company));
						writer.AddString("Value",  valuebuf);

						Money income = 0;
						if (_cur_year - 1 == company->inaugurated_year) {
							for (uint i = 0; i < lengthof(company->yearly_expenses[2]); i++)
								income -= company->yearly_expenses[2][i];
						} else {
							for (uint i = 0; i < lengthof(company->yearly_expenses[1]); i++)
								income -= company->yearly_expenses[1][i];
						}

						char incomebuf[256];
						seprintf(incomebuf, lastof(incomebuf), OTTD_PRINTF64, (int64)income);
						writer.AddString("Income",  incomebuf);
					writer.EndObject();

					writer.StartObject("Vehicles");
						writer.AddLong("Train",    company_stats->num_vehicle[NETWORK_VEH_TRAIN]);
						writer.AddLong("Lorry",    company_stats->num_station[NETWORK_VEH_LORRY]);
						writer.AddLong("Bus",      company_stats->num_station[NETWORK_VEH_BUS]);
						writer.AddLong("Aircraft", company_stats->num_vehicle[NETWORK_VEH_PLANE]);
						writer.AddLong("Ship",     company_stats->num_vehicle[NETWORK_VEH_SHIP]);
					writer.EndObject();

					writer.StartObject("Stations");
						writer.AddLong("Train",    company_stats->num_station[NETWORK_VEH_TRAIN]);
						writer.AddLong("Lorry",    company_stats->num_station[NETWORK_VEH_LORRY]);
						writer.AddLong("Bus",      company_stats->num_station[NETWORK_VEH_BUS]);
						writer.AddLong("Aircraft", company_stats->num_station[NETWORK_VEH_PLANE]);
						writer.AddLong("Ship",     company_stats->num_station[NETWORK_VEH_SHIP]);
					writer.EndObject();
				writer.EndObject();
			}
		writer.EndArray();

		/***** CLIENT INFORMATION *****/
		writer.StartArray("Clients");
			NetworkClientInfo *ci;
			FOR_ALL_CLIENT_INFOS(ci) {
				if(ci->client_playas != COMPANY_SPECTATOR) {
					writer.StartObject();
						writer.AddLong("ID", ci->client_id);
						writer.AddString("Name", ci->client_name);

						int company_id = ci->client_playas + (Company::IsValidID(ci->client_playas) ? 1 : 0);
						if(company_id != 255)
							writer.AddLong("Company", company_id);

						writer.AddBool("IsServer", (ci->client_id == CLIENT_ID_SERVER));
					writer.EndObject();
				}
			}
		writer.EndArray();

		/***** SPECTATOR INFORMATION *****/
		writer.StartArray("Spectators");
			FOR_ALL_CLIENT_INFOS(ci) {
				if (ci->client_playas == COMPANY_SPECTATOR && ci->client_id != CLIENT_ID_SERVER) {
					writer.StartObject();
						writer.AddLong("ID", ci->client_id);
						writer.AddString("Name", ci->client_name);
					writer.EndObject();
				}
			}
		writer.EndArray();
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
