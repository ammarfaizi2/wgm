// SPDX-License-Identifier: GPL-2.0-only
#ifndef WGM__CLIENT_HPP
#define WGM__CLIENT_HPP

#include <memory>
#include <string>
#include <unordered_map>

#include <wgm/helpers.hpp>

namespace wgm {

struct client_config {
	std::string wireguard_id;
	std::string location_relay;
	std::string location_exit;
	std::string public_key;
	std::string local_ip;
	bool expired;

	using json = nlohmann::json;

	client_config(const std::string &json_str);
	client_config(const json &json_obj);
	~client_config(void);
};

typedef client_config client_config_t;

} /* namespace wgm */

#endif /* #ifndef WGM__CLIENT_HPP */
