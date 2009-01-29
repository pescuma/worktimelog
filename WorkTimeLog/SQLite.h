#ifndef __SQLITE_H__
# define __SQLITE_H__

#include <windows.h>
#include <sqlite3.h>
#include <vector>
#include <string>
namespace std {
typedef basic_string<TCHAR, char_traits<TCHAR>, allocator<TCHAR> > tstring;
}


namespace sqlite {

	class Statement;

	class NullRange {};

	template<class T>
	class Range
	{
	public:
		Range() : _start(NULL), _end(NULL), onlyOne(true) {}
		Range(NullRange &dummy) : _start(NULL), _end(NULL), onlyOne(true) {}
		Range(const T &value) : _start(&value), _end(&value), onlyOne(true) {}
		Range(const T *value) : _start(value), _end(value), onlyOne(true) {}
		Range(const T &start, const T &end) : _start(&start), _end(&end), onlyOne(false) {}
		Range(const T *start, const T *end) : _start(start), _end(end), onlyOne(false) {}
		Range(const T &start, const T *end) : _start(&start), _end(end), onlyOne(false) {}
		Range(const T *start, const T &end) : _start(start), _end(&end), onlyOne(false) {}

		bool hasStart() { return _start != NULL; }
		bool hasEnd() { return _end != NULL; }
		bool isSingleValue() { return onlyOne; }
		bool isNull() { return !hasStart() && !hasEnd(); }

		const T & start() { return *_start; }
		const T & end() { return *_end; }

	private:
		const T *_start;
		const T *_end;
		bool onlyOne;
	};

	static inline NullRange ANY() { return NullRange(); }


	class Database
	{
	public:
		Database();
		~Database();

		void open(const TCHAR *filename);
		void close();

		int get(const TCHAR *sql);
		std::vector<int> getAll(const TCHAR *sql);
		void execute(const TCHAR *sql);
		void execute(const TCHAR *sql, int (*callback)(void*,int,char**,char**), void *param);
		Statement prepare(const TCHAR *sql);

		sqlite3_int64 getLastInsertRowID();

		bool isAutoCommitEnabled();


	private:
		sqlite3 *db;

		void checkError(int err);
	};



	class Statement
	{
	public:
		~Statement();

		void reset();
			
		void bind(int index, int value); 
		void bind(const TCHAR *name, int value); 
		void bind(int index, sqlite3_int64 value); 
		void bind(const TCHAR *name, sqlite3_int64 value); 
		void bind(int index, double value);
		void bind(const TCHAR *name, double value);
		void bind(int index, const TCHAR *value);
		void bind(const TCHAR *name, const TCHAR *value);
		void bind(int index, void *value, int bytes);
		void bind(const TCHAR *name, void *value, int bytes);

		/// @return true if has more lines to return
		bool step();
		void execute();

		int getColumnCount();
		int getColumnType(int column);

		void getColumn(int column, int *ret);
		void getColumn(int column, sqlite3_int64 *ret);
		void getColumn(int column, double *ret);
		void getColumn(int column, TCHAR *ret, size_t size);
		void getColumn(int column, TCHAR **ret); /// The caller have to free the returned string in ret
		void getColumn(int column, std::tstring *ret);
		void getColumn(int column, void *ret, size_t size);
		void getColumn(int column, void **ret, size_t *size); /// The caller have to free the returned pointer in ret

		// Helpers
		int getColumnAsInt(int column);
		sqlite3_int64 getColumnAsInt64(int column);
		double getColumnAsDouble(int column);
		TCHAR *getColumnAsString(int column); /// The caller have to free the returned string
		

	private:
		Statement(sqlite3 *db, sqlite3_stmt *stmt);

		sqlite3 *db;
		sqlite3_stmt *stmt;

		void checkError(int err);
		void checkError();

		friend Database;
	};


	class Transaction
	{
	private:
		Database *db;
		bool finished;
		bool valid;

	public:
		Transaction(Database *db);
		~Transaction();

		void commit();
		void rollback();
	};


	class DatabaseException
	{
	private:
		bool haveToFree;

	public:
		int code;
		TCHAR *message;

		DatabaseException(int code, TCHAR *message);
		DatabaseException(sqlite3 *db, int code);
		DatabaseException(const DatabaseException &e);
		~DatabaseException();
	};
}


#endif // __SQLITE2_H__