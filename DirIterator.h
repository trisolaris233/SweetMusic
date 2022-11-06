#ifndef __DIR_ITERATOR_H__
#define __DIR_ITERATOR_H__

#include <io.h>
#include <stack>
#include <queue>

#include "Dir.h"
#include "Sstring.h"
#ifdef WIN32
#	include <Windows.h>
#endif

namespace sweet {

	class DirIterator {
	public:
		DirIterator(const Sstring &path, const SstringArray &nameFilters, Dir::FilterSpec filer = Dir::All);
		DirIterator(const Sstring &path, Dir::FilterSpec filter);
		~DirIterator();
		
		bool		has() const;
		bool		hasNext() const;
		bool		next();
		Sstring		current() const;
		std::size_t count() const;

		static bool isDotOrDotDot(const Sstring &name);

	private:
#ifdef WIN32
		std::stack<HANDLE>	_handleStk;
		WIN32_FIND_DATA		_fileSearchRes;
		WIN32_FIND_DATA		*_entry;
#else
#endif
		std::stack<Sstring>	_subDirectories;
		SstringArray		_nameFilters;
		Dir					_originalPath;
		bool				_firstMatch;
		Dir::FilterSpec		_filter;

		mutable std::vector<Dir>	_fileList;
		mutable std::vector<Dir>::difference_type
									_index;

	protected:
		void advance();
		inline bool atEnd() const { return _handleStk.empty(); }
		bool advanceHelper();
		bool isAcceptable() const;
		void pushSubDirectory(const Sstring &path);
		//bool pushDirectory(const Sstring &filename);
		void pushSearchResult(const Sstring &directory);
		bool matchNameFilter(const Sstring &directory);
		bool matchFilter(const Sstring &directory);

	};

}


#endif