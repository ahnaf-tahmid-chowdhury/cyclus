#ifndef CYCLUS_SRC_SQLITE_BACK_H_
#define CYCLUS_SRC_SQLITE_BACK_H_

#include <string>
#include <map>
#include <set>

#include "query_backend.h"
#include "sqlite_db.h"

namespace cyclus {

/// An Recorder backend that writes data to an sqlite database.  Identically
/// named Datum objects have their data placed as rows in a single table.
/// Handles the following datum value types: int, float, double, std::string,
/// cyclus::Blob. Unsupported value types are stored as an empty string.
class SqliteBack : public FullBackend {
 public:
  /// Creates a new sqlite backend that will write to the database file
  /// specified by path. If the file doesn't exist, a new one is created.
  /// @param path the filepath (including name) to write the sqlite file.
  SqliteBack(std::string path);

  virtual ~SqliteBack();

  /// Writes Datum objects immediately to the database as a single transaction.
  /// @param data group of Datum objects to write to the database together.
  virtual void Notify(DatumList data);

  /// Returns a unique name for this backend.
  std::string Name();

  /// Executes all pending commands.
  void Flush();

  /// Closes the backend, if approriate.
  void Close() {};

  virtual QueryResult Query(std::string table, std::vector<Cond>* conds);

  virtual std::map<std::string, DbTypes> ColumnTypes(std::string table);

  virtual std::set<std::string> Tables();

  /// Returns the underlying sqlite database. Only use this if you really know
  /// what you are doing.
  SqliteDb& db();

 private:
  void Bind(boost::spirit::hold_any v, DbTypes type, SqlStatement::Ptr stmt,
            int index);

  QueryResult GetTableInfo(std::string table);

  std::list<ColumnInfo> Schema(std::string table);

  /// returns a valid sql data type name for v (e.g.  INTEGER, REAL, TEXT, etc).
  std::string SqlType(boost::spirit::hold_any v);

  /// returns a canonical string name for the type in v
  DbTypes Type(boost::spirit::hold_any v);

  /// converts the string value in s to a c++ value corresponding the the
  /// supported sqlite datatype type in a hold_any object.
  boost::spirit::hold_any ColAsVal(SqlStatement::Ptr stmt, int col,
                                   DbTypes type);

  /// Queue up a table-create command for d.
  void CreateTable(Datum* d);

  void BuildStmt(Datum* d);

  /// constructs an SQL INSERT command for d and queues it for db insertion.
  void WriteDatum(Datum* d);

  /// An interface to a sqlite db managed by the SqliteBack class.
  SqliteDb db_;

  /// Stores the database's path+name, declared during construction.
  std::string path_;

  /// table names already existing (created) in the sqlite db.
  std::set<std::string> tbl_names_;

  std::map<std::string, SqlStatement::Ptr> stmts_;
  std::map<std::string, std::vector<DbTypes>> schemas_;
};

}  // namespace cyclus

#endif  // CYCLUS_SRC_SQLITE_BACK_H_
