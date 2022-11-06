#ifndef __LOCAL_DATABASE_H__
#define __LOCAL_DATABASE_H__

#include "Sstring.h"
#include "KompexSQLiteDatabase.h"

namespace sweet {
	
	/*
		LocalDataBase: ���ؔ������,
		�Ñ􄓽���������, ���ӣ� �h�������Ȼ�������
	*/
	class LocalDataBase {
	public:
		typedef Kompex::SQLiteDatabase DatabaseType;
		// ���캯��
		// ������: ���_�򄓽�һ��������
		//              ^^^^
		// ����������ѽ�������һ�������� �t���_֮, 
		// ��������ڴ˔�����t������һ���µĔ�����
		LocalDataBase(const Sstring &path);

		// ����һ���±�
		bool createTable(const Sstring &tablename);
		// ������һ����
		bool renameTable(
			const Sstring &oldname,
			const Sstring &newname
		);
		// ����_db���� �Ñ��Ñ􌍬F���}�s�Ĺ���
		DatabaseType &instance();
		

	private:
		DatabaseType _db;	// �S�o�ı���database
		

	};

}

#endif