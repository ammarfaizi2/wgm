// SPDX-License-Identifier: GPL-2.0-only

#include <cstdio>

#include "helpers.hpp"

using json = nlohmann::json;

namespace wgm {

file_handle::file_handle(const std::string &path, bool create, int lock):
	path_(path)
{
	file_ = fopen(path_.c_str(), "rb+");
	if (!file_ && create)
		file_ = fopen(path_.c_str(), "wb+");

	if (!file_)
		throw std::runtime_error("Failed to open file " + path_ + ": " + str_err_cpp(errno));

	if (lock)
		this->lock(lock);
}

file_handle::~file_handle(void)
{
	if (file_)
		fclose(file_);
}

void file_handle::lock(int lock)
{
	if (flock(fileno(file_), lock) == -1)
		throw std::runtime_error("Failed to lock file: " + str_err_cpp(errno));
}

void file_handle::unlock(void)
{
	if (flock(fileno(file_), LOCK_UN) == -1)
		throw std::runtime_error("Failed to unlock file: " + str_err_cpp(errno));
}

std::string file_handle::get_contents(void)
{
	long len;

	if (fseek(file_, 0, SEEK_END) == -1)
		throw std::runtime_error("Failed to seek to end of file: " + str_err_cpp(errno));

	len = ftell(file_);
	if (len == -1)
		throw std::runtime_error("Failed to get file position: " + str_err_cpp(errno));

	if (fseek(file_, 0, SEEK_SET) == -1)
		throw std::runtime_error("Failed to seek to start of file: " + str_err_cpp(errno));

	std::string ret(len, '\0');
	if (fread(&ret[0], 1, len, file_) != (size_t)len)
		throw std::runtime_error("Failed to read file: " + str_err_cpp(errno));

	return ret;
}

size_t file_handle::put_contents(const std::string &contents)
{
	size_t len;

	if (fseek(file_, 0, SEEK_SET) == -1)
		throw std::runtime_error("Failed to seek to start of file: " + str_err_cpp(errno));

	errno = 0;
	len = fwrite(contents.c_str(), 1, contents.size(), file_);
	if (len != contents.size())
		throw std::runtime_error("Failed to write file: " + str_err_cpp(errno));

	return len;
}

json file_handle::get_json(void)
{
	return json::parse(this->get_contents());
}

void file_handle::put_json(const json &j)
{
	this->put_contents(j.dump(2, ' ', false));
}

} /* namespace wgm */
