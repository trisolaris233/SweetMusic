#ifndef __LOCAL_USER_H__
#define __LOCAL_USER_H__

#include <map>
#include <vector>
#include <memory>

#include "Sstring.h"
#include "LocalTuneItem.h"

#include <include\KompexSQLiteDatabase.h>

namespace icemusic {

	extern std::vector<Sstring> databaseErrLog;

	class LocalUser {
	public:
		typedef LocalTuneList::difType					difType;
		typedef std::vector<LocalTuneList>::size_type	sizeType;

		enum {
			MaxUserName = 20
		};

		const Sstring &name() const;
		// 修改用户名
		bool changeName(const Sstring &newname);
		// 创建歌单
		// listname	歌单名
		bool createList(const Sstring &listname);
		// 删除歌单
		// listname 歌单名
		bool deleteList(const Sstring &listname);
		// 重命名歌单
		// index		本地音乐列表索引
		// newListName	新名称
		bool renameList(
			sizeType index, 
			const Sstring &newListname
		);
		// 添加音乐
		// inde		本地音乐列表索引
		// tune		本地音乐路径
		bool pushMusic(
			sizeType			index,
			const Sstring	&path
		);
		// 删除音乐
		// index	本地音乐列表索引
		// index2	音乐索引
		bool popMusic(
			sizeType index,
			sizeType index2
		);
		
		// 根据listname得到歌单的索引
		sizeType getListIndex(const Sstring &listname);
		// 根据path和listIndex的索引得到tune的索引
		sizeType getTuneIndex(
			sizeType listIndex,
			const Sstring &path
		);
		// 得到歌单的数目
		sizeType getListCount() const {
			return _lists.size();
		}

		// 根据listname得到歌单本尊
		LocalTuneList &getList(const Sstring &listname);
		// 根据index得到歌单本尊
		LocalTuneList &getList(sizeType index);
		
		
		// 判断str是否为合法的用户名
		static bool isValidName(const Sstring &str);

	private:
		Sstring	_name;
		std::vector<LocalTuneList> 
				_lists;
		Sstring _databasePath;

		LocalUser(const Sstring &str);

		friend class LocalUserManager;
		friend class LocalUserDataBase;
		friend class LocalTuneList;

	protected:
		bool _hasList(const Sstring &listname);

	};

	/*
		用于存储用户数据的本地数据库
	*/
	class LocalUserDataBase {
	public:
		// 构造函数, 需传入数据库路径
		LocalUserDataBase(const Sstring &path);
		// 默认析构函数
		~LocalUserDataBase() = default;

		bool isOpen() const;

		bool addUser(const LocalUser *puser);	// 向数据库中添加用户
		std::unique_ptr<LocalUser> 
			getUser(const Sstring &username);	// 在数据库中查询用户
		bool changeName(
			const Sstring &old_name,
			const Sstring &newname,
			const LocalUser *puser
		);										// 用户操作：更改用户名
		bool pushMusic(
			const Sstring &username,
			LocalTuneList &target,
			const Sstring &path
		);										// 用户操作：添加音乐
		bool popMusic(
			const Sstring &username,
			const Sstring &listname,
			const Sstring &path
		);										// 用户操作：移除音乐
		bool createList(
			const Sstring &username, 
			const Sstring &listname
		);										// 用户操作：创建歌单
		bool deleteList(
			const Sstring &username, 
			const Sstring &listname
		);										// 用户操作：删除歌单
		bool renameList(
			const Sstring &username, 
			const Sstring &oldListname, 
			const Sstring &newListname
		);										// 用户操作：重命名歌单

		static std::unique_ptr<LocalUserDataBase>
			createNewDataBase(const Sstring &databasePath);	// 新建一个用户数据库

	private:
		Kompex::SQLiteDatabase
						_database;
		bool			_isopen;

	protected:
		// 判斷是否含有名為tablename的表
		bool _hasTable(const Sstring &tablename);
		// 判斷是否完整
		bool _hasCorrectTable();
		// 根据用户名与音乐列表名计算出数据库中对应的表名
		Sstring _getListTableName(const Sstring &username, const Sstring &listname);
		// 將用戶名錄入general表
		bool _insertin2General(const Sstring &username);
		// 將歌單名錄入用戶表
		bool _insertIn2UserTable(const Sstring &username, const Sstring &listname);
		// 將歌曲路徑錄入用戶的歌單表
		bool _insertIn2ListTable(const Sstring &username, const Sstring &listname, const Sstring &path);
		// 判斷數據庫的general表中是否存在此用戶名
		bool _hasUser(const Sstring &username);
		// 創建用戶表
		bool _createUserTable(const Sstring &username);
		// 創建用戶名和歌單名的對應的表
		bool _createListTable(const Sstring &username, const Sstring &listname);

		// 獲取用戶的歌單列表名
		std::vector<Sstring> _getUserList(const Sstring &username);
		// 獲取歌單的歌曲路徑
		std::vector<Sstring> _getTuneList(const Sstring &username, const Sstring &listname);

		// 更改表名
		bool _renameTable(const Sstring &oldname, const Sstring &newname);
		// 更新数据
		template <typename T>
		bool _updateTable(
			const Sstring &tablename, const Sstring &fieldname,
			const T &olddata, const T &newdata
		);
		// 更新數據の字符串特化
		template <>
		bool _updateTable(const Sstring &tablename, const Sstring &fieldname,
			const Sstring &olddata, const Sstring &newdata);

		// 判断是不是歌单表名[纯数字]
		bool _isListTableName(const Sstring &tablename);
		// 去除两边中括号返回纯数字的表名
		Sstring _getOrigionListTableName(const Sstring &tablename);

		// 删除名为tablename的数据表
		bool _deleteTable(const Sstring &tablename);
		// 删除tablename表中， fieldname列满足condition的记录(满足特指相等)
		template <typename T>
		bool _deleteRecord(const Sstring &tablename, const Sstring &fieldname,
			const T &condition);
		template<>
		// 删除记录の字符串特化
		bool _deleteRecord(const Sstring &tablename, const Sstring &fieldname,
			const Sstring &condition);

	};

	// 本地用戶管理器
	class LocalUserManager {
	public:
		// 取得用戶管理器實例
		// 傳入數據庫路徑
		// 默認參數為默認數據庫路徑
		static LocalUserManager *getInstance(
				const Sstring &databasePath = defaultDatabasePath
		);

		std::unique_ptr<LocalUser> getUser(const Sstring &str);
		bool addUser(LocalUser *puser);
		std::unique_ptr<LocalUser> registerUser(const Sstring &str);

	private:
		static std::map<
			Sstring, std::unique_ptr<LocalUserManager>
		> _mapOfManagers;
		
		static Sstring defaultDatabasePath;

	protected:
		LocalUserManager() = default;
		~LocalUserManager() = default;
		Sstring getDatabasePath();
		std::unique_ptr<LocalUser> createUser(const Sstring &username);

	};

}

#endif