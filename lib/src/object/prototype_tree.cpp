/*
 * This file is part of OpenObjectStore OOS.
 *
 * OpenObjectStore OOS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenObjectStore OOS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenObjectStore OOS. If not, see <http://www.gnu.org/licenses/>.
 */
#include "object/prototype_tree.hpp"

#include "object/object_exception.hpp"
#include "object/object_store.hpp"
#include "object/object_atomizer.hpp"
#include "object/object_container.hpp"

#include <iterator>

namespace oos {

class prototype_tree;

class relation_handler2 : public generic_object_writer<relation_handler2>
{
public:
  typedef std::list<std::string> string_list_t;
  typedef string_list_t::const_iterator const_iterator;

public:
  relation_handler2(prototype_tree &ptree, prototype_node *node)
    : generic_object_writer<relation_handler2>(this)
    , ptree_(ptree)
    , node_(node)
  {}
  virtual ~relation_handler2() {}

  template < class T >
  void write_value(const char*, const T&) {}

  void write_value(const char*, const char*, int) {}

  void write_value(const char *id, const object_container &x)
  {
    /*
     * container knows if it needs
     * a relation table
     */
    x.handle_container_item(ptree_, id, node_);
  }

private:
  prototype_tree &ptree_;
  prototype_node *node_;
};

prototype_iterator::prototype_iterator()
  : node_(NULL)
{}

prototype_iterator::prototype_iterator(prototype_node *node)
  : node_(node)
{}

prototype_iterator::prototype_iterator(const prototype_iterator &x)
  : node_(x.node_)
{}

prototype_iterator& prototype_iterator::operator=(const prototype_iterator &x)
{
  node_ = x.node_;
  return *this;
}

prototype_iterator::~prototype_iterator()
{}

bool prototype_iterator::operator==(const prototype_iterator &i) const
{
  return (node_ == i.node_);
}

bool prototype_iterator::operator!=(const prototype_iterator &i) const
{
  return !operator==(i);
}

prototype_iterator::self& prototype_iterator::operator++()
{
  increment();
  return *this;
}

prototype_iterator::self prototype_iterator::operator++(int)
{
  prototype_node *tmp = node_;
  increment();
  return prototype_iterator(tmp);
}

prototype_iterator::self& prototype_iterator::operator--()
{
  decrement();
  return *this;
}

prototype_iterator::self prototype_iterator::operator--(int)
{
  prototype_node *tmp = node_;
  decrement();
  return prototype_iterator(tmp);
}

prototype_iterator::pointer prototype_iterator::operator->() const
{
  return node_;
}

prototype_iterator::reference prototype_iterator::operator*() const
{
  return *node_;
}

prototype_iterator::pointer prototype_iterator::get() const
{
  return node_;
}

void prototype_iterator::increment()
{
  if (node_) {
    node_ = node_->next_node();
  }
}
void prototype_iterator::decrement()
{
  if (node_) {
    node_ = node_->previous_node();
  }
}

prototype_tree::prototype_tree()
  : first_(new prototype_node)
  , last_(new prototype_node)
{
  prototype_node *root = new prototype_node(new object_producer<object>, "object", true);
  object_proxy *first = new object_proxy(0);
  object_proxy *last = new object_proxy(0);

  // init object proxies
  root->op_first = first;
  root->op_marker = last;
  root->op_last = last;
  root->op_first->next = root->op_last;
  root->op_last->prev = root->op_first;

  // link nodes together
  first_->next = root;
  root->prev = first_;
  root->next = last_;
  last_->prev = root;

  // add to maps
  prototype_map_.insert(std::make_pair("object", root));
  typeid_prototype_map_[root->producer->classname()].insert(std::make_pair("object", root));

}

prototype_tree::~prototype_tree()
{
  delete last_;
  delete first_;
}


prototype_iterator prototype_tree::insert(object_base_producer *producer, const char *type, bool abstract, const char *parent) {
  // set node to root node
  prototype_node *parent_node = find_prototype_node(parent);
  if (!parent_node) {
    throw object_exception("couldn't find parent prototype");
  }

  /*
   * try to insert new prototype node
   */
  prototype_node *node = 0;
  t_prototype_map::iterator i = prototype_map_.find(type);
  if (i == prototype_map_.end()) {
    /* unknown type name try for typeid
     * (unfinished prototype)
     */
    i = prototype_map_.find(producer->classname());
    if (i == prototype_map_.end()) {
      /*
       * no typeid found, seems to be
       * a new type
       * to be sure check in typeid map
       */
      t_typeid_prototype_map::iterator j = typeid_prototype_map_.find(producer->classname());
      if (j != typeid_prototype_map_.end() && j->second.find(type) != j->second.end()) {
        /* unexpected found the
         * typeid check for type
         */
        /* type found in typeid map
         * throw exception
         */
        throw object_exception("unexpectly found prototype");
      } else {
        /* insert new prototype and add to
         * typeid map
         */
        // create new one
        node = new prototype_node(producer, type, abstract);
      }
    } else {
      /* prototype is unfinished,
       * finish it, insert by type name,
       * remove typeid entry and add to
       * typeid map
       */
      node = i->second;
      node->initialize(producer, type, abstract);
      prototype_map_.erase(i);
    }
  } else {
    // already inserted return iterator
    throw object_exception("prototype already inserted");
  }

  parent_node->insert(node);

  // store prototype in map
  // Todo: check return value
  prototype_map_.insert(std::make_pair(type, node)).first;
  typeid_prototype_map_[producer->classname()].insert(std::make_pair(type, node));

  // Check if nodes object has to many relations
  object *o = producer->create();
  relation_handler2 rh(*this, node);
  o->serialize(rh);
  delete o;

  return prototype_iterator(node);
}

prototype_iterator prototype_tree::find(const char *type) {
  return prototype_iterator(find_prototype_node(type));
}


bool prototype_tree::empty() const {
  return first_->next == last_->prev;
}


size_t prototype_tree::size() const {
  return std::distance(begin(), end());
}

void prototype_tree::clear(const char *type, bool recursive) {
  prototype_node *node = find_prototype_node(type);
  if (!node) {
    throw object_exception("couldn't find prototype");
  }
  if (recursive) {
    // clear all objects from child nodes
    // for each child call clear_prototype(child, recursive);
    prototype_node *child = node->next_node();
    while (child && (child != node || child != node->parent)) {
      child->clear();
      child = child->next_node();
    }
  }

  node->clear();
}

void prototype_tree::remove(const char *type) {
  prototype_node *node = find_prototype_node(type);
  if (!node) {
    throw object_exception("couldn't find prototype");
  }

  // remove (and delete) from tree (deletes subsequently all child nodes
  // for each child call remove_prototype(child);
  while (node->first->next != node->last) {
    remove(node->first->next->type.c_str());
  }
  // and objects they're containing
  node->clear();
  // delete prototype node as well
  // unlink node
  node->unlink();
  // get iterator
  t_prototype_map::iterator i = prototype_map_.find(node->type.c_str());
  if (i != prototype_map_.end()) {
    prototype_map_.erase(i);
  }
  // find item in typeid map
  t_typeid_prototype_map::iterator j = typeid_prototype_map_.find(node->producer->classname());
  if (j != typeid_prototype_map_.end()) {
    j->second.erase(type);
    if (j->second.empty()) {
      typeid_prototype_map_.erase(j);
    }
  } else {
    throw object_exception("couldn't find node by id");
  }
  delete node;
}

prototype_iterator prototype_tree::begin() const {
  return prototype_iterator(first_->next);
}

prototype_iterator prototype_tree::end() const {
  return prototype_iterator(last_);
}

prototype_node* prototype_tree::find_prototype_node(const char *type) {
  // check for null
  if (type == 0) {
    throw object_exception("invalid type (null)");
  }
  /*
 * first search in the prototype map
 */
  t_prototype_map::const_iterator i = prototype_map_.find(type);
  if (i == prototype_map_.end()) {
    /*
   * if not found search in the typeid to prototype map
   */
    t_typeid_prototype_map::const_iterator j = typeid_prototype_map_.find(type);
    if (j == typeid_prototype_map_.end()) {
      throw object_exception("unknown prototype type");
    } else {
      const t_prototype_map &val = j->second;
      /*
     * if size is greater one (1) the name
     * is a typeid and has more than one prototype
     * node and therefor it is not unique and an
     * exception is thrown
     */
      if (val.size() > 1) {
        // throw exception
        throw object_exception("type id not unique");
      } else {
        // return the only prototype
        return val.begin()->second;
      }
    }
  } else {
    return i->second;
  }
}

}