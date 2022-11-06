#include "FileInfo.h"
#include "LocalUser.h"
#include "IceException.h"

#include "include\KompexSQLiteStatement.h"
#include "include\KompexSQLiteException.h"

#include <cctype>
#include <vector>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <functional>

#pragma comment(lib, "lib\\sqliteWrapper.lib")

#define CREATEDATABASE \
	Kompex::SQLiteStatement sql(&_database);\
	std::wostringstream oss

#define BEGIN L"BEGIN;"
#define COMMIT L"COMMIT;"
#define ROLLBACK L"ROLLBACK;"
#define ACTBEGIN sql.Sql(BEGIN)
#define ACTROLLBACK sql.Sql(ROLLBACK)
#define ACTCOMMIT sql.Sql(COMMIT)
	

namespace icemusic {

	std::vector<Sstring> databaseErrLog;

	Sstring LocalUserManager::defaultDatabasePath = L"local.db";	// 默認數據庫路徑
	std::map<
		Sstring, std::unique_ptr<LocalUserManager>
	>	LocalUserManager::_mapOfManagers;	

	const Sstring &LocalUser::name() const {
		return _name;
	}

	bool LocalUser::changeName(const Sstring &newname) {
		//if (isValidName(newname)) {	// 判断名称是否合法
			Sstring old_name = _name;

			// 调动数据库 更新之
			LocalUserDataBase database(_databasePath);
			if (database.changeName(old_name, newname, this)) {
				_name = newname;
				return true;
			}
		//}
		return false;
	}

	bool LocalUser::createList(const Sstring &listname) {
		if (_hasList(listname))	// 如果已经存在同名歌单
			return false;

		// 添加歌单
		_lists.push_back(LocalTuneList(listname));
		LocalUserDataBase database(_databasePath);
		return database.createList(this->_name, listname);
	}

	bool LocalUser::deleteList(const Sstring &listname) {
		if (!_hasList(listname))	// 如果试图删除一个tan90的歌单
			return false; 

		_lists.erase(				// 从用户中删除歌单
			std::find_if(
				_lists.begin(),
				_lists.end(), 
				[&listname](const LocalTuneList &l)->bool {
					return l.name() == listname; 
				}
			)
		);

		LocalUserDataBase database(_databasePath);
		// 调用数据库删除歌单
		return database.deleteList(this->_name, listname);
	}

	bool LocalUser::renameList(
		sizeType index, 
		const Sstring &newListname
	) {
		// 如果index不合法
		// 或者已经存在名为newListname的歌单
		if (index >= _lists.size() || _hasList(newListname))
			return false;

		Sstring oldname(_lists[index].name());
		// 如果名称违反了稽本法
		if (!_lists[index].setName(newListname))
			return false;

		LocalUserDataBase database(_databasePath);
		return database.renameList(this->_name, oldname, newListname);
	}

	bool LocalUser::pushMusic(sizeType index, const Sstring &path) {
		if (index >= _lists.size())	// 判断下标是否合法
			return false;

		// 添加音乐项目
		_lists[index].add(
			TuneItemFactory::getInstance()->getTuneItem(path)
		);
		LocalUserDataBase database(_databasePath);
		// 调动数据库
		return database.pushMusic(
			this->_name, _lists[index], path

			);
	}

	bool LocalUser::popMusic(sizeType index, sizeType index2) {
		if (index >= _lists.size())
			return false;

		Sstring path = _lists[index]._list[index2]->path();
		// 移除音乐失败
		if (!_lists[index].remove(index2))
			return false;

		LocalUserDataBase database(_databasePath);
		return database.popMusic(
			this->_name, 
			_lists[index].name(), 
			path
		);
	}

	LocalUser::sizeType LocalUser::getListIndex(const Sstring &listname) {
		for (sizeType i = 0; i < _lists.size(); ++i)
		if (_lists[i].name() == listname)
			return i;
		return -1;
	}

	LocalUser::sizeType LocalUser::getTuneIndex(
		sizeType listIndex,
		const Sstring &path
	) {
		if (listIndex >= _lists.size())
			return -1;

		sizeType res = 0;
		for (auto each : _lists[listIndex]._list) {
			if (path == each->path())
				return res;
			++res;
		}
		return -1;
	}

	LocalTuneList &LocalUser::getList(const Sstring &listname) {
		return _lists[getListIndex(listname)];
	}

	LocalTuneList &LocalUser::getList(sizeType index) {
		return _lists[index];
	}

