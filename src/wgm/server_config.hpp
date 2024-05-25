// SPDX-License-Identifier: GPL-2.0-only
#ifndef WGM__SERVER_HPP
#define WGM__SERVER_HPP

#include <memory>
#include <string>
#include <unordered_map>

#include <wgm/helpers.hpp>

namespace wgm {

struct server_config {
	std::string location;
	std::string country;
	std::string city;
	std::string local_ip;
	uint16_t socks5_port;
	uint16_t wireguard_port;
	std::string private_key;
	std::string public_key;
	std::string preshared_key;
	std::string gateway_ip;

	using json = nlohmann::json;

	server_config(const std::string &json_str);
	server_config(const json &json_obj);
	~server_config(void);
};

typedef server_config server_config_t;

} /* namespace wgm */

#endif /* #ifndef WGM__SERVER_HPP */
