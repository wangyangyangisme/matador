//
// Created by sascha on 9/7/15.
//

#ifndef OOS_SQLTEST_HPP
#define OOS_SQLTEST_HPP


#include "matador/unit/unit_test.hpp"

#include "matador/sql/connection.hpp"

namespace matador {
class connection;
}

class QueryTestUnit : public matador::unit_test
{
public:
  QueryTestUnit(const std::string &name, const std::string &msg, const std::string &db = "memory", const matador::time &timeval = matador::time(2015, 3, 15, 13, 56, 23, 123));

  virtual void initialize();

  void test_datatypes();
  void test_query_value_creator();
  void test_quoted_identifier();
  void test_columns_with_quotes_in_name();
  void test_quoted_literals();
  void test_bind_tablename();
  void test_describe();
  void test_identifier();
  void test_create();
  void test_update();
  void test_anonymous_create();
  void test_anonymous_insert();
  void test_anonymous_update();
  void test_statement_insert();
  void test_statement_update();
  void test_delete();
  void test_foreign_query();
  void test_query();
  void test_query_range_loop();
  void test_query_select();
  void test_query_select_count();
  void test_query_select_columns();
  void test_select_limit();
  void test_update_limit();
  void test_prepared_statement();
  void test_prepared_statement_creation();
  void test_prepared_object_result_twice();
  void test_prepared_scalar_result_twice();
  void test_rows();

protected:
  matador::connection create_connection();

  std::string db() const;

private:
  std::string db_;
  matador::connection connection_;
  matador::time time_val_;
};


#endif //OOS_SQLTEST_HPP
