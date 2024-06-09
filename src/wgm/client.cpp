// SPDX-License-Identifier: GPL-2.0-only

#include <wgm/client.hpp>

namespace wgm {

client::client(const json &j)
{
	try {
		WireguardID_		= j["WireguardID"];
		LocationRelay_		= j["LocationRelay"];
		LocationExit_		= j["LocationExit"];
		PublicKey_		= j["PublicKey"];
		LocalIP_		= j["LocalIP"];
		Expired_		= j["Expired"];
	} catch (const std::exception &e) {
		throw std::runtime_error("Invalid client JSON: " + std::string(e.what()));
	}
}

client::~client(void) = default;

} /* namespace wgm */
