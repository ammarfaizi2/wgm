// SPDX-License-Identifier: GPL-2.0-only
#ifndef WGM__CTX_HPP
#define WGM__CTX_HPP

#include <wgm/server.hpp>

#include <string>
#include <unordered_map>

namespace wgm {

class ctx {
private:
	std::string cfg_file_;
	std::string client_cfg_dir_;
	std::string wg_conn_dir_;
	std::string wg_dir_;

	std::unordered_map<std::string, server> servers_;

	void load_all(void);
	void load_servers(void);
	void load_clients(void);

public:
	using json = nlohmann::json;

	ctx(const char *cfg_file, const char *client_cfg_dir,
	    const char *wg_conn_dir, const char *wg_dir);

	~ctx(void);

	int run(void);

	const std::string &cfg_file(void) const { return cfg_file_; }
	const std::string &client_cfg_dir(void) const { return client_cfg_dir_; }
	const std::string &wg_conn_dir(void) const { return wg_conn_dir_; }

	json get_wg_conn_by_local_interface_ip(const std::string &ip);
};

} /* namespace wgm */

#endif /* WGM__CTX_HPP */
