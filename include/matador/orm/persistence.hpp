//
// Created by sascha on 20.03.16.
//

#ifndef OOS_PERSISTENCE_HPP
#define OOS_PERSISTENCE_HPP

#ifdef _MSC_VER
#ifdef matador_orm_EXPORTS
#define OOS_ORM_API __declspec(dllexport)
#define EXPIMP_ORM_TEMPLATE
#else
#define OOS_ORM_API __declspec(dllimport)
#define EXPIMP_ORM_TEMPLATE extern
#endif
#pragma warning(disable: 4251)
#else
#define OOS_ORM_API
#endif

#include "matador/sql/connection.hpp"

#include "matador/object/object_store.hpp"

#include "matador/orm/basic_table.hpp"
#include "matador/orm/persistence_observer.hpp"

#include <memory>
#include <unordered_map>

namespace matador {

/**
 * @brief Represents the persistence layer for a database and an object_store
 *
 * This class acts as mediator between the database connection
 * and the object_store.
 * It attaches the object prototypes to the object_store and
 * drops or creates the corresponding tables on the database.
 *
 * It also holds a map of all created tables, checks if a table exists
 * and is able to clear the object store.
 */
class OOS_ORM_API persistence
{
public:
  typedef basic_table::table_ptr table_ptr;      /**< Shortcut to the table shared pointer */
  typedef basic_table::t_table_map t_table_map;  /**< Shortcut to map of table shared pointer */

public:
  /**
   * @brief Creates a new persistence object
   *
   * Creates a new persistence object for the given
   * database connection string.
   *
   * @param dns The database connection string
   */
  explicit persistence(const std::string &dns);

  ~persistence();

  /**
   * Inserts a new object prototype into the prototype tree. The prototype
   * consists of a unique type name. To know where the new
   * prototype is inserted into the hierarchy the type name of the parent
   * node is also given.
   *
   * @tparam T       The type of the prototype node
   * @param type     The unique name of the type.
   * @param abstract Indicates if the producers serializable is treated as an abstract node.
   * @param parent   The name of the parent type.
   * @return         Returns new inserted prototype iterator.
   */
  template<class T>
  void attach(const char *type, object_store::abstract_type abstract = object_store::abstract_type::not_abstract,
              const char *parent = nullptr);

  /**
   * Inserts a new object prototype into the prototype tree. The prototype
   * consists of a unique type name. To know where the new
   * prototype is inserted into the hierarchy the type name of the parent
   * node is also given.
   * parameter.
   *
   * @tparam T       The type of the prototype node
   * @tparam S       The type of the parent prototype node
   * @param type     The unique name of the type.
   * @param abstract Indicates if the producers serializable is treated as an abstract node.
   * @return         Returns new inserted prototype iterator.
   */
  template<class T, class S>
  void attach(const char *type, object_store::abstract_type abstract = object_store::abstract_type::not_abstract);

  /**
   * Removes an object prototype from the prototype tree. All children
   * nodes and all objects are also removed.
   *
   * @param type The name of the type to remove.
   */
  void detach(const char *type);

  /**
   * Checks if the given entity as
   * table exists
   *
   * @tparam T entity type class
   * @return True if table exists
   */
  template<class T>
  bool exists()
  {
    t_table_map::iterator i = tables_.find(store_.type<T>());

    if (i == tables_.end()) {
      return false;
    }
    return connection_.exists(i->second->name());
  }

  /**
   * @brief Creates all tables.
   *
   * Creates all tables currently attached
   * to the persistence object.
   */
  void create();

  /**
   * @brief Drops all tables.
   *
   * Drops all tables currently attached
   * to the persistence object.
   */
  void drop();

  /**
   * clears all objects from object store
   */
  void clear();

  /**
   * @brief Finds a table by its name
   *
   * Finds a table by its name. If the name is unknown
   * map::end is returned.
   *
   * @param type The name of the table
   * @return The iterator of the table or end
   */
  t_table_map::iterator find_table(const std::string &type);

  /**
   * @brief Returns the begin of the table map
   *
   * @return The begin of the table map
   */
  t_table_map::iterator begin();

  /**
   * @brief Returns the end of the table map
   *
   * @return The end of the table map
   */
  t_table_map::iterator end();

  /**
   * @brief Return a reference to the underlaying object_store
   *
   * @return A reference to the object_store.
   */
  object_store &store();

  /**
   * @brief Return a const reference to the underlaying object_store
   *
   * @return A const reference to the object_store.
   */
  const object_store &store() const;

  /**
   * @brief Return a reference to the underlaying database connection
   *
   * @return A reference to the database connection.
   */
  connection &conn();

  /**
   * @brief Return a const reference to the underlaying database connection
   *
   * @return A const reference to the database connection.
   */
  const connection &conn() const;

private:
  template < class T >
  friend class persistence_observer;

private:
  connection connection_;
  object_store store_;

  t_table_map tables_;
};

}

#include "persistence_observer.tpp"

namespace matador {

template<class T>
void persistence::attach(const char *type, object_store::abstract_type abstract, const char *parent)
{
  store_.attach<T>(type, abstract, parent, { new persistence_observer<T>(*this) });
}

template<class T, class S>
void persistence::attach(const char *type, object_store::abstract_type abstract)
{
  store_.attach<T,S>(type, abstract, { new persistence_observer<T>(*this) });
}

}

#endif //OOS_PERSISTENCE_HPP