	bool LocalUser::isValidName(const Sstring &str) {
		return (!str.empty() && str.size() <= MaxUserName);
	}

	LocalUser::LocalUser(const Sstring &str) :
		_name(str) {}

	bool LocalUser::_hasList(const Sstring &listname) {
		for (auto l : _lists)
		if (l.name() == listname)
			return true;
		return false;
	}

	LocalUserDataBase::LocalUserDataBase(const Sstring &path) :
		_database(path.c_str()) {
		if (_database.GetDatabaseHandle() == nullptr)
			_isopen = false;
		else
			_isopen = true;
	}

	bool LocalUserDataBase::isOpen() const {
		return _database.GetDatabaseHandle() != nullptr;
	}

	bool LocalUserDataBase::addUser(const LocalUser *puser) {
		// 將用戶插入general表中
		// 前条件： 存在数据库
		if (isOpen() && !_insertin2General(puser->name())) {
			return false;
		}

		// 创建用户表
		if (!_createUserTable(puser->name())) {	// 失败
			// 战略滚回
			_deleteRecord(L"general", L"username", puser->name());
			return false;
		}
		
		for (auto eachList : puser->_lists) {
			// 創建歌單列表于用戶表
			if (!_insertIn2UserTable(puser->name(), eachList.name()))
				return false;

			// 創建歌單表
			if (!_createListTable(puser->name(),
				eachList.name()
			))	return false;

			for (auto eachPath : eachList._list) {
				// 往歌單表中插入音樂路徑
				if (!_insertIn2ListTable(puser->name(),
					eachList.name(), eachPath->path()
				))	return false;
			}
		}
		return true;
	}

	std::unique_ptr<LocalUser> LocalUserDataBase::getUser(
		const Sstring &username
	) {
		std::unique_ptr<LocalUser> nilPtr(nullptr);

		// 判斷general表中是否有此用戶
		if (!_hasUser(username))
			return nilPtr;

		// 取得用戶實例
		std::unique_ptr<LocalUser> pResUser(
			new LocalUser(username)
		);
		// 分配失敗
		if (pResUser == nullptr)
			return nilPtr;

		std::vector<Sstring> nameVtr;
		std::vector<std::vector<Sstring>> pathVtr;

		// 獲取歌單列表
		nameVtr = _getUserList(username);
		
		for (auto eachListName : nameVtr) {
			// 獲取每一個歌單的每一首歌的路徑
			auto v = _getTuneList(username, eachListName);
			pathVtr.push_back(v);
		}
		
		// 加入用戶中
		auto pathVtrItr = pathVtr.begin();
		for (auto itr = nameVtr.begin(); 
			itr != nameVtr.end() && pathVtrItr != pathVtr.end();
			++itr, ++pathVtrItr
		) {
			TuneList tmp;
			
			for (auto eachPath : *pathVtrItr) {
				tmp.push_back(TuneItemFactory::getInstance()->getTuneItem(eachPath));
			}

			pResUser->_lists.push_back(
				LocalTuneList(
					*itr, 
					std::move(tmp)
				)
			);
		}
		return pResUser;
		
	}

	// 即时更新数据库
	// oldname为旧名
	// puser应传除了名字以外没有变动的
	// user实例, 换句话说
	// 改名后立即调用changeName方法。
	bool LocalUserDataBase::changeName(
		const Sstring &oldname,
		const Sstring &newname,
		const LocalUser *puser
	) {
		// 更新general表中登記的用戶名
		if (!_updateTable(
			L"general", L"username",
			oldname,
			newname
			)
		)	return false;

		// 重命名用戶表
		if (!_renameTable(oldname, newname)) {
			_updateTable(L"general", L"username", newname, oldname);
			return false;
		}

		for (auto eachList : puser->_lists) {
			Sstring oldTableName = _getListTableName(oldname, eachList.name());
			Sstring newTableName = _getListTableName(puser->name(), eachList.name());
			if (!_renameTable(oldTableName, newTableName))
				return false;
		}
		
		return true;
	}

	bool LocalUserDataBase::pushMusic(
		const Sstring &username,
		LocalTuneList &target,
		const Sstring &path
	) {
		return _insertIn2ListTable(
			username, target.name(), path
		);
	}

	bool LocalUserDataBase::popMusic(
		const Sstring &username,
		const Sstring &listname,
		const Sstring &path) {
		return
			_deleteRecord(
			_getListTableName(username, listname),
			L"path", path
		);
	}

