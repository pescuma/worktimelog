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


void Time::connectTo(sqlite::Database *db)
{
	this->db = db;
}


void Time::store()
{
	store(db, this);
}


void Time::remove()
{
	remove(db, this);
}


bool Time::isStored()
{
	return id > 0;
}


void Time::createTable(sqlite::Database *db)
{
	if (db == NULL)
		throw sqlite::DatabaseException(SQLITE_ERROR, _T("Invalid database"));
	
	sqlite::Transaction trans(db);
	
	std::tstring oldTable;
	{
		sqlite::Statement stmt = db->prepare(_T("SELECT sql FROM sqlite_master WHERE type == 'table' AND name = 'Time'"));
		if (stmt.step())
			stmt.getColumn(0, &oldTable);
	}
	
	if (oldTable.length() > 0)
	{
		bool rebuild = false;
		
		if (oldTable.find(_T(", taskID ")) == (size_t) -1)
			rebuild = true;
		else if (oldTable.find(_T(", start ")) == (size_t) -1)
			rebuild = true;
		else if (oldTable.find(_T(", end ")) == (size_t) -1)
			rebuild = true;
		
		if(rebuild)
		{
			db->execute(_T("ALTER TABLE Time RENAME TO TMP_OLD_Time"));
			
			db->execute(
				_T("CREATE TABLE Time (")
					_T("id INTEGER PRIMARY KEY, ")
					_T("taskID INTEGER, ")
					_T("start INTEGER NOT NULL DEFAULT 0, ")
					_T("end INTEGER NOT NULL DEFAULT 0")
				_T(")")
			);
			
			std::tstring sql = _T("INSERT INTO Time(id");
			if (oldTable.find(_T(", taskID ")) != (size_t) -1)
				sql += _T(", taskID");
			if (oldTable.find(_T(", start ")) != (size_t) -1)
				sql += _T(", start");
			if (oldTable.find(_T(", end ")) != (size_t) -1)
				sql += _T(", end");
			sql += _T(") SELECT id");
			if (oldTable.find(_T(", taskID ")) != (size_t) -1)
				sql += _T(", taskID");
			if (oldTable.find(_T(", start ")) != (size_t) -1)
				sql += _T(", start");
			if (oldTable.find(_T(", end ")) != (size_t) -1)
				sql += _T(", end");
			sql += _T(" FROM TMP_OLD_Time");
			
			db->execute(sql.c_str());
			
			db->execute(_T("DROP TABLE TMP_OLD_Time"));
		}
	}
	else
	{
		db->execute(
			_T("CREATE TABLE IF NOT EXISTS Time (")
				_T("id INTEGER PRIMARY KEY, ")
				_T("taskID INTEGER, ")
				_T("start INTEGER NOT NULL DEFAULT 0, ")
				_T("end INTEGER NOT NULL DEFAULT 0")
			_T(")")
		);
	}

	db->execute(_T("CREATE INDEX IF NOT EXISTS INDEX_Time_start ON Time(start)"));

	db->execute(_T("CREATE INDEX IF NOT EXISTS INDEX_Time_end ON Time(end)"));
	
	trans.commit();
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


std::vector<Time> Time::queryAll(sqlite::Database *db, sqlite::Range<Task> pTask, sqlite::Range<time_t> pStart, sqlite::Range<time_t> pEnd, const TCHAR *pOrderBy)
{
	if (db == NULL)
		throw sqlite::DatabaseException(SQLITE_ERROR, _T("Invalid database"));

	std::vector<Time> ret;
	
	std::tstring sql = _T("SELECT * FROM Time ");
	
	bool hasWhere = false;
	if (!pTask.isNull())
	{
		sql += ( hasWhere ? _T("AND ") : _T("WHERE ") );
		if (pTask.isSingleValue())
		{
			sql += _T("taskID == ? ");
		}
		else
		{
			if (pTask.hasStart())
				sql += _T("taskID >= ? ");
			if (pTask.hasStart() && pTask.hasEnd())
				sql += _T("AND ");
			if (pTask.hasEnd())
				sql += _T("taskID < ? ");
		}
		hasWhere = true;
	}
	if (!pStart.isNull())
	{
		sql += ( hasWhere ? _T("AND ") : _T("WHERE ") );
		if (pStart.isSingleValue())
		{
			sql += _T("start == ? ");
		}
		else
		{
			if (pStart.hasStart())
				sql += _T("start >= ? ");
			if (pStart.hasStart() && pStart.hasEnd())
				sql += _T("AND ");
			if (pStart.hasEnd())
				sql += _T("start < ? ");
		}
		hasWhere = true;
	}
	if (!pEnd.isNull())
	{
		sql += ( hasWhere ? _T("AND ") : _T("WHERE ") );
		if (pEnd.isSingleValue())
		{
			sql += _T("end == ? ");
		}
		else
		{
			if (pEnd.hasStart())
				sql += _T("end >= ? ");
			if (pEnd.hasStart() && pEnd.hasEnd())
				sql += _T("AND ");
			if (pEnd.hasEnd())
				sql += _T("end < ? ");
		}
		hasWhere = true;
	}
	
	sql += _T("ORDER BY ");
	if (pOrderBy == NULL)
		sql += _T("id ASC");
	else
		sql += pOrderBy;
	
	sqlite::Statement stmt = db->prepare(sql.c_str());
	
	int bind = 1;
	if (!pTask.isNull())
	{
		if (pTask.hasStart())
			stmt.bind(bind++, pTask.start().id);
		if (!pTask.isSingleValue() && pTask.hasEnd())
			stmt.bind(bind++, pTask.end().id);
	}
	if (!pStart.isNull())
	{
		if (pStart.hasStart())
			stmt.bind(bind++, (sqlite3_int64) pStart.start());
		if (!pStart.isSingleValue() && pStart.hasEnd())
			stmt.bind(bind++, (sqlite3_int64) pStart.end());
	}
	if (!pEnd.isNull())
	{
		if (pEnd.hasStart())
			stmt.bind(bind++, (sqlite3_int64) pEnd.start());
		if (!pEnd.isSingleValue() && pEnd.hasEnd())
			stmt.bind(bind++, (sqlite3_int64) pEnd.end());
	}
	
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
