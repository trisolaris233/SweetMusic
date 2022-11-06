#ifndef __DIR_H__
#define __DIR_H__

#include <string>
#include "Sstring.h"

//#include <initializer_list>


namespace sweet {

	// Dir
	// �ṩ����Ŀ¼��������ȡ��Ϣ�Ľӿ�
	class Dir {
	public:
		enum FilterSpec {
			Dirs		= 0x001,
			Files		= 0x002,
			All			= 0x004,
		};

		// ���ֿ�ζ���캯������ѡ��
		Dir() = default;
		Dir(const Sstring &filePath);
		Dir(const Sstring &&filePath);
		Dir(const Dir& another);
		~Dir() = default;

		Dir &operator=(const Dir&rhs);
		Dir &operator=(Dir &&rhs);
		
		void setPath(const Sstring &filePath);
		Sstring path() const;
		Sstring absolutePath() const;
		Sstring dirName() const;
		Sstring canonicalPath() const;

		bool isRelative() const;
		bool isAbsolute() const;
		bool exists() const;

		void setFilter(FilterSpec filter);
		//void setNameFilters(const std::initializer_list<Sstring> &filterList);
		void setNameFilters(const SstringArray &filterList);
		
		SstringArray entryList();

		static bool isRelativePath(const Sstring &filePath);
		static bool isAbsolutePath(const Sstring &filePath);

	private:
		Sstring			_curPath;
		FilterSpec		_curFilters;
		SstringArray	_curNameFilters;
		mutable SstringArray	_curFiles;

	protected:
		void getMatchFiles(SstringArray* res);

	};

}

#endif