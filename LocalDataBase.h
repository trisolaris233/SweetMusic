#ifndef __LOCAL_DATABASE_H__
#define __LOCAL_DATABASE_H__

#include "Sstring.h"
#include "KompexSQLiteDatabase.h"

namespace sweet {
	
	/*
		LocalDataBase: 本地數據庫類,
		用戶創建表，數據庫, 增加， 刪除數據等基本操作
	*/
	class LocalDataBase {
	public:
		typedef Kompex::SQLiteDatabase DatabaseType;
		// 構造函數
		// 副作用: 打開或創建一個數據庫
		//              ^^^^
		// 如果數據庫已經存在且一切正常， 則打開之, 
		// 如果不存在此數據庫則會創建一個新的數據庫
		LocalDataBase(const Sstring &path);

		// 創建一個新表
		bool createTable(const Sstring &tablename);
		// 重命名一個表
		bool renameTable(
			const Sstring &oldname,
			const Sstring &newname
		);
		// 返回_db本尊 用戶用戶實現更複雜的功能
		DatabaseType &instance();
		

	private:
		DatabaseType _db;	// 維護的本地database
		

	};

}

#endif