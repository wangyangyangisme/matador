//
// Created by sascha on 30.03.18.
//

#ifndef MATADOR_BELONGS_TO_RESOLVER_HPP
#define MATADOR_BELONGS_TO_RESOLVER_HPP

#include "matador/utils/access.hpp"

#include "matador/object/belongs_to.hpp"
#include "matador/object/has_one.hpp"
#include "matador/object/prototype_node.hpp"

#include "matador/orm/basic_table.hpp"

#include <cstddef>

namespace matador {
namespace detail {

/// @cond MATADOR_DEV

template < class T >
class belongs_to_resolver
{
public:
  typedef std::map<std::string, std::shared_ptr<matador::basic_table>> t_table_map;

public:
  explicit belongs_to_resolver(basic_table &tbl) : table_(tbl) {}

  t_table_map resolve()
  {
    table_map_.clear();

    auto node = table_.persistence_unit().store().template find<T>();

    if (node == table_.persistence_unit().store().end()) {
      // Todo: introduce throw_orm_exception
      throw std::logic_error("belongs_to_resolver::resolve: no owner table " + std::string(typeid(T).name()) + " found");
    }
    prototype_node* n = node.get();
    T* proto = n->prototype<T>();
    serialize(*proto);

    return table_map_;
  }

  template<class V>
  void serialize(V &obj)
  {
    matador::access::serialize(*this, obj);
  }

  template<class V>
  void serialize(const char *, V &) {}
  void serialize(const char *, char *, size_t) {}

  template<class V>
  void serialize(const char *id, belongs_to<V> &, cascade_type)
  {
    auto it = table_.find_table<V>();
    if (it != table_.end_table()) {
      table_map_.insert(std::make_pair(id, it->second));
    }
  }

  template<class V>
  void serialize(const char *id, has_one<V> &, cascade_type)
  {
    auto it = table_.find_table<V>();
    if (it != table_.end_table()) {
      table_map_.insert(std::make_pair(id, it->second));
    }
  }

  void serialize(const char *, abstract_has_many &, const char *, const char *, cascade_type) {}
  void serialize(const char *, abstract_has_many &, cascade_type) {}

private:
  basic_table &table_;

  t_table_map table_map_;
};

/// @endcond

}
}
#endif //MATADOR_BELONGS_TO_RESOLVER_HPP
