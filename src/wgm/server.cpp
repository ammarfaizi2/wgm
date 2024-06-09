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

} /* namespace wgm */
