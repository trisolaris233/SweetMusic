#include "FileInfo.h"
#include "LocalUser.h"
#include "LocalTune.h"
#include "LocalTuneItem.h"

namespace icemusic {

	TuneItemFactory* TuneItemFactory::_instance = nullptr;

	TuneProperty::TuneProperty(const LocalTuneItem &obj) {
		auto tune = LocalTune(obj.path());
		title = tune.getTitle();
		if (title.empty())
			title = FileInfo(obj.path()).filename();
		artist = tune.getArtist();
		album = tune.getAlbum();
		length = tune.getLength();
		year = tune.getYear();
	}

	LocalTuneItem::LocalTuneItem(const Sstring &path) :
		_tune(path)
		, _weight(0)
	{}

	inline const Sstring& LocalTuneItem::path() const {
		return _tune.path();
	}

	inline unsigned int LocalTuneItem::weight() const {
		return _weight;
	}

	TuneProperty LocalTuneItem::getProperty() const {
		return TuneProperty(*this);
	}

	bool LocalTuneItem::remove() {
		return _tune.remove();
	}

	bool LocalTuneItem::isEmpty() const {
		return _tune.isEmpty();
	}

	Sstring LocalTuneItem::directory() const {
		return FileInfo(_tune.path()).absoluteFilePath();
	}

	unsigned int LocalTuneItem::operator++(int) {
		return _weight++;
	}

	unsigned int LocalTuneItem::operator++() {
		return ++_weight;
	}

	TuneItemFactory *TuneItemFactory::getInstance() {
		if (!_instance)
			_instance = new TuneItemFactory();
		return _instance;
	}

	LocalTuneItem *TuneItemFactory::getTuneItem(const Sstring &path) {
		auto hashFunc = std::hash<Sstring>();
		KeyType res = hashFunc(path);
		if (_mapOfLocalMusic.count(res))
			return _mapOfLocalMusic[res].get();
		else if (FileInfo(path).exists()) {
			_mapOfLocalMusic[res] =
				std::unique_ptr<LocalTuneItem>(new LocalTuneItem(path));
			return _mapOfLocalMusic[res].get();
		}
		return nullptr;
	}

	std::unique_ptr<LocalTuneList>
	LocalTuneList::createLocalTuneList(
		const Sstring &listname,
		TuneList &&list
	) {
		if (isValidListName(listname))	// 如果歌单名符合基本法
			// 返回实例
			return std::unique_ptr<LocalTuneList>(
				new LocalTuneList(
					listname,
					std::move(list)
				)
			);
		// 返回空指针
		return std::unique_ptr<LocalTuneList>(nullptr);

	}

	Sstring LocalTuneList::name() const {
		return _name;
	}

	Sstring LocalTuneList::name() {
		return _name;
	}

	LocalTuneList::iterator LocalTuneList::begin() {
		return _list.begin();
	}

	LocalTuneList::iterator LocalTuneList::end() {
		return _list.end();
	}

	LocalTuneList::constIterator LocalTuneList::begin() const {
		return _list.begin();
	}

	LocalTuneList::constIterator LocalTuneList::end() const {
		return _list.end();
	}

	LocalTuneItem *LocalTuneList::operator[](difType index) {
		return _list[index];
	}

	bool LocalTuneList::setName(const Sstring &newname) {
		bool res = isValidListName(newname);
		if (res)
			_name = newname;
		return res;
	}

	bool LocalTuneList::add(LocalTuneItem *tune) {
		if (tune == nullptr)
			return false;
		_list.push_back(tune);
		return true;
	}

	bool LocalTuneList::remove(difType index) {
		if (index >= _list.size())
			return false;

		// 移除之
		_list.erase(_list.begin() + index);
		return true;
	}

	inline bool LocalTuneList::isValidListName(const Sstring &str) {
		return (!str.empty()) && str.size() <= MaxListName;
	}

	LocalTuneList::LocalTuneList(
		const Sstring &str,
		TuneList &&list
	) :
		_name(str)
		, _list(list)
	{}
	
	

}