	bool LocalUserDataBase::createList(
		const Sstring &username, 
		const Sstring &listname
	) {
		// 将歌单名加入用户表中
		if (!_insertIn2UserTable(username, listname))
			return false;
		// 创建歌单表
		if (!_createListTable(username, listname))
			return false;
		return true;
	}

	bool LocalUserDataBase::deleteList(
		const Sstring &username, 
		const Sstring &listname
	) {
		// 删除用户表中的歌单名
		if (!_deleteRecord(username, L"listname", listname))
			return false;

		// 删除歌单对应的歌单表
		if (!_deleteTable(_getListTableName(username, listname)))
			return false;
		return true;
	}

	bool LocalUserDataBase::renameList(
		const Sstring &username, 
		const Sstring &oldListname, 
		const Sstring &newListname
	) {
		std::string errorString("failed in renameList.\ndetails: ");

		// 将用户表中的歌单重命名
		if (!_updateTable(username, L"listname", oldListname, newListname))
			return false;

		// 重命名歌单表
		if (!_renameTable(
			_getListTableName(username, oldListname),
			_getListTableName(username, newListname)
			)
		)	return false;
		return true;
	}

	std::unique_ptr<LocalUserDataBase> 
	LocalUserDataBase::createNewDataBase(
		const Sstring &databasePath
	) {
		std::unique_ptr<LocalUserDataBase> 
			nilPtr(nullptr),
			pRes(new LocalUserDataBase(databasePath));

		if (!pRes->isOpen())
			return nilPtr;

		// 創建初始表general
		Kompex::SQLiteStatement sql (&(pRes->_database));
		std::wostringstream oss;
		ACTBEGIN;
		oss
			<< L"CREATE TABLE general (username GRAPHIC("
			<< LocalUser::MaxUserName
			<< L") NOT NULL UNIQUE);";

		try {
			sql.SqlStatement(oss.str().c_str());
		}
		catch (Kompex::SQLiteException &e) {
			ACTROLLBACK;
			throw SweetException(e.GetErrorDescription());
			return false;
		}
		ACTCOMMIT;
		return pRes;
	}

	bool LocalUserDataBase::_hasTable(const Sstring &tablename) {
		Kompex::SQLiteStatement sql(&_database);
		Sstring statement(
			L"SELECT * FROM " + tablename + L";"
		);
		try {
			sql.Sql(statement.c_str());
		}
		catch (...) {
			return false;
		}
		bool res = false;
		try {
			if (sql.GetDataCount() >= 0) {
				return true;
			}
		}
		catch (...) {
			return false;
		}
		return res;
	}

	bool LocalUserDataBase::_hasCorrectTable() {
		return _hasTable(L"general");
	}

	Sstring LocalUserDataBase::_getListTableName(
		const Sstring &username,
		const Sstring &listname
		) {
		std::hash<Sstring> getHash;
		std::wstringstream oss;

		auto res = getHash(username) + getHash(listname);
		oss << res;
		Sstring tmp = oss.str();
		Sstring tableName;

		for (auto ch : tmp)
			if (std::isdigit(ch))
				tableName += ch;
		
		return L"[" + tableName + L"]";
	}

	// 將用戶錄入general表
	bool LocalUserDataBase::_insertin2General(const Sstring &username) {
		// 如果已经存在同名用户
		// 则返回false
		if (_hasUser(username)) {
			databaseErrLog.push_back(
				L"failed in inserting username to general table : there's almost a same username."
			);
			return false;
		}
		
		CREATEDATABASE;
		ACTBEGIN;
		Sstring statement = 
			L"INSERT INTO general VALUES ("
			+ Sstring(L"\'") + username + L"\');";

		// 嘗試創建用戶
		try {
			sql.SqlStatement(statement.c_str());
		}
		catch (Kompex::SQLiteException &e) {
			ACTROLLBACK;	// 战略回滚
			auto str = e.GetErrorDescription();
			databaseErrLog.push_back(Sstring(str.begin(), str.end()));
			return false;
		}
		ACTCOMMIT;
		return true;
	}

