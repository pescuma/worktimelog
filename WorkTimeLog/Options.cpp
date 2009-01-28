#include "Options.h"


#define MAX_REGS(_X_) ( sizeof(_X_) / sizeof(_X_[0]) )



Options::Options(sqlite::Database *db, sqlite::Statement *stmt) : currentTime(db)
{
	this->db = db;

	stmt->getColumn(0, &id);
	sqlite3_int64 currentTimeID = stmt->getColumnAsInt64(1);
	if (currentTimeID > 0)
		currentTime = Time::query(db, currentTimeID);
	else
		currentTime.id = -1;
	lastCheck = (time_t) stmt->getColumnAsInt64(2);
	stopTimeMs = stmt->getColumnAsInt(3);
	startTimeMs = stmt->getColumnAsInt(4);
	idleDuringStartTimeMs = stmt->getColumnAsInt(5);
}


Options::Options(sqlite::Database *db) : currentTime(db)
{
	this->db = db;
	
	id = -1;
	lastCheck = 0;
	stopTimeMs = 0;
	startTimeMs = 0;
	idleDuringStartTimeMs = 0;
}


Options::~Options()
{
}


void Options::store()
{
	store(db, this);
}


void Options::remove()
{
	delete(db, this);
}



void Options::createTable(sqlite::Database *db)
{
	if (db == NULL)
		throw sqlite::DatabaseException(SQLITE_ERROR, _T("Invalid database"));

	db->execute(
		_T("CREATE TABLE IF NOT EXISTS Options (")
			_T("id INTEGER PRIMARY KEY, ")
			_T("currentTimeID INTEGER, ")
			_T("lastCheck INTEGER, ")
			_T("stopTimeMs INTEGER, ")
			_T("startTimeMs INTEGER, ")
			_T("idleDuringStartTimeMs INTEGER")
		_T(")")
	);
}


void Options::dropTable(sqlite::Database *db)
{
	if (db == NULL)
		throw sqlite::DatabaseException(SQLITE_ERROR, _T("Invalid database"));

	db->execute(_T("DROP TABLE IF EXISTS Options"));
}


Options Options::query(sqlite::Database *db, sqlite3_int64 id)
{
	if (db == NULL)
		throw sqlite::DatabaseException(SQLITE_ERROR, _T("Invalid database"));

	sqlite::Statement stmt = db->prepare(_T("SELECT * FROM Options WHERE id = ?"));

	stmt.bind(1, id);

	if (!stmt.step())
		throw sqlite::DatabaseException(SQLITE_NOTFOUND, _T("Options not found"));

	return Options(db, &stmt);
}	


std::vector<Options> Options::queryAll(sqlite::Database *db)
{
	if (db == NULL)
		throw sqlite::DatabaseException(SQLITE_ERROR, _T("Invalid database"));

	std::vector<Options> ret;

	sqlite::Statement stmt = db->prepare(_T("SELECT * FROM Options ORDER BY id ASC"));
	while (stmt.step())
		ret.push_back(Options(db, &stmt));

	return ret;
}


std::vector<Options> Options::queryAll(sqlite::Database *db, Time *pCurrentTime, time_t *pLastCheck, int *pStopTimeMs, int *pStartTimeMs, int *pIdleDuringStartTimeMs, const TCHAR *pOrderBy)
{
	if (db == NULL)
		throw sqlite::DatabaseException(SQLITE_ERROR, _T("Invalid database"));

	std::vector<Options> ret;
	
	std::tstring sql = _T("SELECT * FROM Options ");
	
	bool hasWhere = false;
	if (pCurrentTime != NULL)
	{
		sql += ( hasWhere ? _T("AND ") : _T("WHERE ") );
		sql += _T("currentTimeID == ? ");
		hasWhere = true;
	}
	if (pLastCheck != NULL)
	{
		sql += ( hasWhere ? _T("AND ") : _T("WHERE ") );
		sql += _T("lastCheck == ? ");
		hasWhere = true;
	}
	if (pStopTimeMs != NULL)
	{
		sql += ( hasWhere ? _T("AND ") : _T("WHERE ") );
		sql += _T("stopTimeMs == ? ");
		hasWhere = true;
	}
	if (pStartTimeMs != NULL)
	{
		sql += ( hasWhere ? _T("AND ") : _T("WHERE ") );
		sql += _T("startTimeMs == ? ");
		hasWhere = true;
	}
	if (pIdleDuringStartTimeMs != NULL)
	{
		sql += ( hasWhere ? _T("AND ") : _T("WHERE ") );
		sql += _T("idleDuringStartTimeMs == ? ");
		hasWhere = true;
	}
	
	sql += _T("ORDER BY ");
	if (pOrderBy == NULL)
		sql += _T("id ASC");
	else
		sql += pOrderBy;
	
	sqlite::Statement stmt = db->prepare(sql.c_str());
	
	int bind = 1;
	if (pCurrentTime != NULL)
		stmt.bind(bind++, pCurrentTime->id);
	if (pLastCheck != NULL)
		stmt.bind(bind++, (sqlite3_int64) *pLastCheck);
	if (pStopTimeMs != NULL)
		stmt.bind(bind++, *pStopTimeMs);
	if (pStartTimeMs != NULL)
		stmt.bind(bind++, *pStartTimeMs);
	if (pIdleDuringStartTimeMs != NULL)
		stmt.bind(bind++, *pIdleDuringStartTimeMs);
	
	while (stmt.step())
		ret.push_back(Options(db, &stmt));

	return ret;
}


void Options::store(sqlite::Database *db, Options *obj)
{
	if (db == NULL)
		throw sqlite::DatabaseException(SQLITE_ERROR, _T("Invalid database"));

	sqlite::Transaction trans(db);

	bool update = (obj->id > 0);
	sqlite3_int64 id;
	
	{
		sqlite::Statement stmt = db->prepare(_T("INSERT OR REPLACE INTO Options VALUES (?, ?, ?, ?, ?, ?)"));

		if (update)
			stmt.bind(1, obj->id);
		if (obj->currentTime.id > 0)
			stmt.bind(2, obj->currentTime.id);
		stmt.bind(3, (sqlite3_int64) obj->lastCheck);
		stmt.bind(4, obj->stopTimeMs);
		stmt.bind(5, obj->startTimeMs);
		stmt.bind(6, obj->idleDuringStartTimeMs);

		stmt.execute();

		id = db->getLastInsertRowID();
	}
	
	if (update && id != obj->id)
		throw sqlite::DatabaseException(SQLITE_ERROR, _T("Wrong id"));
	
	trans.commit();
	
	obj->id = id;
}


void Options::remove(sqlite::Database *db, Options *obj)
{
	if (db == NULL)
		throw sqlite::DatabaseException(SQLITE_ERROR, _T("Invalid database"));
	if (obj->id <= 0)
		throw sqlite::DatabaseException(SQLITE_ERROR, _T("Object is not in database"));
	
	sqlite::Transaction trans(db);
	
	{
		sqlite::Statement stmt = db->prepare(_T("DELETE FROM Options WHERE id == ?"));
		stmt.bind(1, obj->id);
		stmt.execute();
	}
	
	trans.commit();
	
	obj->id = -1;
}
