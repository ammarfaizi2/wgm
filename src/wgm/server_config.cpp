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
	location	= j.at("location").get<std::string>();
	country		= j.at("country").get<std::string>();
	city		= j.at("city").get<std::string>();
	local_ip	= j.at("local_ip").get<std::string>();
	socks5_port	= j.at("socks5_port").get<uint16_t>();
	wireguard_port	= j.at("wireguard_port").get<uint16_t>();
	private_key	= j.at("private_key").get<std::string>();
	public_key	= j.at("public_key").get<std::string>();
	preshared_key	= j.at("preshared_key").get<std::string>();
	gateway_ip	= j.at("gateway_ip").get<std::string>();
}

server_config::~server_config(void) = default;

} /* namespace wgm */
