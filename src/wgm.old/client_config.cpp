// SPDX-License-Identifier: GPL-2.0-only

#include <wgm/client_config.hpp>

namespace wgm {

using json = nlohmann::json;

client_config::client_config(const std::string &json_str)
{
	client_config(json::parse(json_str));
}

client_config::client_config(const json &j)
{
	wireguard_id	= j.at("WireguardID").get<std::string>();
	location_relay	= j.at("LocationRelay").get<std::string>();
	location_exit	= j.at("LocationExit").get<std::string>();
	public_key	= j.at("PublicKey").get<std::string>();
	local_ip	= j.at("LocalIP").get<std::string>();
	expired		= j.at("Expired").get<bool>();
}

client_config::~client_config(void) = default;

} /* namespace wgm */

