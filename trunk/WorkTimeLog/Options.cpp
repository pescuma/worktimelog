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
	autoTrack = (stmt->getColumnAsInt(3) != 0);
	stopTimeMs = stmt->getColumnAsInt(4);
	startTimeMs = stmt->getColumnAsInt(5);
	idleDuringStartTimeMs = stmt->getColumnAsInt(6);
	showBallons = (stmt->getColumnAsInt(7) != 0);
}


Options::Options(sqlite::Database *db) : currentTime(db)
{
	this->db = db;
	
	id = -1;
	lastCheck = 0;
	autoTrack = true;
	stopTimeMs = 600000;
	startTimeMs = 5000;
	idleDuringStartTimeMs = 1000;
	showBallons = true;
}


Options::~Options()
{
}


void Options::connectTo(sqlite::Database *db)
{
	this->db = db;
}


void Options::store()
{
	store(db, this);
}


void Options::remove()
{
	remove(db, this);
}


bool Options::isStored()
{
	return id > 0;
}


void Options::createTable(sqlite::Database *db)
{
	if (db == NULL)
		throw sqlite::DatabaseException(SQLITE_ERROR, _T("Invalid database"));
	
	sqlite::Transaction trans(db);
	
	std::tstring oldTable;
	{
		sqlite::Statement stmt = db->prepare(_T("SELECT sql FROM sqlite_master WHERE type == 'table' AND name = 'Options'"));
		if (stmt.step())
			stmt.getColumn(0, &oldTable);
	}
	
	if (oldTable.length() > 0)
	{
		bool rebuild = false;
		
		if (oldTable.find(_T(", currentTimeID ")) == (size_t) -1)
			rebuild = true;
		else if (oldTable.find(_T(", lastCheck ")) == (size_t) -1)
			rebuild = true;
		else if (oldTable.find(_T(", autoTrack ")) == (size_t) -1)
			rebuild = true;
		else if (oldTable.find(_T(", stopTimeMs ")) == (size_t) -1)
			rebuild = true;
		else if (oldTable.find(_T(", startTimeMs ")) == (size_t) -1)
			rebuild = true;
		else if (oldTable.find(_T(", idleDuringStartTimeMs ")) == (size_t) -1)
			rebuild = true;
		else if (oldTable.find(_T(", showBallons ")) == (size_t) -1)
			rebuild = true;
		
		if(rebuild)
		{
			db->execute(_T("ALTER TABLE Options RENAME TO TMP_OLD_Options"));
			
			db->execute(
				_T("CREATE TABLE Options (")
					_T("id INTEGER PRIMARY KEY, ")
					_T("currentTimeID INTEGER, ")
					_T("lastCheck INTEGER NOT NULL DEFAULT 0, ")
					_T("autoTrack VARCHAR NOT NULL DEFAULT 1, ")
					_T("stopTimeMs INTEGER NOT NULL DEFAULT 600000, ")
					_T("startTimeMs INTEGER NOT NULL DEFAULT 5000, ")
					_T("idleDuringStartTimeMs INTEGER NOT NULL DEFAULT 1000, ")
					_T("showBallons VARCHAR NOT NULL DEFAULT 1")
				_T(")")
			);
			
			std::tstring sql = _T("INSERT INTO Options(id");
			if (oldTable.find(_T(", currentTimeID ")) != (size_t) -1)
				sql += _T(", currentTimeID");
			if (oldTable.find(_T(", lastCheck ")) != (size_t) -1)
				sql += _T(", lastCheck");
			if (oldTable.find(_T(", autoTrack ")) != (size_t) -1)
				sql += _T(", autoTrack");
			if (oldTable.find(_T(", stopTimeMs ")) != (size_t) -1)
				sql += _T(", stopTimeMs");
			if (oldTable.find(_T(", startTimeMs ")) != (size_t) -1)
				sql += _T(", startTimeMs");
			if (oldTable.find(_T(", idleDuringStartTimeMs ")) != (size_t) -1)
				sql += _T(", idleDuringStartTimeMs");
			if (oldTable.find(_T(", showBallons ")) != (size_t) -1)
				sql += _T(", showBallons");
			sql += _T(") SELECT id");
			if (oldTable.find(_T(", currentTimeID ")) != (size_t) -1)
				sql += _T(", currentTimeID");
			if (oldTable.find(_T(", lastCheck ")) != (size_t) -1)
				sql += _T(", lastCheck");
			if (oldTable.find(_T(", autoTrack ")) != (size_t) -1)
				sql += _T(", autoTrack");
			if (oldTable.find(_T(", stopTimeMs ")) != (size_t) -1)
				sql += _T(", stopTimeMs");
			if (oldTable.find(_T(", startTimeMs ")) != (size_t) -1)
				sql += _T(", startTimeMs");
			if (oldTable.find(_T(", idleDuringStartTimeMs ")) != (size_t) -1)
				sql += _T(", idleDuringStartTimeMs");
			if (oldTable.find(_T(", showBallons ")) != (size_t) -1)
				sql += _T(", showBallons");
			sql += _T(" FROM TMP_OLD_Options");
			
			db->execute(sql.c_str());
			
			db->execute(_T("DROP TABLE TMP_OLD_Options"));
		}
	}
	else
	{
		db->execute(
			_T("CREATE TABLE IF NOT EXISTS Options (")
				_T("id INTEGER PRIMARY KEY, ")
				_T("currentTimeID INTEGER, ")
				_T("lastCheck INTEGER NOT NULL DEFAULT 0, ")
				_T("autoTrack VARCHAR NOT NULL DEFAULT 1, ")
				_T("stopTimeMs INTEGER NOT NULL DEFAULT 600000, ")
				_T("startTimeMs INTEGER NOT NULL DEFAULT 5000, ")
				_T("idleDuringStartTimeMs INTEGER NOT NULL DEFAULT 1000, ")
				_T("showBallons VARCHAR NOT NULL DEFAULT 1")
			_T(")")
		);
	}
	
	trans.commit();
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


std::vector<Options> Options::queryAll(sqlite::Database *db, sqlite::Range<Time> pCurrentTime, sqlite::Range<time_t> pLastCheck, sqlite::Range<bool> pAutoTrack, sqlite::Range<int> pStopTimeMs, sqlite::Range<int> pStartTimeMs, sqlite::Range<int> pIdleDuringStartTimeMs, sqlite::Range<bool> pShowBallons, const TCHAR *pOrderBy)
{
	if (db == NULL)
		throw sqlite::DatabaseException(SQLITE_ERROR, _T("Invalid database"));

	std::vector<Options> ret;
	
	std::tstring sql = _T("SELECT * FROM Options ");
	
	bool hasWhere = false;
	if (!pCurrentTime.isNull())
	{
		sql += ( hasWhere ? _T("AND ") : _T("WHERE ") );
		if (pCurrentTime.isSingleValue())
		{
			sql += _T("currentTimeID == ? ");
		}
		else
		{
			if (pCurrentTime.hasStart())
				sql += _T("currentTimeID >= ? ");
			if (pCurrentTime.hasStart() && pCurrentTime.hasEnd())
				sql += _T("AND ");
			if (pCurrentTime.hasEnd())
				sql += _T("currentTimeID < ? ");
		}
		hasWhere = true;
	}
	if (!pLastCheck.isNull())
	{
		sql += ( hasWhere ? _T("AND ") : _T("WHERE ") );
		if (pLastCheck.isSingleValue())
		{
			sql += _T("lastCheck == ? ");
		}
		else
		{
			if (pLastCheck.hasStart())
				sql += _T("lastCheck >= ? ");
			if (pLastCheck.hasStart() && pLastCheck.hasEnd())
				sql += _T("AND ");
			if (pLastCheck.hasEnd())
				sql += _T("lastCheck < ? ");
		}
		hasWhere = true;
	}
	if (!pAutoTrack.isNull())
	{
		sql += ( hasWhere ? _T("AND ") : _T("WHERE ") );
		if (pAutoTrack.isSingleValue())
		{
			sql += _T("autoTrack == ? ");
		}
		else
		{
			if (pAutoTrack.hasStart())
				sql += _T("autoTrack >= ? ");
			if (pAutoTrack.hasStart() && pAutoTrack.hasEnd())
				sql += _T("AND ");
			if (pAutoTrack.hasEnd())
				sql += _T("autoTrack < ? ");
		}
		hasWhere = true;
	}
	if (!pStopTimeMs.isNull())
	{
		sql += ( hasWhere ? _T("AND ") : _T("WHERE ") );
		if (pStopTimeMs.isSingleValue())
		{
			sql += _T("stopTimeMs == ? ");
		}
		else
		{
			if (pStopTimeMs.hasStart())
				sql += _T("stopTimeMs >= ? ");
			if (pStopTimeMs.hasStart() && pStopTimeMs.hasEnd())
				sql += _T("AND ");
			if (pStopTimeMs.hasEnd())
				sql += _T("stopTimeMs < ? ");
		}
		hasWhere = true;
	}
	if (!pStartTimeMs.isNull())
	{
		sql += ( hasWhere ? _T("AND ") : _T("WHERE ") );
		if (pStartTimeMs.isSingleValue())
		{
			sql += _T("startTimeMs == ? ");
		}
		else
		{
			if (pStartTimeMs.hasStart())
				sql += _T("startTimeMs >= ? ");
			if (pStartTimeMs.hasStart() && pStartTimeMs.hasEnd())
				sql += _T("AND ");
			if (pStartTimeMs.hasEnd())
				sql += _T("startTimeMs < ? ");
		}
		hasWhere = true;
	}
	if (!pIdleDuringStartTimeMs.isNull())
	{
		sql += ( hasWhere ? _T("AND ") : _T("WHERE ") );
		if (pIdleDuringStartTimeMs.isSingleValue())
		{
			sql += _T("idleDuringStartTimeMs == ? ");
		}
		else
		{
			if (pIdleDuringStartTimeMs.hasStart())
				sql += _T("idleDuringStartTimeMs >= ? ");
			if (pIdleDuringStartTimeMs.hasStart() && pIdleDuringStartTimeMs.hasEnd())
				sql += _T("AND ");
			if (pIdleDuringStartTimeMs.hasEnd())
				sql += _T("idleDuringStartTimeMs < ? ");
		}
		hasWhere = true;
	}
	if (!pShowBallons.isNull())
	{
		sql += ( hasWhere ? _T("AND ") : _T("WHERE ") );
		if (pShowBallons.isSingleValue())
		{
			sql += _T("showBallons == ? ");
		}
		else
		{
			if (pShowBallons.hasStart())
				sql += _T("showBallons >= ? ");
			if (pShowBallons.hasStart() && pShowBallons.hasEnd())
				sql += _T("AND ");
			if (pShowBallons.hasEnd())
				sql += _T("showBallons < ? ");
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
	if (!pCurrentTime.isNull())
	{
		if (pCurrentTime.hasStart())
			stmt.bind(bind++, pCurrentTime.start().id);
		if (!pCurrentTime.isSingleValue() && pCurrentTime.hasEnd())
			stmt.bind(bind++, pCurrentTime.end().id);
	}
	if (!pLastCheck.isNull())
	{
		if (pLastCheck.hasStart())
			stmt.bind(bind++, (sqlite3_int64) pLastCheck.start());
		if (!pLastCheck.isSingleValue() && pLastCheck.hasEnd())
			stmt.bind(bind++, (sqlite3_int64) pLastCheck.end());
	}
	if (!pAutoTrack.isNull())
	{
		if (pAutoTrack.hasStart())
			stmt.bind(bind++, pAutoTrack.start() ? 1 : 0);
		if (!pAutoTrack.isSingleValue() && pAutoTrack.hasEnd())
			stmt.bind(bind++, pAutoTrack.end() ? 1 : 0);
	}
	if (!pStopTimeMs.isNull())
	{
		if (pStopTimeMs.hasStart())
			stmt.bind(bind++, pStopTimeMs.start());
		if (!pStopTimeMs.isSingleValue() && pStopTimeMs.hasEnd())
			stmt.bind(bind++, pStopTimeMs.end());
	}
	if (!pStartTimeMs.isNull())
	{
		if (pStartTimeMs.hasStart())
			stmt.bind(bind++, pStartTimeMs.start());
		if (!pStartTimeMs.isSingleValue() && pStartTimeMs.hasEnd())
			stmt.bind(bind++, pStartTimeMs.end());
	}
	if (!pIdleDuringStartTimeMs.isNull())
	{
		if (pIdleDuringStartTimeMs.hasStart())
			stmt.bind(bind++, pIdleDuringStartTimeMs.start());
		if (!pIdleDuringStartTimeMs.isSingleValue() && pIdleDuringStartTimeMs.hasEnd())
			stmt.bind(bind++, pIdleDuringStartTimeMs.end());
	}
	if (!pShowBallons.isNull())
	{
		if (pShowBallons.hasStart())
			stmt.bind(bind++, pShowBallons.start() ? 1 : 0);
		if (!pShowBallons.isSingleValue() && pShowBallons.hasEnd())
			stmt.bind(bind++, pShowBallons.end() ? 1 : 0);
	}
	
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
		sqlite::Statement stmt = db->prepare(_T("INSERT OR REPLACE INTO Options VALUES (?, ?, ?, ?, ?, ?, ?, ?)"));

		if (update)
			stmt.bind(1, obj->id);
		if (obj->currentTime.id > 0)
			stmt.bind(2, obj->currentTime.id);
		stmt.bind(3, (sqlite3_int64) obj->lastCheck);
		stmt.bind(4, obj->autoTrack ? 1 : 0);
		stmt.bind(5, obj->stopTimeMs);
		stmt.bind(6, obj->startTimeMs);
		stmt.bind(7, obj->idleDuringStartTimeMs);
		stmt.bind(8, obj->showBallons ? 1 : 0);

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