	// 将歌单名录入用户表
	bool LocalUserDataBase::_insertIn2UserTable(
		const Sstring &username,	// 用户名
		const Sstring &listname		// 歌单名
	) {
		CREATEDATABASE;
		ACTBEGIN;
		oss << "INSERT INTO " << username << " VALUES ("
			<< "\'" << listname << "\')";
		Sstring statement(oss.str());

		try {
			sql.SqlStatement(statement.c_str());
		}
		catch (Kompex::SQLiteException &e) {
			ACTROLLBACK;
			auto str = e.GetErrorDescription();
			databaseErrLog.push_back(Sstring(str.begin(), str.end()));
			return false;
		}
		ACTCOMMIT;
		return true;
	}

	// 将路径录入歌单表
	bool LocalUserDataBase::_insertIn2ListTable(
		const Sstring &username,	// 用户名
		const Sstring &listname,	// 歌单名
		const Sstring &path			// 路径
	) {
		CREATEDATABASE;
		ACTBEGIN;
		oss
			<< "INSERT INTO " << _getListTableName(username, listname) << " VALUES("
			<< "\'" << path << "\');";
		Sstring statement(oss.str());

		try {
			sql.SqlStatement(statement.c_str());
		}
		catch (Kompex::SQLiteException &e) {
			ACTROLLBACK;
			auto str = e.GetErrorDescription();
			databaseErrLog.push_back(Sstring(str.begin(), str.end()));
			return false;
		}
		ACTCOMMIT;
		return true;

	}

	bool LocalUserDataBase::_hasUser(const Sstring &username) {

		CREATEDATABASE;
		Sstring statement(
			L"SELECT * FROM general WHERE username = \'"
			+ username + L"\';"
		);

		try {
			sql.Sql(statement.c_str());
		}
		catch (Kompex::SQLiteException &) {
			//throw SweetException(e.GetErrorDescription());
			return false;
		}
		try {
			if (sql.FetchRow())
				return true;
		}
		catch (Kompex::SQLiteException &) {
			//throw SweetException(e.GetErrorDescription());
			return false;
		}
		return false;
	}

	bool LocalUserDataBase::_createUserTable(const Sstring &username) {
		// 如果此用户不存在与general表中
		if (!_hasUser(username)) {
			databaseErrLog.push_back(
				L"there's no such an user in general."
			);
			return false;
		}
		
		CREATEDATABASE;
		ACTBEGIN;
		oss
			<< "CREATE TABLE " << username << " ("
			<< "listname GRAPHIC(" << LocalTuneList::MaxListName << ") NOT NULL"
			<< ");";
		Sstring statement(oss.str());

		try {
			sql.SqlStatement(statement.c_str());
		}
		catch (Kompex::SQLiteException &e) {
			ACTROLLBACK;
			auto str = e.GetErrorDescription();
			databaseErrLog.push_back(Sstring(str.begin(), str.end()));
			return false;
		}
		ACTCOMMIT;
		return true;
	}

	bool LocalUserDataBase::_createListTable(
		const Sstring &username,
		const Sstring &listname
	) {
		// 如果用户不存在与general表中
		if (!_hasUser(username)) {
			databaseErrLog.push_back(
				L"there's no such an user in general."
			);
			return false;
		}
		
		CREATEDATABASE;
		ACTBEGIN;
		Sstring tablename(_getListTableName(username, listname));
		oss
			<< "CREATE TABLE" << tablename << " ("
			<< "path GRAPHIC(" << _MAX_PATH << ")"
			<< ");";
		Sstring statement(oss.str());

		try {
			sql.SqlStatement(statement.c_str());
		}
		catch (Kompex::SQLiteException &e) {
			ACTROLLBACK;
			auto str = e.GetErrorDescription();
			databaseErrLog.push_back(Sstring(str.begin(), str.end()));
			return false;
		}
		ACTCOMMIT;
		return true;

	}

	// 获取用户的歌单列表
	std::vector<Sstring> LocalUserDataBase::_getUserList(
		const Sstring &username	// 用户名
	) {
		typedef std::vector<Sstring> resType;
		resType res;

		// 如果用户不存在于general
		if (!_hasUser(username)) {
			databaseErrLog.push_back(
				L"there's no such an user in general."
			);
			return res;
		}

		Kompex::SQLiteStatement sql(&_database);
		Sstring statement(L"SELECT listname FROM " + username + L";");
		try {
			sql.Sql(statement.c_str());
		}
		catch (Kompex::SQLiteException &e) {
			auto str = e.GetErrorDescription();
			databaseErrLog.push_back(Sstring(str.begin(), str.end()));
			return res;
		}
		try {
			while (sql.FetchRow()) {
				res.push_back(sql.GetColumnString16(0));
			}
		}
		catch (Kompex::SQLiteException &e) {
			auto str = e.GetErrorDescription();
			databaseErrLog.push_back(Sstring(str.begin(), str.end()));
			return res;
		}

		return res;
	}

