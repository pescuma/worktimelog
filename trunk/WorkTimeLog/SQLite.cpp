#include <windows.h>
#include <tchar.h>
#include "Sqlite.h"
#include "scope.h"


//#pragma comment(lib, "sqlite3.lib")


namespace sqlite {

	static bool SQLITE_SUCCESS(int e) 
	{
		return e == SQLITE_OK || e == SQLITE_DONE || e == SQLITE_ROW;
	}


	class TcharToUtf8
	{
	public:
		TcharToUtf8(const char *str) : utf8(NULL)
		{
			int size = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
			if (size <= 0)
				throw DatabaseException(SQLITE_NOMEM, _T("Could not convert string to WCHAR"));

			scope<WCHAR *> tmp = (WCHAR *) malloc(size * sizeof(WCHAR));
			if (tmp == NULL)
				throw DatabaseException(SQLITE_NOMEM, _T("malloc returned NULL"));

			MultiByteToWideChar(CP_ACP, 0, str, -1, tmp, size);

			init(tmp);
		}


		TcharToUtf8(const WCHAR *str) : utf8(NULL)
		{
			init(str);
		}


		~TcharToUtf8()
		{
			if (utf8 != NULL)
				free(utf8);
		}

		operator char *()
		{
			return utf8;
		}

	private:
		char *utf8;

		void init(const WCHAR *str)
		{
			int size = WideCharToMultiByte(CP_UTF8, 0, str, -1, NULL, 0, NULL, NULL);
			if (size <= 0)
				throw DatabaseException(SQLITE_NOMEM, _T("Could not convert string to UTF8"));

			utf8 = (char *) malloc(size);
			if (utf8 == NULL)
				throw DatabaseException(SQLITE_NOMEM, _T("malloc returned NULL"));

			WideCharToMultiByte(CP_UTF8, 0, str, -1, utf8, size, NULL, NULL);
		}
	};



	class Utf8ToTchar
	{
	public:
		Utf8ToTchar(const char *str) : tchar(NULL)
		{
			if (str == NULL)
				return;

			int size = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
			if (size <= 0)
				throw DatabaseException(SQLITE_NOMEM, _T("Could not convert string to WCHAR"));

			scope<WCHAR *> tmp = (WCHAR *) malloc(size * sizeof(WCHAR));
			if (tmp == NULL)
				throw DatabaseException(SQLITE_NOMEM, _T("malloc returned NULL"));

			MultiByteToWideChar(CP_UTF8, 0, str, -1, tmp, size);

#ifdef UNICODE

			tchar = tmp.detach();

#else

			size = WideCharToMultiByte(CP_ACP, 0, tmp, -1, NULL, 0, NULL, NULL);
			if (size <= 0)
				throw DatabaseException(SQLITE_NOMEM, _T("Could not convert string to ACP"));

			tchar = (TCHAR *) malloc(size);
			if (tchar == NULL)
				throw DatabaseException(SQLITE_NOMEM, _T("malloc returned NULL"));

			WideCharToMultiByte(CP_ACP, 0, tmp, -1, tchar, size, NULL, NULL);

#endif
		}


		~Utf8ToTchar()
		{
			if (tchar != NULL)
				free(tchar);
		}

		TCHAR *detach()
		{
			TCHAR *ret = tchar;
			tchar = NULL;
			return ret;
		}

		operator TCHAR *()
		{
			return tchar;
		}

	private:
		TCHAR *tchar;
	};



	/////////////////////////////////////////////////////////////////////////////////////////



	Database::Database() : db(NULL)
	{
	}


	Database::~Database()
	{
		close();
	}


	void Database::checkError(int err)
	{
		if (!SQLITE_SUCCESS(err))
			throw DatabaseException(db, err);
	}

	void Database::open(const TCHAR *filename)
	{
		close();

		int ret = sqlite3_open(TcharToUtf8(filename), &db);

		if (ret != SQLITE_OK)
		{
			if (db != NULL)
			{
				sqlite3_close(db);
				db = NULL;
			}

			throw DatabaseException(db, ret);
		}
	}


	void Database::close()
	{
		if (db == NULL)
			return;
		
		int ret = sqlite3_close(db);
		db = NULL;

		checkError(ret);
	}


	static int copyFirstFromFirstColumn(void *param, int numCols, char **values, char **columns)
	{
		int *ret = (int *)param;

		if (numCols <= 0)
			return 0;

		if (values[0] == 0)
			*ret = 0;
		else
			*ret = atoi(values[0]);

		return 0;
	}

	int Database::get(const TCHAR *sql)
	{
		int ret = 0;
		execute(sql, copyFirstFromFirstColumn, &ret);
		return ret;
	}


	static int copyAllFromFirstColumn(void *param, int numCols, char **values, char **columns)
	{
		std::vector<int> *ret = (std::vector<int> *) param;

		if (numCols <= 0)
			return 0;

		if (values[0] == 0)
			ret->push_back(0);
		else
			ret->push_back(atoi(values[0]));

		return 0;
	}

