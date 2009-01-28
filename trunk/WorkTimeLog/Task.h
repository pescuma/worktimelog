#ifndef __TASK_H__
# define __TASK_H__

#include <windows.h>
#include <tchar.h>
#include <vector>
#include <string>
#include "SQLite.h"

namespace std {
typedef basic_string<TCHAR, char_traits<TCHAR>, allocator<TCHAR> > tstring;
}



class Task
{
private:
	sqlite::Database *db;
	
public:
	
	sqlite3_int64 id;
	std::tstring name;

	Task(sqlite::Database *db, sqlite::Statement *stmt);
	Task(sqlite::Database *db = NULL);
	~Task();
	
	void store();
	void remove();
	
	
	static void createTable(sqlite::Database *db);
	static void dropTable(sqlite::Database *db);
	
	static Task query(sqlite::Database *db, sqlite3_int64 id);
	static std::vector<Task> queryAll(sqlite::Database *db);
	static std::vector<Task> queryAll(sqlite::Database *db, std::tstring *name = NULL, const TCHAR *orderBy = NULL);
	
	static void store(sqlite::Database *db, Task *obj);
	static void remove(sqlite::Database *db, Task *obj);
	
};


#endif // __TASK_H__
