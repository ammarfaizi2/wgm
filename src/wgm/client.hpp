// SPDX-License-Identifier: GPL-2.0-only
#ifndef WGM__CLIENT_HPP
#define WGM__CLIENT_HPP

#include <string>
#include <cstdint>
#include <cstdbool>
#include <nlohmann/json.hpp>

namespace wgm {

class client {
private:
	std::string WireguardID_;
	std::string LocationRelay_;
	std::string LocationExit_;
	std::string PublicKey_;
	std::string LocalIP_;
	bool Expired_;

public:
	using json = nlohmann::json;

	client(const json &j);
	~client(void);

	inline const std::string &WireguardID(void) const { return WireguardID_; }
	inline const std::string &LocationRelay(void) const { return LocationRelay_; }
	inline const std::string &LocationExit(void) const { return LocationExit_; }
	inline bool Expired(void) const { return Expired_; }
};

} /* namespace wgm */

#endif /* WGM__CLIENT_HPP */
