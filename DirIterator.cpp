#include <io.h>
#include <cctype>
#include <iostream>
#include "FileInfo.h"
#include "DirIterator.h"

namespace sweet {

	DirIterator::DirIterator(
					const Sstring &path,
					const SstringArray &nameFilters,
					Dir::FilterSpec filter
	) :
		_nameFilters(nameFilters)
		, _originalPath(path)
		, _filter(filter)
		, _index(0)	{
#ifdef WIN32
		// �������Ŀ¼
		pushSubDirectory(path);
		// ��������ֱ��������һ�����ϵ���Ŀ
		while ((_fileList.empty())
			&& hasNext()) {
			advance();
		}
		/*while (true) {
			if (atEnd())
				return;
			if (isAcceptable())
				return;
			if (advanceHelper())
				return;
		}*/
#else
#endif
	}

	DirIterator::DirIterator(const Sstring &path, Dir::FilterSpec filter)
		: _originalPath(path)
		, _filter(filter) {
#ifdef WIN32
		// �������Ŀ¼
		pushSubDirectory(path);
		// ��������ֱ��������һ�����ϵ���Ŀ
		while ((_fileList.empty())
			&& hasNext())
			advance();
#else
#endif
	}

	DirIterator::~DirIterator() {
#ifdef WIN32
		// ��δ����ľ���Ƴ�
		while (!_handleStk.empty()) {
			FindClose(_handleStk.top());
			_handleStk.top();
		}
#else
#endif
	}

	bool DirIterator::has() const {
		return(hasNext() || _index < _fileList.size());
	}

	bool DirIterator::hasNext() const {
#ifdef WIN32
		// �����к�̵�������
		// 1.����δ����ľ��
		// 2.���д�������Ŀ¼
		// 3.��ǰָ�벻Ϊ��
		if (_handleStk.empty()
			&& _subDirectories.empty()
			&& nullptr == _entry
			) {
			return false;
		}
		return true;
#else
#endif
	}

	bool DirIterator::next() {
		// ����ļ�Ŀ¼Ϊ�ջ����Ѿ����׵���
		// ���ܼ����������� �����֮
		while ((_fileList.empty()
			|| (_index + 1) >= (_fileList.size() - 1))
			&& hasNext()) {
			advance();
		}
		// ���_index�Ϸ�������
		if (_index + 1 <= (_fileList.size())) {
			++_index;
			return true;
		}
		return false;
	}

	// ���ص�ǰ��Ŀ¼���ļ�
	Sstring DirIterator::current() const {
		if (_index < _fileList.size())
			return _fileList[_index].path();
		return Sstring();
	}

	std::size_t DirIterator::count() const {
		return _index + 1;
	}

	// �ж��Ƿ�Ϊͬ������һ��Ŀ¼
	bool DirIterator::isDotOrDotDot(const Sstring &name) {
		if (name[0] == L'.' && name[1] == 0)
			return true;
		if (name[0] == L'.' && name[1] == L'.' && name[2] == 0)
			return true;
		return false;
	}

	// ��������
	void DirIterator::advance() {
		while (true) {
			if (advanceHelper())
				return;
			if (atEnd())
				return;
			if (isAcceptable())
				return;
		}
	}

	// ��Ҫ����������
	bool DirIterator::advanceHelper() {
#ifdef WIN32
		if (_handleStk.empty())
			return true;

		_entry = &_fileSearchRes;
		if (_firstMatch) {
			_firstMatch = false;
		}
		else {
			if (!FindNextFileW(_handleStk.top(), _entry))
				_entry = nullptr;
		}
		
		while (_entry && isDotOrDotDot(_entry->cFileName)) {
			if (!FindNextFileW(_handleStk.top(), _entry))
				_entry = nullptr;
		}

		if (!_entry) {
			_subDirectories.pop();
			auto handle = _handleStk.top();
			FindClose(handle);
			_handleStk.pop();
			return false;
		}

		auto sub = Dir(_subDirectories.top()).canonicalPath();
		sub += _entry->cFileName;

		pushSearchResult(sub);	// ����������
		if (_entry->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			pushSubDirectory(sub);
#else
#endif
		return false;
	}

	bool DirIterator::isAcceptable() const {
		return(_entry != nullptr);
	}

	void DirIterator::pushSubDirectory(const Sstring &path) {
#ifdef WIN32
		Dir ftmp(path);
		Sstring searchDir = ftmp.canonicalPath() + L"\\*";
		HANDLE	handle = FindFirstFileW(
			searchDir.c_str(),
			&_fileSearchRes
		);
		_firstMatch = true;
#else
#endif
		_handleStk.push(handle);
		_subDirectories.push(searchDir);
		_entry = nullptr;
	}

	//bool DirIterator::pushDirectory(const Sstring &filename) {
	//	FileInfo info(filename);
	//	//std::wcout << filename << "\textension = " << info.extension() << std::endl;
	//	if (_nameFilters.empty()) {
	//		_fileList.push_back(filename);
	//		return true;
	//	}
	//	if (_nameFilters.end() != std::find(
	//				_nameFilters.begin(),
	//				_nameFilters.end(),
	//				info.extension())
	//	) {
	//		_fileList.push_back(filename);
	//		return true;
	//	}
	//	return false;
	//}

	void DirIterator::pushSearchResult(const Sstring &directory) {
		if (matchFilter(directory) && matchNameFilter(directory))
			_fileList.push_back(directory);
	}

	bool DirIterator::matchNameFilter(const Sstring &directory) {
		if (_nameFilters.empty())	// ���û���ļ�������
			return true;
		FileInfo info(directory);	// ���ҹ����б�, �Ƿ����֮
		Sstring extO = info.extension();
		// ����׺��ת���ɴ�Сд
		std::transform(extO.begin(), extO.end(), extO.begin(), std::tolower);

		if (_nameFilters.end() != std::find(
			_nameFilters.begin(),
			_nameFilters.end(),
			extO)
			) 
			return true;
		return false;
	}

	bool DirIterator::matchFilter(const Sstring &directory) {
		if (_filter == Dir::All)
			return true;
		FileInfo info(directory);
		switch (_filter) {
		case Dir::Dirs:
			return info.isDir();
			break;
		case Dir::Files:
			return info.isFile();
			break;
		default:
			break;
		}
		return false;
	}


}