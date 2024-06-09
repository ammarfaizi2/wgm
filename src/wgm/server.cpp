// SPDX-License-Identifier: GPL-2.0-only

#include <wgm/server.hpp>

namespace wgm {

server::server(const json &j)
{
	IPRelay_		= j["IPRelay"];
	LocationRelay_		= j["LocationRelay"];
	Location_		= j["Location"];
	Country_		= j["Country"];
	City_			= j["City"];
	LocalIP_		= j["LocalIP"];
	Socks5Port_		= j["Socks5Port"];
	WireguardPort_		= j["WireguardPort"];
	WireguardSubnet_ 	= j["WireguardSubnet"];
	MTU_			= j["MTU"];
	PrivateKey_		= j["PrivateKey"];
	PublicKey_		= j["PublicKey"];
	PresharedKey_		= j["PresharedKey"];
}

server::~server(void) = default;

void server::add_client(const client &c)
{
	clients_.emplace(c.WireguardID(), c);
}

std::string server::gen_wg_config(void)
{
	std::string ret = "";

	ret += "[Interface]\n";
	ret += "MTU = " + std::to_string(MTU_) + "\n";
	ret += "PrivateKey = " + PrivateKey_ + "\n";
	ret += "Table = off\n";
	ret += "Address = " + WireguardSubnet_ + "\n";
	ret += "ListenPort = " + std::to_string(WireguardPort_) + "\n";
	ret += "\n";

	for (const auto &i : clients_) {
		ret += "[Peer]\n";
		ret += "PresharedKey = " + PresharedKey_ + "\n";
		ret += "PublicKey = " + i.second.PublicKey() + "\n";
		ret += "AllowedIPs = " + i.second.LocalIP() + "/32\n";
		ret += "\n";
	}

	return ret;
}

} /* namespace wgm */
