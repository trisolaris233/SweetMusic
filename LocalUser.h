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
		// �޸��û���
		bool changeName(const Sstring &newname);
		// �����赥
		// listname	�赥��
		bool createList(const Sstring &listname);
		// ɾ���赥
		// listname �赥��
		bool deleteList(const Sstring &listname);
		// �������赥
		// index		���������б�����
		// newListName	������
		bool renameList(
			sizeType index, 
			const Sstring &newListname
		);
		// �������
		// inde		���������б�����
		// tune		��������·��
		bool pushMusic(
			sizeType			index,
			const Sstring	&path
		);
		// ɾ������
		// index	���������б�����
		// index2	��������
		bool popMusic(
			sizeType index,
			sizeType index2
		);
		
		// ����listname�õ��赥������
		sizeType getListIndex(const Sstring &listname);
		// ����path��listIndex�������õ�tune������
		sizeType getTuneIndex(
			sizeType listIndex,
			const Sstring &path
		);
		// �õ��赥����Ŀ
		sizeType getListCount() const {
			return _lists.size();
		}

		// ����listname�õ��赥����
		LocalTuneList &getList(const Sstring &listname);
		// ����index�õ��赥����
		LocalTuneList &getList(sizeType index);
		
		
		// �ж�str�Ƿ�Ϊ�Ϸ����û���
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
		���ڴ洢�û����ݵı������ݿ�
	*/
	class LocalUserDataBase {
	public:
		// ���캯��, �贫�����ݿ�·��
		LocalUserDataBase(const Sstring &path);
		// Ĭ����������
		~LocalUserDataBase() = default;

		bool isOpen() const;

		bool addUser(const LocalUser *puser);	// �����ݿ�������û�
		std::unique_ptr<LocalUser> 
			getUser(const Sstring &username);	// �����ݿ��в�ѯ�û�
		bool changeName(
			const Sstring &old_name,
			const Sstring &newname,
			const LocalUser *puser
		);										// �û������������û���
		bool pushMusic(
			const Sstring &username,
			LocalTuneList &target,
			const Sstring &path
		);										// �û��������������
		bool popMusic(
			const Sstring &username,
			const Sstring &listname,
			const Sstring &path
		);										// �û��������Ƴ�����
		bool createList(
			const Sstring &username, 
			const Sstring &listname
		);										// �û������������赥
		bool deleteList(
			const Sstring &username, 
			const Sstring &listname
		);										// �û�������ɾ���赥
		bool renameList(
			const Sstring &username, 
			const Sstring &oldListname, 
			const Sstring &newListname
		);										// �û��������������赥

		static std::unique_ptr<LocalUserDataBase>
			createNewDataBase(const Sstring &databasePath);	// �½�һ���û����ݿ�

	private:
		Kompex::SQLiteDatabase
						_database;
		bool			_isopen;

	protected:
		// �Д��Ƿ�������tablename�ı�
		bool _hasTable(const Sstring &tablename);
		// �Д��Ƿ�����
		bool _hasCorrectTable();
		// �����û����������б�����������ݿ��ж�Ӧ�ı���
		Sstring _getListTableName(const Sstring &username, const Sstring &listname);
		// ���Ñ������general��
		bool _insertin2General(const Sstring &username);
		// �����������Ñ���
		bool _insertIn2UserTable(const Sstring &username, const Sstring &listname);
		// ������·������Ñ��ĸ�α�
		bool _insertIn2ListTable(const Sstring &username, const Sstring &listname, const Sstring &path);
		// �Д��������general�����Ƿ���ڴ��Ñ���
		bool _hasUser(const Sstring &username);
		// �����Ñ���
		bool _createUserTable(const Sstring &username);
		// �����Ñ����͸�����Č����ı�
		bool _createListTable(const Sstring &username, const Sstring &listname);

		// �@ȡ�Ñ��ĸ���б���
		std::vector<Sstring> _getUserList(const Sstring &username);
		// �@ȡ��εĸ���·��
		std::vector<Sstring> _getTuneList(const Sstring &username, const Sstring &listname);

		// ���ı���
		bool _renameTable(const Sstring &oldname, const Sstring &newname);
		// ��������
		template <typename T>
		bool _updateTable(
			const Sstring &tablename, const Sstring &fieldname,
			const T &olddata, const T &newdata
		);
		// ���������ַ����ػ�
		template <>
		bool _updateTable(const Sstring &tablename, const Sstring &fieldname,
			const Sstring &olddata, const Sstring &newdata);

		// �ж��ǲ��Ǹ赥����[������]
		bool _isListTableName(const Sstring &tablename);
		// ȥ�����������ŷ��ش����ֵı���
		Sstring _getOrigionListTableName(const Sstring &tablename);

		// ɾ����Ϊtablename�����ݱ�
		bool _deleteTable(const Sstring &tablename);
		// ɾ��tablename���У� fieldname������condition�ļ�¼(������ָ���)
		template <typename T>
		bool _deleteRecord(const Sstring &tablename, const Sstring &fieldname,
			const T &condition);
		template<>
		// ɾ����¼���ַ����ػ�
		bool _deleteRecord(const Sstring &tablename, const Sstring &fieldname,
			const Sstring &condition);

	};

	// �����Ñ�������
	class LocalUserManager {
	public:
		// ȡ���Ñ�����������
		// ���딵����·��
		// Ĭ�J������Ĭ�J������·��
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