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
	std::string ipt_path_;
	std::string ip2_path_;
	std::string true_path_;
	std::string wg_quick_path_;

	std::unordered_map<std::string, server> servers_;

	void load_all(void);
	void load_servers(void);
	void load_clients(void);
	void make_config_and_bring_up_all(bool first_boot = false);
	void wg_quick_up(const std::string &name);
	void wg_quick_down(const std::string &name);
	void put_wg_config_file_and_up(const server &s, bool first_run = false);

public:
	using json = nlohmann::json;

	ctx(std::string cfg_file, std::string client_cfg_dir,
	    std::string wg_conn_dir, std::string wg_dir,
	    std::string ipt_path = "/usr/sbin/iptables",
	    std::string ip2_path = "/usr/sbin/ip",
	    std::string true_path = "/usr/bin/true",
	    std::string wg_quick_path = "/usr/bin/wg-quick"
	);

	~ctx(void);

	int run(void);

	const std::string &cfg_file(void) const { return cfg_file_; }
	const std::string &client_cfg_dir(void) const { return client_cfg_dir_; }
	const std::string &wg_conn_dir(void) const { return wg_conn_dir_; }

	json get_wg_conn_by_local_interface_ip(const std::string &ip);
};

} /* namespace wgm */

#endif /* WGM__CTX_HPP */
