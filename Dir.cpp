#include <io.h>
#include "Dir.h"
#include <cctype>
#include <sstream>
#include <cstring>
#include <algorithm>

#include "DirIterator.h"

#ifdef WIN32
	#include <Windows.h>

#endif

namespace sweet {

	Dir::Dir(const Sstring &filePath) :
		_curPath(filePath)
		, _curFilters(Dir::All) {}

	Dir::Dir(const Sstring &&filePath) :
		_curPath(filePath)
		, _curFilters(Dir::All) {}

	Dir::Dir(const Dir &another) :
		_curPath(another.path())
		, _curFilters(Dir::All) {}


	Dir &Dir::operator=(const Dir &rhs) {
		_curPath = rhs.path();
		return *this;
	}

	Dir &Dir::operator=(Dir &&rhs) {
		*this = rhs;
		return *this;
	}

	void Dir::setPath(const Sstring &filePath) {
		_curPath = filePath;
	}

	Sstring Dir::path() const {
		return _curPath;
	}

	Sstring Dir::absolutePath() const {
		if (_curPath.empty())
			return Sstring();

#ifdef WIN32
		wchar_t buffer[MAX_PATH] = { 0 };
		auto	res = GetFullPathName(
			_curPath.c_str(),
			MAX_PATH,
			buffer,
			nullptr
		);
		return (res == 0) ?
			Sstring() : Sstring(buffer, buffer + res);
#else

#endif
	}

	Sstring Dir::dirName() const {
		if (_curPath.empty())
			return Sstring();
		
		auto firstPos = _curPath.find_last_of('\\');
		if (firstPos == Sstring::npos)
			return _curPath;

		auto secondPos = _curPath.find_last_of('\\', firstPos - 1);
		if (secondPos == Sstring::npos)
			return Sstring(
					_curPath.begin(),
					_curPath.begin() + firstPos
			);
		return Sstring(
			_curPath.begin() + secondPos + 1,
			_curPath.begin() + firstPos
		);
	}

	Sstring Dir::canonicalPath() const {
		if (_curPath.empty())
			return Sstring(); 

		Sstring invalidChars(L"/:*?");
		Sstring absPath = absolutePath();
		auto firstPos = absPath.find_last_of('\\');

		if (firstPos == Sstring::npos)
			return absPath;

		Sstring lastDir(
			absPath.begin() + firstPos + 1,
			absPath.end()
		);
		if (lastDir.empty())
			lastDir = dirName();
		if (lastDir.empty())
			return absolutePath();
		lastDir.erase(
			std::remove_if(
				lastDir.begin(),
				lastDir.end(),
				[&invalidChars](wchar_t c)->bool{
				return(invalidChars.find(c) != Sstring::npos);
			}), 
			lastDir.end()
		);
		return Sstring(
			absPath.begin(),
			absPath.begin() + firstPos + 1
		) + lastDir;
	}

	bool Dir::isRelative() const {
		return Dir::isRelativePath(_curPath);
	}

	bool Dir::isAbsolute() const {
		return Dir::isAbsolutePath(_curPath);
	}

	bool Dir::exists() const {
		return (!_curPath.empty()) && 
				(_waccess(_curPath.c_str(), 0) != -1);
	}

	void Dir::setFilter(Dir::FilterSpec filter) {
		_curFilters = filter;
	}

	//void Dir::setNameFilters(
	//	const std::initializer_list<Sstring> &nameFilters
	//	) {
	//	if (nameFilters.size() != 0)
	//		_curNameFilters.clear();
	//	copy(
	//		nameFilters.begin(),
	//		nameFilters.end(),
	//		_curNameFilters.begin()
	//	);
	//}

	void Dir::setNameFilters(const SstringArray &nameFilters) {
		_curNameFilters.clear();
		for(auto x : nameFilters) 
			_curNameFilters.push_back(x);
	}

	SstringArray Dir::entryList() {
		SstringArray tmp;
		getMatchFiles(&tmp);
		return tmp;
	}

	bool Dir::isRelativePath(const Sstring &filePath) {
		if (filePath.empty())
			return false;
		return true;
	}

	bool Dir::isAbsolutePath(const Sstring &filePath) {
		if (filePath.empty() || filePath.length() <= 3)
			return false;
		return std::isalpha(filePath[0]) && // ÅÌ·û
			(filePath[1] == ':') && // Ã°ºÅ
			(filePath[2] == '\\');	// Â·¾¶·ûºÅ
	}

	void Dir::getMatchFiles(SstringArray *res) {
		DirIterator itr(this->path(), _curNameFilters, _curFilters);
		while (itr.hasNext()) {
			res->push_back(itr.current());
			itr.next();
		}
	}

}