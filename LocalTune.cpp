#include "LocalTune.h"

#include <iomanip>
#include <sstream>

#include <tag.h>
#include <fileref.h>
#include <mpegfile.h>
#include <id3v2tag.h>
#include <tpropertymap.h>
#include <attachedpictureframe.h>

#pragma comment(lib, "lib\\tag.lib")

namespace sweet {

	Sstring TuneMetaData::getTitle(
				const LocalTune &obj
	) const {
		return getTag(Title, obj);
	}

	Sstring TuneMetaData::getArtist(
		const LocalTune &obj
	) const {
		return getTag(Artist, obj);
	}

	Sstring TuneMetaData::getAlbum(
		const LocalTune &obj
		) const {
		return getTag(Album, obj);
	}

	Sstring TuneMetaData::getComment(
		const LocalTune &obj
	) const {
		return getTag(Comment, obj);
	}

	Sstring TuneMetaData::getYear(
		const LocalTune &obj
		) const {
		return getTag(Year, obj);
	}

	Sstring TuneMetaData::getLength(
		const LocalTune &obj
		) const {
		return getTag(Length, obj);
	}

	bool TuneMetaData::setTitle(
		LocalTune &obj,
		const Sstring &arg
	) {
		return setTag(Title, obj, arg);
	}

	bool TuneMetaData::setArtist(
		LocalTune &obj,
		const Sstring &arg
		) {
		return setTag(Artist, obj, arg);
	}

	bool TuneMetaData::setAlbum(
		LocalTune &obj,
		const Sstring &arg
		) {
		return setTag(Album, obj, arg);
	}

	bool TuneMetaData::setComment(
		LocalTune &obj,
		const Sstring &arg
		) {
		return setTag(Comment, obj, arg);
	}

	Sstring TuneMetaData::getTag(
		TuneMetaData::TagType t,
		const LocalTune &obj
	) const {
		if (obj.isEmpty())
			return Sstring();

		TagLib::FileRef currentFile(obj.path().c_str());
		TagLib::Tag *tag = currentFile.tag();

		if (!currentFile.isNull() && currentFile.tag()) {
			TagLib::Tag *tag = currentFile.tag();
			switch (t) {
			case Title:
				return tag->title().toWString();
				break;

			case Artist:
				return tag->artist().toWString();
				break;

			case Album:
				return tag->album().toWString();
				break;

			case Comment:
				return tag->comment().toWString();
				break;

			case Year: {
				int year = tag->year();
				std::wostringstream oss;
				oss << year;
				return oss.str();
			}
				break;

			case Length: {
				int seconds = currentFile.audioProperties()->length() % 60;
				int minutes = (currentFile.audioProperties()->length() - seconds) / 60;
				std::wostringstream oss;
				oss << minutes << L":"  << seconds;
				return oss.str();
			}
				break;
			}
		}
		return Sstring();
	}

	bool TuneMetaData::setTag(
		TagType t, LocalTune &obj
		, const Sstring &arg
	) {
		if (obj.isEmpty())
			return false;

		TagLib::FileRef currentFile(obj.path().c_str());
		if (!currentFile.isNull() && currentFile.tag()) {
			TagLib::Tag *tag = currentFile.tag();
			switch (t) {
			case Title:
				tag->setTitle(arg);
				break;

			case Artist:
				tag->setArtist(arg);
				break;

			case Album:
				tag->setAlbum(arg);
				break;

			case Comment:
				tag->setComment(arg);
				break;
			}
			currentFile.save();
			return true;
		}
		return false;
	}

	LocalTune::LocalTune() :
		_path()
	{}

	LocalTune::LocalTune(const Sstring &path)  :
		_path(path) 
	{}

	inline const Sstring &LocalTune::path() const {
		return _path;
	}

	inline void LocalTune::setPath(const Sstring &path) {
		_path = path;
	}

	Sstring LocalTune::getTitle() const {
		return TuneMetaData().getTitle(*this);
	}

	Sstring LocalTune::getArtist() const {
		return TuneMetaData().getArtist(*this); 
	}

	Sstring LocalTune::getAlbum() const {
		return TuneMetaData().getAlbum(*this);
	}

	Sstring LocalTune::getComment() const {
		return TuneMetaData().getComment(*this);
	}

	Sstring LocalTune::getYear() const {
		return TuneMetaData().getYear(*this);
	}

	Sstring LocalTune::getLength() const {
		return TuneMetaData().getLength(*this);
	}

	bool LocalTune::setTitle(const Sstring &arg) {
		return TuneMetaData().setTitle(*this, arg);
	}

	bool LocalTune::setArtist(const Sstring &arg) {
		return TuneMetaData().setArtist(*this, arg);
	}

	bool LocalTune::setAlbum(const Sstring &arg) {
		return TuneMetaData().setAlbum(*this, arg);
	}

	bool LocalTune::setComment(const Sstring &arg) {
		return TuneMetaData().setComment(*this, arg);
	}

	bool LocalTune::remove() {
		bool res = (_wremove(_path.c_str()) == 0);
		if (res) 			// 如果成功删除文件
			_path	= L"";	// 路径置空
		return res;
	}

	bool LocalTune::isEmpty() const {
		return _path == L"";
	}


}