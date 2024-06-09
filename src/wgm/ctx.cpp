// SPDX-License-Identifier: GPL-2.0-only

#include <wgm/entry.hpp>
#include <wgm/ctx.hpp>

#include <stdexcept>

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

inline void ctx::load_servers(void)
{
	json j = load_json_from_file(cfg_file_.c_str());

	// 'j' must be an array.
	if (!j.is_array())
		throw std::runtime_error("Invalid JSON file: " + cfg_file_ +": expected an array");

	// Reset the servers_ map.
	servers_.clear();

	for (const auto &i : j) {
		server s(i);
		servers_.emplace(s.Location(), s);
	}
}

inline void ctx::load_clients(void)
{
	std::vector<std::string> files = scandir(client_cfg_dir_.c_str(), true);

	for (const std::string &f : files) {
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
	return 0;
}

} /* namespace wgm */