	// 得到路径列表
	std::vector<Sstring> LocalUserDataBase::_getTuneList(
		const Sstring &username,	// 用户名
		const Sstring &listname		// 歌单名
	) {
		typedef std::vector<Sstring> resType;
		resType res;

		// 如果用户不存在于general
		if (!_hasUser(username)) {
			databaseErrLog.push_back(
				L"there's no such an user in general."
			);
			return res;
		}

		Kompex::SQLiteStatement sql(&_database);
		Sstring statement(L"SELECT * FROM "
			+ _getListTableName(username, listname)
			+ L";");
		try {
			sql.Sql(statement.c_str());
		}
		catch (Kompex::SQLiteException &e) {
			auto str = e.GetErrorDescription();
			databaseErrLog.push_back(Sstring(str.begin(), str.end()));
			return res;
		}
		try {
			while (sql.FetchRow()) {
				res.push_back(sql.GetColumnString16(0));
			}
		}
		catch (Kompex::SQLiteException &e) {
			auto str = e.GetErrorDescription();
			databaseErrLog.push_back(Sstring(str.begin(), str.end()));
			return res;
		}

		return res;
	}

	bool LocalUserDataBase::_renameTable(
		const Sstring &oldname, 
		const Sstring &newname
	) {
		// 不存在名為oldname的表
		// 或者已存在名為newname的表
		if (!_hasTable(oldname)) {
			databaseErrLog.push_back(L"there's no such a table named " + oldname);
			return false;
		}

		CREATEDATABASE;
		ACTBEGIN;
		Sstring	statement(
			L"ALTER TABLE \""
			+ _getOrigionListTableName(oldname)
			+ L"\" RENAME TO \"" + _getOrigionListTableName(newname) + L"\";"
		);

		try {
			sql.SqlStatement(statement.c_str());
		}
		catch (Kompex::SQLiteException &e) {
			ACTROLLBACK;
			auto str = e.GetErrorDescription();
			databaseErrLog.push_back(Sstring(str.begin(), str.end()));
			return false;
		}
		ACTCOMMIT;
		return true;
	}

	template <typename T>
	bool LocalUserDataBase::_updateTable(
		const Sstring &tablename,
		const Sstring &fieldname,
		const T &olddata,
		const T &newdata
	) {
		if (!_hasTable(tablename)) {
			databaseErrLog.push_back(L"there's no such a table named " + tablename);
			return false;
		}

		CREATEDATABASE;
		ACTBEGIN;
		Sstring dataText1, dataText2;
		oss << olddata;
		dataText1 = oss.str();
		oss.str(L""); 
		oss << newdata;
		dataText2 = oss.str();

		Sstring statement(
			L"UPDATE " + tablename + " SET " + fieldname + " = "
			+ dataText1 + L" WHERE " + fieldname + " = " +
			+dataText2 + L";"
		);
		
		try {
			sql.SqlStatement(statement.c_str());
		}
		catch (Kompex::SQLiteException &e) {
			ACTROLLBACK;
			auto str = e.GetErrorDescription();
			databaseErrLog.push_back(Sstring(str.begin(), str.end()));
			return false;
		}
		ACTCOMMIT;
		return true;
	}

	template <>
	bool LocalUserDataBase::_updateTable(const Sstring &tablename, const Sstring &fieldname,
		const Sstring &olddata, const Sstring &newdata) {
		if (!_hasTable(tablename)) {
			databaseErrLog.push_back(L"there's no such a table named " + tablename);
			return false;
		}

		CREATEDATABASE;
		ACTBEGIN;
		Sstring statement(
			L"UPDATE " + tablename + L" SET " + fieldname + L" = \'"
			+ newdata + L"\' WHERE " + fieldname + L" = \'"
			+ olddata + L"\';"
		);

		try {
			sql.SqlStatement(statement.c_str());
		}
		catch (Kompex::SQLiteException &e) {
			ACTROLLBACK;
			auto str = e.GetErrorDescription();
			databaseErrLog.push_back(Sstring(str.begin(), str.end()));
			return false;
		}
		ACTCOMMIT;
		return true;
	}

	bool LocalUserDataBase::_isListTableName(const Sstring &tablename) {
		return tablename.size() >= 2
			&& tablename[0] == '[' 
			&& tablename[tablename.length() - 1] == ']';
	}