	std::vector<int> Database::getAll(const TCHAR *sql)
	{
		std::vector<int> ret;
		execute(sql, copyAllFromFirstColumn, &ret);
		return ret;
	}


	void Database::execute(const TCHAR *sql)
	{
		execute(sql, NULL, NULL);
	}


	void Database::execute(const TCHAR *sql, int (*callback)(void*,int,char**,char**), void *param)
	{
		checkError( sqlite3_exec(db, TcharToUtf8(sql), callback, param, NULL) );
	}



	Statement Database::prepare(const TCHAR *sql)
	{
		sqlite3_stmt *stmt = NULL;
		const char *tail = NULL;

		int ret = sqlite3_prepare_v2(db, TcharToUtf8(sql), -1, &stmt, &tail);

		if (ret != SQLITE_OK)
		{
			if (stmt != NULL)
				sqlite3_finalize(stmt);

			throw DatabaseException(db, ret);
		}

		return Statement(db, stmt);
	}


	sqlite3_int64 Database::getLastInsertRowID()
	{
		return sqlite3_last_insert_rowid(db);
	}


	bool Database::isAutoCommitEnabled()
	{
		return sqlite3_get_autocommit(db) != 0;
	}



	/////////////////////////////////////////////////////////////////////////////////////////



	Statement::Statement(sqlite3 *aDb, sqlite3_stmt *aStmt) : db(aDb), stmt(aStmt)
	{
	}


	Statement::~Statement()
	{
		sqlite3_finalize(stmt);
	}

	
	void Statement::checkError(int err)
	{
		if (!SQLITE_SUCCESS(err))
			throw DatabaseException(db, err);
	}

	
	void Statement::checkError()
	{
		checkError( sqlite3_errcode(db) );
	}


	void Statement::reset()
	{
		checkError( sqlite3_reset(stmt) );
	}


	void Statement::bind(int index, int value)
	{
		checkError( sqlite3_bind_int(stmt, index, value) );
	}


	void Statement::bind(const TCHAR *name, int value)
	{
		int index = sqlite3_bind_parameter_index(stmt, TcharToUtf8(name));
		if (index <= 0)
			throw DatabaseException(SQLITE_NOTFOUND, _T("Parameter name not found"));

		bind(index, value);
	}


	void Statement::bind(int index, sqlite3_int64 value)
	{
		checkError( sqlite3_bind_int64(stmt, index, value) );
	}


	void Statement::bind(const TCHAR *name, sqlite3_int64 value)
	{
		int index = sqlite3_bind_parameter_index(stmt, TcharToUtf8(name));
		if (index <= 0)
			throw DatabaseException(SQLITE_NOTFOUND, _T("Parameter name not found"));

		bind(index, value);
	}


	void Statement::bind(int index, double value)
	{
		checkError( sqlite3_bind_double(stmt, index, value) );
	}


	void Statement::bind(const TCHAR *name, double value)
	{
		int index = sqlite3_bind_parameter_index(stmt, TcharToUtf8(name));
		if (index <= 0)
			throw DatabaseException(SQLITE_NOTFOUND, _T("Parameter name not found"));

		bind(index, value);
	}


	void Statement::bind(int index, const TCHAR *value)
	{
		if (value == NULL)
			checkError( sqlite3_bind_null(stmt, index) );
		else
			checkError( sqlite3_bind_text(stmt, index, TcharToUtf8(value), -1, SQLITE_TRANSIENT) );
	}


	void Statement::bind(const TCHAR *name, const TCHAR *value)
	{
		int index = sqlite3_bind_parameter_index(stmt, TcharToUtf8(name));
		if (index <= 0)
			throw DatabaseException(SQLITE_NOTFOUND, _T("Parameter name not found"));

		bind(index, value);
	}


	void Statement::bind(int index, void *value, int bytes)
	{
		if (value == NULL)
			checkError( sqlite3_bind_zeroblob(stmt, index, bytes) );
		else
			checkError( sqlite3_bind_blob(stmt, index, value, bytes, SQLITE_TRANSIENT) );
	}


	void Statement::bind(const TCHAR *name, void *value, int bytes)
	{
		int index = sqlite3_bind_parameter_index(stmt, TcharToUtf8(name));
		if (index <= 0)
			throw DatabaseException(SQLITE_NOTFOUND, _T("Parameter name not found"));

		bind(index, value, bytes);
	}


	bool Statement::step()
	{
		int ret = sqlite3_step(stmt);
		checkError(ret);
		return ret == SQLITE_ROW;
	}


	void Statement::execute()
	{
		int ret = sqlite3_step(stmt);
		checkError(ret);
	}


	int Statement::getColumnCount()
	{
		return sqlite3_column_count(stmt);
	}


