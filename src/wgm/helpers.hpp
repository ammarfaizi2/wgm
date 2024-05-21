// SPDX-License-Identifier: GPL-2.0-only
#ifndef WGM__HELPERS_HPP
#define WGM__HELPERS_HPP

#include <string>
#include <cerrno>
#include <memory>

#include <sys/file.h>

#include <nlohmann/json.hpp>

#define str_err_cpp(ERR) std::string(strerror(ERR))

namespace wgm {

class file_handle {
private:
	std::string path_;
	FILE *file_;

public:
	using json = nlohmann::json;

	file_handle(const std::string &path, bool create = false, int lock = 0);
	~file_handle(void);
	void lock(int lock);
	void unlock(void);
	std::string get_contents(void);
	size_t put_contents(const std::string &contents);

	json get_json(void);
	void put_json(const json &j);
};

typedef file_handle file_t;

} /* namespace wgm */

#endif /* #ifndef WGM__HELPERS_HPP */
