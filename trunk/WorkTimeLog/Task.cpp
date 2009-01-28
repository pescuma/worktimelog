#include "Task.h"


#define MAX_REGS(_X_) ( sizeof(_X_) / sizeof(_X_[0]) )



Task::Task(sqlite::Database *db, sqlite::Statement *stmt)
{
	this->db = db;

	stmt->getColumn(0, &id);
	stmt->getColumn(1, &name);
}


Task::Task(sqlite::Database *db)
{
	this->db = db;
	
	id = -1;
	name = _T("");
}


Task::~Task()
{
}


void Task::store()
{
	store(db, this);
}


void Task::remove()
{
	delete(db, this);
}



void Task::createTable(sqlite::Database *db)
{
	if (db == NULL)
		throw sqlite::DatabaseException(SQLITE_ERROR, _T("Invalid database"));

	db->execute(
		_T("CREATE TABLE IF NOT EXISTS Task (")
			_T("id INTEGER PRIMARY KEY, ")
			_T("name VARCHAR")
		_T(")")
	);
}


void Task::dropTable(sqlite::Database *db)
{
	if (db == NULL)
		throw sqlite::DatabaseException(SQLITE_ERROR, _T("Invalid database"));

	db->execute(_T("DROP TABLE IF EXISTS Task"));
}


Task Task::query(sqlite::Database *db, sqlite3_int64 id)
{
	if (db == NULL)
		throw sqlite::DatabaseException(SQLITE_ERROR, _T("Invalid database"));

	sqlite::Statement stmt = db->prepare(_T("SELECT * FROM Task WHERE id = ?"));

	stmt.bind(1, id);

	if (!stmt.step())
		throw sqlite::DatabaseException(SQLITE_NOTFOUND, _T("Task not found"));

	return Task(db, &stmt);
}	


std::vector<Task> Task::queryAll(sqlite::Database *db)
{
	if (db == NULL)
		throw sqlite::DatabaseException(SQLITE_ERROR, _T("Invalid database"));

	std::vector<Task> ret;

	sqlite::Statement stmt = db->prepare(_T("SELECT * FROM Task ORDER BY id ASC"));
	while (stmt.step())
		ret.push_back(Task(db, &stmt));

	return ret;
}


std::vector<Task> Task::queryAll(sqlite::Database *db, std::tstring *pName, const TCHAR *pOrderBy)
{
	if (db == NULL)
		throw sqlite::DatabaseException(SQLITE_ERROR, _T("Invalid database"));

	std::vector<Task> ret;
	
	std::tstring sql = _T("SELECT * FROM Task ");
	
	bool hasWhere = false;
	if (pName != NULL)
	{
		sql += ( hasWhere ? _T("AND ") : _T("WHERE ") );
		sql += _T("name == ? ");
		hasWhere = true;
	}
	
	sql += _T("ORDER BY ");
	if (pOrderBy == NULL)
		sql += _T("id ASC");
	else
		sql += pOrderBy;
	
	sqlite::Statement stmt = db->prepare(sql.c_str());
	
	int bind = 1;
	if (pName != NULL)
		stmt.bind(bind++, pName->c_str());
	
	while (stmt.step())
		ret.push_back(Task(db, &stmt));

	return ret;
}


void Task::store(sqlite::Database *db, Task *obj)
{
	if (db == NULL)
		throw sqlite::DatabaseException(SQLITE_ERROR, _T("Invalid database"));

	sqlite::Transaction trans(db);

	bool update = (obj->id > 0);
	sqlite3_int64 id;
	
	{
		sqlite::Statement stmt = db->prepare(_T("INSERT OR REPLACE INTO Task VALUES (?, ?)"));

		if (update)
			stmt.bind(1, obj->id);
		stmt.bind(2, obj->name.c_str());

		stmt.execute();

		id = db->getLastInsertRowID();
	}
	
	if (update && id != obj->id)
		throw sqlite::DatabaseException(SQLITE_ERROR, _T("Wrong id"));
	
	trans.commit();
	
	obj->id = id;
}


void Task::remove(sqlite::Database *db, Task *obj)
{
	if (db == NULL)
		throw sqlite::DatabaseException(SQLITE_ERROR, _T("Invalid database"));
	if (obj->id <= 0)
		throw sqlite::DatabaseException(SQLITE_ERROR, _T("Object is not in database"));
	
	sqlite::Transaction trans(db);
	
	{
		sqlite::Statement stmt = db->prepare(_T("DELETE FROM Task WHERE id == ?"));
		stmt.bind(1, obj->id);
		stmt.execute();
	}
	
	trans.commit();
	
	obj->id = -1;
}
