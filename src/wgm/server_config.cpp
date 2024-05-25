// SPDX-License-Identifier: GPL-2.0-only

#include <wgm/server_config.hpp>

namespace wgm {

using json = nlohmann::json;

server_config::server_config(const std::string &json_str)
{
	server_config(json::parse(json_str));
}

server_config::server_config(const json &j)
{
	location	= j.at("Location").get<std::string>();
	country		= j.at("Country").get<std::string>();
	city		= j.at("City").get<std::string>();
	local_ip	= j.at("LocalIP").get<std::string>();
	socks5_port	= j.at("Socks5Port").get<uint16_t>();
	wireguard_port	= j.at("WireguardPort").get<uint16_t>();
	private_key	= j.at("PrivateKey").get<std::string>();
	public_key	= j.at("PublicKey").get<std::string>();
	preshared_key	= j.at("PresharedKey").get<std::string>();

	try {
		gateway_ip	= j.at("GatewayIp").get<std::string>();
	} catch (json::out_of_range &e) {
		gateway_ip = "";
	}
}

server_config::~server_config(void) = default;

} /* namespace wgm */
