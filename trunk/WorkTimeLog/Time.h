#ifndef __TIME_H__
# define __TIME_H__

#include <windows.h>
#include <tchar.h>
#include <time.h>
#include <vector>
#include "SQLite.h"

#include "Task.h"



class Time
{
private:
	sqlite::Database *db;
	
public:
	
	sqlite3_int64 id;
	Task task;
	time_t start;
	time_t end;

	Time(sqlite::Database *db, sqlite::Statement *stmt);
	Time(sqlite::Database *db = NULL);
	~Time();
	
	void store();
	void remove();
	
	
	static void createTable(sqlite::Database *db);
	static void dropTable(sqlite::Database *db);
	
	static Time query(sqlite::Database *db, sqlite3_int64 id);
	static std::vector<Time> queryAll(sqlite::Database *db);
	static std::vector<Time> queryAll(sqlite::Database *db, Task *task = NULL, time_t *start = NULL, time_t *end = NULL, const TCHAR *orderBy = NULL);
	
	static void store(sqlite::Database *db, Time *obj);
	static void remove(sqlite::Database *db, Time *obj);
	
};


#endif // __TIME_H__
