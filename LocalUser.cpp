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

	Sstring LocalUserManager::defaultDatabasePath = L"local.db";	// Ĭ�J������·��
	std::map<
		Sstring, std::unique_ptr<LocalUserManager>
	>	LocalUserManager::_mapOfManagers;	

	const Sstring &LocalUser::name() const {
		return _name;
	}

	bool LocalUser::changeName(const Sstring &newname) {
		//if (isValidName(newname)) {	// �ж������Ƿ�Ϸ�
			Sstring old_name = _name;

			// �������ݿ� ����֮
			LocalUserDataBase database(_databasePath);
			if (database.changeName(old_name, newname, this)) {
				_name = newname;
				return true;
			}
		//}
		return false;
	}

	bool LocalUser::createList(const Sstring &listname) {
		if (_hasList(listname))	// ����Ѿ�����ͬ���赥
			return false;

		// ��Ӹ赥
		_lists.push_back(LocalTuneList(listname));
		LocalUserDataBase database(_databasePath);
		return database.createList(this->_name, listname);
	}

	bool LocalUser::deleteList(const Sstring &listname) {
		if (!_hasList(listname))	// �����ͼɾ��һ��tan90�ĸ赥
			return false; 

		_lists.erase(				// ���û���ɾ���赥
			std::find_if(
				_lists.begin(),
				_lists.end(), 
				[&listname](const LocalTuneList &l)->bool {
					return l.name() == listname; 
				}
			)
		);

		LocalUserDataBase database(_databasePath);
		// �������ݿ�ɾ���赥
		return database.deleteList(this->_name, listname);
	}

	bool LocalUser::renameList(
		sizeType index, 
		const Sstring &newListname
	) {
		// ���index���Ϸ�
		// �����Ѿ�������ΪnewListname�ĸ赥
		if (index >= _lists.size() || _hasList(newListname))
			return false;

		Sstring oldname(_lists[index].name());
		// �������Υ���˻�����
		if (!_lists[index].setName(newListname))
			return false;

		LocalUserDataBase database(_databasePath);
		return database.renameList(this->_name, oldname, newListname);
	}

	bool LocalUser::pushMusic(sizeType index, const Sstring &path) {
		if (index >= _lists.size())	// �ж��±��Ƿ�Ϸ�
			return false;

		// ���������Ŀ
		_lists[index].add(
			TuneItemFactory::getInstance()->getTuneItem(path)
		);
		LocalUserDataBase database(_databasePath);
		// �������ݿ�
		return database.pushMusic(
			this->_name, _lists[index], path

			);
	}

	bool LocalUser::popMusic(sizeType index, sizeType index2) {
		if (index >= _lists.size())
			return false;

		Sstring path = _lists[index]._list[index2]->path();
		// �Ƴ�����ʧ��
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
		// ���Ñ�����general����
		// ǰ������ �������ݿ�
		if (isOpen() && !_insertin2General(puser->name())) {
			return false;
		}

		// �����û���
		if (!_createUserTable(puser->name())) {	// ʧ��
			// ս�Թ���
			_deleteRecord(L"general", L"username", puser->name());
			return false;
		}
		
		for (auto eachList : puser->_lists) {
			// ��������б����Ñ���
			if (!_insertIn2UserTable(puser->name(), eachList.name()))
				return false;

			// ������α�
			if (!_createListTable(puser->name(),
				eachList.name()
			))	return false;

			for (auto eachPath : eachList._list) {
				// ����α��в�������·��
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

		// �Д�general�����Ƿ��д��Ñ�
		if (!_hasUser(username))
			return nilPtr;

		// ȡ���Ñ���
		std::unique_ptr<LocalUser> pResUser(
			new LocalUser(username)
		);
		// ����ʧ��
		if (pResUser == nullptr)
			return nilPtr;

		std::vector<Sstring> nameVtr;
		std::vector<std::vector<Sstring>> pathVtr;

		// �@ȡ����б�
		nameVtr = _getUserList(username);
		
		for (auto eachListName : nameVtr) {
			// �@ȡÿһ����ε�ÿһ�׸��·��
			auto v = _getTuneList(username, eachListName);
			pathVtr.push_back(v);
		}
		
		// �����Ñ���
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

	// ��ʱ�������ݿ�
	// oldnameΪ����
	// puserӦ��������������û�б䶯��
	// userʵ��, ���仰˵
	// ��������������changeName������
	bool LocalUserDataBase::changeName(
		const Sstring &oldname,
		const Sstring &newname,
		const LocalUser *puser
	) {
		// ����general���е�ӛ���Ñ���
		if (!_updateTable(
			L"general", L"username",
			oldname,
			newname
			)
		)	return false;

		// �������Ñ���
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
		// ���赥�������û�����
		if (!_insertIn2UserTable(username, listname))
			return false;
		// �����赥��
		if (!_createListTable(username, listname))
			return false;
		return true;
	}

	bool LocalUserDataBase::deleteList(
		const Sstring &username, 
		const Sstring &listname
	) {
		// ɾ���û����еĸ赥��
		if (!_deleteRecord(username, L"listname", listname))
			return false;

		// ɾ���赥��Ӧ�ĸ赥��
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

		// ���û����еĸ赥������
		if (!_updateTable(username, L"listname", oldListname, newListname))
			return false;

		// �������赥��
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

		// ������ʼ��general
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

	// ���Ñ����general��
	bool LocalUserDataBase::_insertin2General(const Sstring &username) {
		// ����Ѿ�����ͬ���û�
		// �򷵻�false
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

		// �Lԇ�����Ñ�
		try {
			sql.SqlStatement(statement.c_str());
		}
		catch (Kompex::SQLiteException &e) {
			ACTROLLBACK;	// ս�Իع�
			auto str = e.GetErrorDescription();
			databaseErrLog.push_back(Sstring(str.begin(), str.end()));
			return false;
		}
		ACTCOMMIT;
		return true;
	}

	// ���赥��¼���û���
	bool LocalUserDataBase::_insertIn2UserTable(
		const Sstring &username,	// �û���
		const Sstring &listname		// �赥��
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

	// ��·��¼��赥��
	bool LocalUserDataBase::_insertIn2ListTable(
		const Sstring &username,	// �û���
		const Sstring &listname,	// �赥��
		const Sstring &path			// ·��
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
		// ������û���������general����
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
		// ����û���������general����
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

	// ��ȡ�û��ĸ赥�б�
	std::vector<Sstring> LocalUserDataBase::_getUserList(
		const Sstring &username	// �û���
	) {
		typedef std::vector<Sstring> resType;
		resType res;

		// ����û���������general
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

	// �õ�·���б�
	std::vector<Sstring> LocalUserDataBase::_getTuneList(
		const Sstring &username,	// �û���
		const Sstring &listname		// �赥��
	) {
		typedef std::vector<Sstring> resType;
		resType res;

		// ����û���������general
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
		// ����������oldname�ı�
		// �����Ѵ�������newname�ı�
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
		// ���Դ����ݿ���ȡ���û�
		auto ptrFirst = LocalUserDataBase(
			getDatabasePath()).getUser(str);
		// ���ݿ���û�д��û�
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

		// �Д��Ñ����Ƿ���Ч
		if (!LocalUser::isValidName(str))
			return nilPtr;

		// ����һ���µ��Ñ�
		auto res = createUser(str);
		return res;
	}

	std::unique_ptr<LocalUser> 
	LocalUserManager::createUser(const Sstring &username) {
		// �����û�ʵ��
		std::unique_ptr<LocalUser> ptr(new LocalUser(username));
		ptr->_databasePath = getDatabasePath();
		if (!LocalUserDataBase(getDatabasePath()).addUser(ptr.get()))
			return std::unique_ptr<LocalUser>(nullptr);
		return ptr;
	}



}