	Sstring LocalUserDataBase::_getOrigionListTableName(const Sstring &tablename) {
		if (_isListTableName(tablename))
			return tablename.substr(1, tablename.length() - 2);
		return tablename;
	}

	bool LocalUserDataBase::_deleteTable(const Sstring &tablename) {
		CREATEDATABASE;
		ACTBEGIN;
		Sstring statement(L"DROP TABLE " + tablename + L";");

		try {
			sql.SqlStatement(statement.c_str());
		}
		catch (Kompex::SQLiteException &e) {
			ACTROLLBACK;
			auto str = e.GetErrorDescription();
			databaseErrLog.push_back(Sstring(str.begin(), str.end()));
			return false;
		}
		ACTCOMMIT;
		return true;
	}

	template <typename T>
	bool LocalUserDataBase::_deleteRecord(
		const Sstring &tablename,
		const Sstring &fieldname,
		const T &condition
	) {
		CREATEDATABASE;
		ACTBEGIN;
		oss
			<< "DELETE FROM " << tablename
			<< " WHERE " << fieldname << " = "
			<< condition << ";";
		Sstring statement(oss.str());

		try {
			sql.SqlStatement(statement.c_str());
		}
		catch (Kompex::SQLiteException &e) {
			ACTROLLBACK;
			auto str = e.GetErrorDescription();
			databaseErrLog.push_back(Sstring(str.begin(), str.end()));
			return false;
		}
		ACTCOMMIT;
		return true;
	}

	template <>
	bool LocalUserDataBase::_deleteRecord(
		const Sstring &tablename,
		const Sstring &fieldname,
		const Sstring &condition
	) {
		CREATEDATABASE;
		ACTBEGIN;
		oss
			<< "DELETE FROM " << tablename
			<< " WHERE " << fieldname << " = \'"
			<< condition << "\';";
		Sstring statement(oss.str());

		try {
			sql.SqlStatement(statement.c_str());
		}
		catch (Kompex::SQLiteException &e) {
			ACTROLLBACK;
			auto str = e.GetErrorDescription();
			databaseErrLog.push_back(Sstring(str.begin(), str.end()));
			return false;
		}
		ACTCOMMIT;
		return true;
	}

	LocalUserManager *LocalUserManager::getInstance(
				const Sstring &databasePath
	) {
		if (_mapOfManagers.count(databasePath))
			return _mapOfManagers[databasePath].get();
		else if (
			!FileInfo(databasePath).exists() &&
			LocalUserDataBase::createNewDataBase(
			databasePath
			).get() == nullptr
		)	return nullptr;
		_mapOfManagers[databasePath] = 
			std::unique_ptr<LocalUserManager>(new LocalUserManager);
		return _mapOfManagers[databasePath].get();
	}

	std::unique_ptr<LocalUser> 
	LocalUserManager::getUser(const Sstring &str) {
		// 尝试从数据库中取得用户
		auto ptrFirst = LocalUserDataBase(
			getDatabasePath()).getUser(str);
		// 数据库中没有此用户
		if (ptrFirst.get() == nullptr)
			return nullptr;
		ptrFirst.get()->_databasePath = getDatabasePath();
		return ptrFirst;
	}

	bool LocalUserManager::addUser(LocalUser *puser) {
		return LocalUserDataBase(
			getDatabasePath()).addUser(puser);
	}

	
	Sstring LocalUserManager::getDatabasePath() {
		for (auto itr = _mapOfManagers.begin();
			itr != _mapOfManagers.end();++itr)
		if (itr->second.get() == this)
			return itr->first;
		return Sstring();
	}

	std::unique_ptr<LocalUser> 
	LocalUserManager::registerUser(const Sstring &str) {
		std::unique_ptr<LocalUser> nilPtr(nullptr);

		// 判斷用戶名是否有效
		if (!LocalUser::isValidName(str))
			return nilPtr;

		// 創建一個新的用戶
		auto res = createUser(str);
		return res;
	}

	std::unique_ptr<LocalUser> 
	LocalUserManager::createUser(const Sstring &username) {
		// 创建用户实例
		std::unique_ptr<LocalUser> ptr(new LocalUser(username));
		ptr->_databasePath = getDatabasePath();
		if (!LocalUserDataBase(getDatabasePath()).addUser(ptr.get()))
			return std::unique_ptr<LocalUser>(nullptr);
		return ptr;
	}



}
