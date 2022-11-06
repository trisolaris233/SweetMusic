#ifndef __LOCAL_TUNE_H__
#define __LOCAL_TUNE_H__

#include "Sstring.h"

namespace sweet {

	class LocalTune;

	class TuneMetaData {
	public:
		enum TagType {
			Title,
			Artist,
			Album,
			Comment,
			Year,
			Length
		};

		Sstring getTitle(const LocalTune& obj) const;
		Sstring getArtist(const LocalTune &obj) const;
		Sstring getAlbum(const LocalTune &obj) const;
		Sstring getComment(const LocalTune &obj) const;
		Sstring getYear(const LocalTune &obj) const;
		Sstring getLength(const LocalTune &obj) const;

		bool setTitle(LocalTune &obj, const Sstring &arg);
		bool setArtist(LocalTune &obj, const Sstring &arg);
		bool setAlbum(LocalTune &obj, const Sstring &arg);
		bool setComment(LocalTune &obj, const Sstring &arg);

	protected:
		Sstring getTag(TagType t, const LocalTune& obj) const;
		bool setTag(TagType t, LocalTune &obj, const Sstring &arg);

	};

	class LocalTune {
	public:
		LocalTune();
		LocalTune(const Sstring &path);
		~LocalTune() = default;

		const Sstring &path() const;
		void setPath(const Sstring &path);

		Sstring getTitle() const;
		Sstring getArtist() const;
		Sstring getAlbum() const;
		Sstring getComment() const;
		Sstring getYear() const;
		Sstring getLength() const;

		bool setTitle(const Sstring &arg);
		bool setArtist(const Sstring &arg);
		bool setAlbum(const Sstring &arg);
		bool setComment(const Sstring &arg);

		bool remove();
		bool isEmpty() const;

	private:
		Sstring _path;

	};

}

#endif