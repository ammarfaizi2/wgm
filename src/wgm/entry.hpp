// SPDX-License-Identifier: GPL-2.0-only
#ifndef WGM__ENTRY_HPP
#define WGM__ENTRY_HPP

#include <nlohmann/json.hpp>

namespace wgm {

std::string load_str_from_file(const char *file);
nlohmann::json load_json_from_file(const char *file);
std::vector<std::string> scandir(const char *dir, bool skip_dot_files = false);
void store_str_to_file(const char *file, const std::string &str);
void pr_warn(const char *fmt, ...);
void pr_debug(const char *fmt, ...);

} /* namespace wgm */

#endif /* WGM__ENTRY_HPP */
