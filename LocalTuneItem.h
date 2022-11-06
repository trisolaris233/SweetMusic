#ifndef __LOCAL_TUNE_ITEM_H__
#define __LOCAL_TUNE_ITEM_H__

#include <map>
#include <memory>
#include <functional>

#include "Sstring.h"
#include "LocalTune.h"

using namespace sweet;

namespace icemusic {

	class LocalTuneItem;
	class LocalUser;
	//class LocalUserDataBase;

	struct TuneProperty {
		TuneProperty(const LocalTuneItem &obj);

		Sstring title;
		Sstring artist;
		Sstring album;
		Sstring length;
		Sstring year;
	};

	class LocalTuneItem {
	public:
		LocalTuneItem(const Sstring &path);
		~LocalTuneItem() = default;

		const Sstring &path() const;
		unsigned int weight() const;
		void setPath(const Sstring &path);
		TuneProperty getProperty() const;

		bool remove();
		bool isEmpty() const;

		Sstring directory() const;

		unsigned int operator++(int);
		unsigned int operator++();

	private:
		LocalTune		_tune;
		unsigned int	_weight;

	};

	class TuneItemFactory {
	public:
		typedef std::hash<Sstring>::result_type KeyType;

		static TuneItemFactory *getInstance();
		LocalTuneItem *getTuneItem(const Sstring &path);

	private:
		std::map<
			KeyType,
			std::unique_ptr<LocalTuneItem>
		> _mapOfLocalMusic;

		static TuneItemFactory *_instance;

	protected:
		TuneItemFactory() = default;

	};

	typedef std::vector<LocalTuneItem*> TuneList;
	
	class LocalTuneList {
	public:
		enum {
			MaxListName = 50
		};
		typedef TuneList::size_type			sizeType;
		typedef TuneList::difference_type	difType;
		typedef TuneList::iterator			iterator;
		typedef TuneList::const_iterator	constIterator;
		
		static std::unique_ptr<LocalTuneList>
			createLocalTuneList(const Sstring &listname, TuneList &&list = {});

		// getter: 返回歌单名
		Sstring name() const;
		Sstring name();

		iterator begin();
		constIterator begin() const;

		iterator end();
		constIterator end() const;

		sizeType size() const { return _list.size(); }

		LocalTuneItem *operator[](difType index);

		// setter: 设置歌单名
		bool setName(const Sstring &newname);
		// 添加音乐
		bool add(LocalTuneItem *tune);
		// 移除音乐
		// index：	索引
		bool remove(difType index); 

		static bool isValidListName(const Sstring &str);

	//private:
		TuneList _list;
		Sstring  _name;

		friend class LocalUserDataBase;

	//protected:
		//LocalTuneList();
		LocalTuneList(const Sstring &str, TuneList &&list = {});

	};

}

#endif