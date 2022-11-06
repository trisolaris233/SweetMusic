#ifndef __FILE_INFO_H__
#define __FILE_INFO_H__

#include "Sstring.h"
#include "FileIOConfig.h"

namespace sweet {

	class Date;
	class Dir;

	class FileInfo {
	public:
		FileInfo() = default;
		FileInfo(const Sstring &file);
		FileInfo(const FileInfo &file);
		~FileInfo() = default;
		FileInfo &operator=(FileInfo &&rhs);
		FileInfo &operator=(const FileInfo &rhs); 

		Dir absoluteDir() const;
		Sstring absoluteFilePath() const;
		Sstring absolutePath() const; 
		Sstring baseName() const;
		Sstring filename() const;
		Sstring filePath() const;
		Sstring extension() const;
		Sstring path() const;

		Dir  dir() const;
		bool exists() const;
		bool isAbsolute() const;
		bool isRelative() const;
		bool isDir() const;
		bool isFile() const;

		void setFile(const Sstring &file);
		void setFile(const FileInfo &file);

		bool operator==(const FileInfo &rhs) const;
		bool operator!=(const FileInfo &rhs) const;

		int64 size() const;
		Date  lastModified() const;
		Date  lastAccess() const;
		Date  createTime() const;

	private:
		Sstring _curPath;

	};

}

#endif