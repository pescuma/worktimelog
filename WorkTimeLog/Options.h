#ifndef __OPTIONS_H__
# define __OPTIONS_H__

#include <windows.h>
#include <tchar.h>
#include <time.h>
#include <vector>
#include "SQLite.h"

#include "Time.h"



class Options
{
private:
	sqlite::Database *db;
	
public:
	
	sqlite3_int64 id;
	Time currentTime;
	time_t lastCheck;
	int stopTimeMs;
	int startTimeMs;
	int idleDuringStartTimeMs;

	Options(sqlite::Database *db, sqlite::Statement *stmt);
	Options(sqlite::Database *db = NULL);
	~Options();
	
	void connectTo(sqlite::Database *db);
	void store();
	void remove();
	bool isStored();
	
	
	static void createTable(sqlite::Database *db);
	static void dropTable(sqlite::Database *db);
	
	static Options query(sqlite::Database *db, sqlite3_int64 id);
	static std::vector<Options> queryAll(sqlite::Database *db);
	static std::vector<Options> queryAll(sqlite::Database *db, sqlite::Range<Time> currentTime = sqlite::ANY(), sqlite::Range<time_t> lastCheck = sqlite::ANY(), sqlite::Range<int> stopTimeMs = sqlite::ANY(), sqlite::Range<int> startTimeMs = sqlite::ANY(), sqlite::Range<int> idleDuringStartTimeMs = sqlite::ANY(), const TCHAR * orderBy = NULL);
	
	static void store(sqlite::Database *db, Options *obj);
	static void remove(sqlite::Database *db, Options *obj);
	
};


#endif // __OPTIONS_H__