	int Statement::getColumnType(int column)
	{
		int ret = sqlite3_column_type(stmt, column);
		if (ret == 0)
			checkError();
		return ret;
	}


	int Statement::getColumnAsInt(int column)
	{
		int ret;
		getColumn(column, &ret);
		return ret;
	}


	sqlite3_int64 Statement::getColumnAsInt64(int column)
	{
		sqlite3_int64 ret;
		getColumn(column, &ret);
		return ret;
	}


	double Statement::getColumnAsDouble(int column)
	{
		double ret;
		getColumn(column, &ret);
		return ret;
	}


	TCHAR *Statement::getColumnAsString(int column)
	{
		TCHAR *ret;
		getColumn(column, &ret);
		return ret;
	}


	void Statement::getColumn(int column, int *ret)
	{
		*ret = sqlite3_column_int(stmt, column);
		if (*ret == 0)
			checkError();
	}


	void Statement::getColumn(int column, sqlite3_int64 *ret)
	{
		*ret = sqlite3_column_int64(stmt, column);
		if (*ret == 0)
			checkError();
	}


	void Statement::getColumn(int column, double *ret)
	{
		*ret = sqlite3_column_double(stmt, column);
		if (*ret == 0.0)
			checkError();
	}


	void Statement::getColumn(int column, TCHAR *ret, size_t size)
	{
		const char *txt = (const char *) sqlite3_column_text(stmt, column);
		if (txt == NULL)
		{
			checkError();
			lstrcpyn(ret, _T(""), size);
			return;
		}

		lstrcpyn(ret, Utf8ToTchar(txt), size);
	}


	void Statement::getColumn(int column, TCHAR **ret)
	{
		const char *txt = (const char *) sqlite3_column_text(stmt, column);
		if (txt == NULL)
		{
			checkError();
			*ret = NULL;
			return;
		}

		*ret = Utf8ToTchar(txt).detach();
	}


	void Statement::getColumn(int column, std::tstring *ret)
	{
		const char *txt = (const char *) sqlite3_column_text(stmt, column);
		if (txt == NULL)
		{
			checkError();
			*ret = _T("");
			return;
		}

		*ret = Utf8ToTchar(txt);
	}


	void Statement::getColumn(int column, void *ret, size_t size)
	{
		const void *blob = sqlite3_column_blob(stmt, column);
		if (blob == NULL)
		{
			checkError();
			memset(ret, 0, size);
			return;
		}

		int blobSize = sqlite3_column_bytes(stmt, column);
		if (blobSize <= 0)
		{
			checkError();
			memset(ret, 0, size);
			return;
		}
		
		memcpy(ret, blob, min(size, (size_t) blobSize));
		if (size > (size_t) blobSize)
			memset(&((char *)ret)[blobSize], 0, size - blobSize);
	}


	void Statement::getColumn(int column, void **ret, size_t *size)
	{
		const void *blob = sqlite3_column_blob(stmt, column);
		if (blob == NULL)
		{
			checkError();
			*ret = NULL;
			*size = 0;
			return;
		}

		*size = sqlite3_column_bytes(stmt, column);
		if (*size == 0)
		{
			checkError();
			*ret = NULL;
			return;
		}

		*ret = malloc(*size);
		if (*ret == NULL)
			throw DatabaseException(SQLITE_NOMEM, _T("malloc returned NULL"));

		memcpy(*ret, blob, *size);
	}


	
	/////////////////////////////////////////////////////////////////////////////////////



	Transaction::Transaction(Database *db)
	{
		this->db = db;
		finished = false;
		valid = db->isAutoCommitEnabled();
		
		if (valid)
			db->execute(_T("BEGIN TRANSACTION"));
	}


	Transaction::~Transaction()
	{
		if (valid && !finished)
			rollback();
	}


	void Transaction::commit()
	{
		finished = true;

		if (valid)
			db->execute(_T("COMMIT"));
	}


	void Transaction::rollback()
	{
		finished = true;

		if (valid)
			db->execute(_T("ROLLBACK"));
	}


	
	/////////////////////////////////////////////////////////////////////////////////////



	DatabaseException::DatabaseException(int code, TCHAR *message)
	{
		this->code = code;
		this->message = message;
		this->haveToFree = false;
	}

	DatabaseException::DatabaseException(sqlite3 *db, int code)
	{
		this->code = code;
		this->message = Utf8ToTchar(sqlite3_errmsg(db)).detach();
		this->haveToFree = true;
	}

	DatabaseException::DatabaseException(const DatabaseException &e)
	{
		code = e.code;
		if (e.haveToFree)
			message = _tcsdup(e.message);
		else
			message = e.message;
		haveToFree = e.haveToFree;
	}

	DatabaseException::~DatabaseException()
	{
		if (haveToFree && message != NULL)
			free(message);
	}


}