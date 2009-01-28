#include "Time.h"


#define MAX_REGS(_X_) ( sizeof(_X_) / sizeof(_X_[0]) )



Time::Time(sqlite::Database *db, sqlite::Statement *stmt) : task(db)
{
	this->db = db;

	stmt->getColumn(0, &id);
	sqlite3_int64 taskID = stmt->getColumnAsInt64(1);
	if (taskID > 0)
		task = Task::query(db, taskID);
	else
		task.id = -1;
	start = (time_t) stmt->getColumnAsInt64(2);
	end = (time_t) stmt->getColumnAsInt64(3);
}


Time::Time(sqlite::Database *db) : task(db)
{
	this->db = db;
	
	id = -1;
	start = 0;
	end = 0;
}


Time::~Time()
{
}


void Time::store()
{
	store(db, this);
}


void Time::remove()
{
	delete(db, this);
}



void Time::createTable(sqlite::Database *db)
{
	if (db == NULL)
		throw sqlite::DatabaseException(SQLITE_ERROR, _T("Invalid database"));

	db->execute(
		_T("CREATE TABLE IF NOT EXISTS Time (")
			_T("id INTEGER PRIMARY KEY, ")
			_T("taskID INTEGER, ")
			_T("start INTEGER, ")
			_T("end INTEGER")
		_T(")")
	);

	db->execute(_T("CREATE INDEX IF NOT EXISTS INDEX_Time_start ON Time(start)"));

	db->execute(_T("CREATE INDEX IF NOT EXISTS INDEX_Time_end ON Time(end)"));
}


void Time::dropTable(sqlite::Database *db)
{
	if (db == NULL)
		throw sqlite::DatabaseException(SQLITE_ERROR, _T("Invalid database"));

	db->execute(_T("DROP INDEX IF EXISTS INDEX_Time_start"));

	db->execute(_T("DROP INDEX IF EXISTS INDEX_Time_end"));

	db->execute(_T("DROP TABLE IF EXISTS Time"));
}


Time Time::query(sqlite::Database *db, sqlite3_int64 id)
{
	if (db == NULL)
		throw sqlite::DatabaseException(SQLITE_ERROR, _T("Invalid database"));

	sqlite::Statement stmt = db->prepare(_T("SELECT * FROM Time WHERE id = ?"));

	stmt.bind(1, id);

	if (!stmt.step())
		throw sqlite::DatabaseException(SQLITE_NOTFOUND, _T("Time not found"));

	return Time(db, &stmt);
}	


std::vector<Time> Time::queryAll(sqlite::Database *db)
{
	if (db == NULL)
		throw sqlite::DatabaseException(SQLITE_ERROR, _T("Invalid database"));

	std::vector<Time> ret;

	sqlite::Statement stmt = db->prepare(_T("SELECT * FROM Time ORDER BY id ASC"));
	while (stmt.step())
		ret.push_back(Time(db, &stmt));

	return ret;
}


std::vector<Time> Time::queryAll(sqlite::Database *db, Task *pTask, time_t *pStart, time_t *pEnd, const TCHAR *pOrderBy)
{
	if (db == NULL)
		throw sqlite::DatabaseException(SQLITE_ERROR, _T("Invalid database"));

	std::vector<Time> ret;
	
	std::tstring sql = _T("SELECT * FROM Time ");
	
	bool hasWhere = false;
	if (pTask != NULL)
	{
		sql += ( hasWhere ? _T("AND ") : _T("WHERE ") );
		sql += _T("taskID == ? ");
		hasWhere = true;
	}
	if (pStart != NULL)
	{
		sql += ( hasWhere ? _T("AND ") : _T("WHERE ") );
		sql += _T("start == ? ");
		hasWhere = true;
	}
	if (pEnd != NULL)
	{
		sql += ( hasWhere ? _T("AND ") : _T("WHERE ") );
		sql += _T("end == ? ");
		hasWhere = true;
	}
	
	sql += _T("ORDER BY ");
	if (pOrderBy == NULL)
		sql += _T("id ASC");
	else
		sql += pOrderBy;
	
	sqlite::Statement stmt = db->prepare(sql.c_str());
	
	int bind = 1;
	if (pTask != NULL)
		stmt.bind(bind++, pTask->id);
	if (pStart != NULL)
		stmt.bind(bind++, (sqlite3_int64) *pStart);
	if (pEnd != NULL)
		stmt.bind(bind++, (sqlite3_int64) *pEnd);
	
	while (stmt.step())
		ret.push_back(Time(db, &stmt));

	return ret;
}


void Time::store(sqlite::Database *db, Time *obj)
{
	if (db == NULL)
		throw sqlite::DatabaseException(SQLITE_ERROR, _T("Invalid database"));

	sqlite::Transaction trans(db);

	bool update = (obj->id > 0);
	sqlite3_int64 id;
	
	{
		sqlite::Statement stmt = db->prepare(_T("INSERT OR REPLACE INTO Time VALUES (?, ?, ?, ?)"));

		if (update)
			stmt.bind(1, obj->id);
		if (obj->task.id > 0)
			stmt.bind(2, obj->task.id);
		stmt.bind(3, (sqlite3_int64) obj->start);
		stmt.bind(4, (sqlite3_int64) obj->end);

		stmt.execute();

		id = db->getLastInsertRowID();
	}
	
	if (update && id != obj->id)
		throw sqlite::DatabaseException(SQLITE_ERROR, _T("Wrong id"));
	
	trans.commit();
	
	obj->id = id;
}


void Time::remove(sqlite::Database *db, Time *obj)
{
	if (db == NULL)
		throw sqlite::DatabaseException(SQLITE_ERROR, _T("Invalid database"));
	if (obj->id <= 0)
		throw sqlite::DatabaseException(SQLITE_ERROR, _T("Object is not in database"));
	
	sqlite::Transaction trans(db);
	
	{
		sqlite::Statement stmt = db->prepare(_T("DELETE FROM Time WHERE id == ?"));
		stmt.bind(1, obj->id);
		stmt.execute();
	}
	
	trans.commit();
	
	obj->id = -1;
}