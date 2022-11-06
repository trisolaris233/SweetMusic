#include "LocalDataBase.h"

namespace sweet {

	LocalDataBase::LocalDataBase(const Sstring &path) :
		_db(path.c_str()) {
	}

	bool LocalDataBase::createTable(const Sstring &tablename) {

		Sstring sqlS = 
			 L"CREATE TABLE " + tablename + L" ("
			 L"listname GRAPHIC(" + LocalTuneList::MaxListName << +") NOT NULL"
			<< ");";
	}

}