// SPDX-License-Identifier: GPL-2.0-only
#ifndef WGM__SERVER_HPP
#define WGM__SERVER_HPP

#include <nlohmann/json.hpp>
#include <wgm/client.hpp>

#include <string>
#include <cstdint>
#include <unordered_map>

namespace wgm {

class ctx;

class server {
private:
	std::string IPRelay_;
	std::string LocationRelay_;
	std::string Location_;
	std::string Country_;
	std::string City_;
	std::string LocalIP_;
	uint16_t Socks5Port_;
	uint16_t WireguardPort_;
	std::string WireguardSubnet_;
	uint16_t MTU_;
	std::string PrivateKey_;
	std::string PublicKey_;
	std::string PresharedKey_;

	std::unordered_map<std::string, client> clients_;

	ctx *ctx_;
public:
	using json = nlohmann::json;

	server(const json &j, ctx *ctx);
	~server(void);
	void add_client(const client &c);
	std::string gen_wg_config(
		std::string ipt_path  = "/usr/sbin/iptables",
		std::string ip2_path  = "/usr/sbin/ip",
		std::string true_path = "/usr/bin/true"
	) const;

	inline size_t num_clients(void) const { return clients_.size(); }

	inline const std::string &IPRelay(void) const { return IPRelay_; }
	inline const std::string &LocationRelay(void) const { return LocationRelay_; }
	inline const std::string &Location(void) const { return Location_; }
};

} /* namespace wgm */

#endif /* WGM__SERVER_HPP */
