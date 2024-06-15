// SPDX-License-Identifier: GPL-2.0-only

#include <wgm/entry.hpp>
#include <wgm/ctx.hpp>

#include <stdexcept>
#include <cstdlib>
#include <cctype>

namespace wgm {

using json = nlohmann::json;

ctx::ctx(std::string cfg_file, std::string client_cfg_dir,
	 std::string wg_conn_dir, std::string wg_dir,
	 std::string ipt_path, std::string ip2_path,
	 std::string true_path, std::string wg_quick_path):
	cfg_file_(cfg_file),
	client_cfg_dir_(client_cfg_dir),
	wg_conn_dir_(wg_conn_dir),
	wg_dir_(wg_dir),
	ipt_path_(ipt_path),
	ip2_path_(ip2_path),
	true_path_(true_path),
	wg_quick_path_(wg_quick_path)
{
	load_all();
}

ctx::~ctx(void) = default;

json ctx::get_wg_conn_by_local_interface_ip(const std::string &ip)
{
	std::vector<std::string> files = scandir(wg_conn_dir_.c_str(), true);

	for (const std::string &f : files) {
		try {
			json j = load_json_from_file(std::string(wg_conn_dir_ + "/" + f).c_str());

			if (j["local_interface_ip"] == ip)
				return j;

		} catch (const std::exception &e) {
			pr_warn("Failed to load Wireguard connection file: %s\n", e.what());
		}
	}

	throw std::runtime_error("No Wireguard connection file found for IP: " + ip);
}

inline void ctx::load_servers(void)
{
	json j = load_json_from_file(cfg_file_.c_str());

	// 'j' must be an array.
	if (!j.is_array())
		throw std::runtime_error("Invalid JSON file: " + cfg_file_ +": expected an array");

	// Reset the servers_ map.
	servers_.clear();

	for (const auto &i : j) {
		try {
			server s(i, this);
			servers_.emplace(s.Location(), s);
		} catch (const std::exception &e) {
			pr_warn("Failed to load server config: %s\n", e.what());
		}
	}
}

static inline bool is_str_hex(const std::string &s)
{
	for (const char &c : s) {
		if (!std::isxdigit(c))
			return false;
	}

	return true;
}

inline void ctx::load_clients(void)
{
	std::vector<std::string> files = scandir(client_cfg_dir_.c_str(), true);

	for (const std::string &f : files) {
		if (!is_str_hex(f))
			continue;

		try {
			std::string full_path = client_cfg_dir_ + "/" + f;
			json j = load_json_from_file(full_path.c_str());

			client c(j);
			if (c.Expired())
				continue;

			auto it = servers_.find(c.LocationExit());
			if (it == servers_.end())
				throw std::runtime_error("Invalid client config file: " + f + ": unknown exit location: " + c.LocationExit());				

			it->second.add_client(c);
			pr_debug("Loaded client config file: '%s'\n", f.c_str());
		} catch (const std::exception &e) {
			pr_warn("Failed to load client config file: '%s': %s\n", f.c_str(), e.what());
		}
	}
}

void ctx::load_all(void)
{
	load_servers();
	load_clients();
}

int ctx::run(void)
{

	for (auto &i : servers_) {
		server &s = i.second;

		if (s.num_clients() == 0) {
			pr_warn("No clients for server: %s\n", s.Location().c_str());
			continue;
		}

		std::string cfg_name = "wgm-" + s.Location();
		std::string cfg_path = wg_dir_ + "/" + cfg_name + ".conf";
		std::string new_cfg = s.gen_wg_config();
		std::string old_cfg = "";

		try {
			old_cfg = load_str_from_file(cfg_path.c_str());
		} catch (const std::exception &e) {
			old_cfg = "";
		}

		if (old_cfg == new_cfg) {
			pr_debug("No changes for server: %s\n", s.Location().c_str());
			continue;
		}

		try {
			int err = 0;

			store_str_to_file(cfg_path.c_str(), new_cfg);
			pr_debug("Updated server config: %s\n", s.Location().c_str());

			std::string dw_cmd = "/usr/bin/wg-quick down " + cfg_name;
			std::string up_cmd = "/usr/bin/wg-quick up " + cfg_name;

			err |= system(dw_cmd.c_str());
			err |= system(up_cmd.c_str());
			(void)err;
		} catch (const std::exception &e) {
			pr_warn("Failed to update server config: %s: %s\n", s.Location().c_str(), e.what());
		}
	}

	return 0;
}

} /* namespace wgm */
