#include "Dir.h"
#include "Date.h"
#include "FileInfo.h"

#include <io.h>
#include <fstream>
#include <algorithm>

namespace sweet {

	FileInfo::FileInfo(const Sstring &file) :
		_curPath(file) {}

	FileInfo::FileInfo(const FileInfo &file) :
		_curPath(file.path()) {}

	FileInfo &FileInfo::operator=(FileInfo &&rhs) {
		*this = rhs;
		return *this;
	}

	FileInfo &FileInfo::operator=(const FileInfo &rhs) {
		_curPath = rhs.path();
		return *this;
	}

	Dir FileInfo::absoluteDir() const {
		Dir tmp(_curPath);
		return Dir(tmp.absolutePath());
	}

	Sstring FileInfo::absoluteFilePath() const {
		Dir tmp(_curPath);
		return tmp.absolutePath();
	}
	
	Sstring FileInfo::absolutePath() const {
		FileInfo tmp(absoluteFilePath());
		return tmp.filePath();
	}

	Sstring FileInfo::baseName() const {
		auto firstPos = _curPath.find_last_of('\\');
		auto secondPos = _curPath.find('.');
		if (firstPos == Sstring::npos && secondPos == Sstring::npos)
			return Sstring();
		if (firstPos == Sstring::npos)
			return Sstring(_curPath.begin(), _curPath.begin() + secondPos);
		return Sstring(_curPath.begin() + firstPos + 1, _curPath.end());
	}

	Sstring FileInfo::filename() const {
		if (!isFile())
			return Sstring();
		auto firstPos = _curPath.find_last_of('\\');
		if (firstPos == Sstring::npos)
			return _curPath;
		return Sstring(_curPath.begin() + firstPos + 1, _curPath.end());
	}

	Sstring FileInfo::filePath() const {
		auto firstPos = _curPath.find_last_of('\\');
		if (firstPos == Sstring::npos)
			return _curPath;
		return Sstring(_curPath.begin(), _curPath.begin() + firstPos);
	}

	Sstring FileInfo::extension() const {
		auto firstPos = _curPath.find_last_of('.');
		if (firstPos == Sstring::npos)
			return Sstring();
		return Sstring(_curPath.begin() + firstPos + 1, _curPath.end());
	}

	Sstring FileInfo::path() const {
		if (!isFile())
			return _curPath;
		auto firstPos = _curPath.find_last_of('\\');
		if (firstPos == Sstring::npos)
			return _curPath;
		return Sstring(_curPath.begin(), _curPath.begin() + firstPos);
	}

	Dir FileInfo::dir() const {
		return Dir(_curPath);
	}

	bool FileInfo::exists() const {
		return Dir(_curPath).exists();
	}

	bool FileInfo::isAbsolute() const {
		return Dir::isAbsolutePath(_curPath);
	}

	bool FileInfo::isRelative() const {
		return Dir::isRelativePath(_curPath);
	}

	bool FileInfo::isDir() const {
		return _curPath.find('.') == Sstring::npos;
	}

	bool FileInfo::isFile() const {
		return _curPath.find('.') != Sstring::npos;
	}

	void FileInfo::setFile(const Sstring &file) {
		_curPath = file;
	}

	void FileInfo::setFile(const FileInfo &file) {
		_curPath = file.path();
	}

	bool FileInfo::operator==(const FileInfo &rhs) const {
		return _curPath == rhs.path();
	}

	bool FileInfo::operator!=(const FileInfo &rhs) const {
		return _curPath != rhs.path();
	}

	int64 FileInfo::size() const {
		std::wifstream in(_curPath);
		if (!in.is_open()) return -1;

		in.seekg(0, std::ios_base::end);
		return static_cast<int64>(in.tellg());
	}

	Date FileInfo::lastModified() const {
		_wfinddata_t fileInfo;
		auto handle = _wfindfirst(_curPath.c_str(), &fileInfo);
		if (-1 == handle)
			return Date();
		return Date(fileInfo.time_write);
	}

	Date FileInfo::lastAccess() const {
		_wfinddata_t fileInfo;
		auto handle = _wfindfirst(_curPath.c_str(), &fileInfo);
		if (-1 == handle)
			return Date();
		return Date(fileInfo.time_access);
	}

	Date FileInfo::createTime() const {
		_wfinddata_t fileInfo;
		auto handle = _wfindfirst(_curPath.c_str(), &fileInfo);
		if (-1 == handle)
			return Date();
		return Date(fileInfo.time_create);
	}

}