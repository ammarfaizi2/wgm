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

	std::unordered_map<std::string, server> servers_;

	void load_all(void);
	void load_servers(void);
	void load_clients(void);

public:
	ctx(const char *cfg_file, const char *client_cfg_dir,
	    const char *wg_conn_dir);

	~ctx(void);

	int run(void);
};

} /* namespace wgm */

#endif /* WGM__CTX_HPP */
