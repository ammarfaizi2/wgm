// SPDX-License-Identifier: GPL-2.0-only

#include <wgm/entry.hpp>
#include <wgm/ctx.hpp>

#include <stdexcept>
#include <cctype>

namespace wgm {

using json = nlohmann::json;

ctx::ctx(const char *cfg_file, const char *client_cfg_dir,
	 const char *wg_conn_dir):
	cfg_file_(cfg_file),
	client_cfg_dir_(client_cfg_dir),
	wg_conn_dir_(wg_conn_dir)
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

		std::string wg_cfg = s.gen_wg_config();

		printf("Writing Wireguard config file:\n%s\n", wg_cfg.c_str());
	}

	return 0;
}

} /* namespace wgm */
