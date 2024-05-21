// SPDX-License-Identifier: GPL-2.0-only
#ifndef WGM__WGMD_HPP
#define WGM__WGMD_HPP

#include <nlohmann/json.hpp>

namespace wgm {

class wgmd_ctx {
private:
	std::string config_file_path_;
	std::string client_cfg_dir_;

	void load_config(void);

public:
	wgmd_ctx(const std::string &config_file_path,
		 const std::string &client_cfg_dir);

	~wgmd_ctx(void);
};

} /* namespace wgm */

#endif /* #ifndef WGM__WGMD_HPP */
