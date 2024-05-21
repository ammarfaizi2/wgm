// SPDX-License-Identifier: GPL-2.0-only
#ifndef WGM__HELPERS_HPP
#define WGM__HELPERS_HPP

#include <string>
#include <cerrno>
#include <memory>
#include <sys/file.h>
#include <nlohmann/json.hpp>

#define str_err_cpp(ERR) std::string(strerror(ERR))

#include <wgm/helpers/file_handle.hpp>

typedef wgm::helpers::file_handle file_t;

#endif /* #ifndef WGM__HELPERS_HPP */
