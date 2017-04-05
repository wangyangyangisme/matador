#include "matador/object/object_inserter.hpp"
#include "matador/object/object_store.hpp"

namespace matador {
namespace detail {

template<class T>
void object_inserter::insert(object_proxy *proxy, T *o, bool notify)
{

  notify_ = notify;

  object_proxy_stack_.push(proxy);

  modified_marker_ = [](object_store &store, object_proxy *oproxy) {
    store.mark_modified<T>(oproxy);
  };

  if (proxy->obj()) {
    matador::access::serialize(*this, *o);
  }
  object_proxy_stack_.pop();
}

template<class T>
void object_inserter::serialize(T &x)
{
  matador::access::serialize(*this, x);
}

template<class T>
void object_inserter::serialize(const char *, belongs_to<T> &x, cascade_type cascade) {
  if (x.is_inserted() || (x.proxy_ && x.proxy_->obj() == nullptr)) {
    return;
  }
  x.is_inserted_ = true;
  x.cascade_ = cascade;
  x.owner_ = object_proxy_stack_.top();

  prototype_iterator foreign_node = ostore_.find<T>();
  if (foreign_node != ostore_.end()) {
    auto i = foreign_node->relation_field_endpoint_map_.find(foreign_node->type_index());
    if (i != foreign_node->relation_field_endpoint_map_.end()) {
      x.relation_info_ = i->second;
    }
  }

  // object was seen by inserter stop inserting
  if (!object_proxies_.insert(x.proxy_).second) {
    return;
  }

  if (!x.proxy_) {
    return;
  }

  if (x.id()) {
    // do the pointer count
    object_proxy_stack_.push(x.proxy_);
    matador::access::serialize(*this, *(T*)x.ptr());
    object_proxy_stack_.pop();
  } else {
    // new object
    ostore_.insert<T>(x.proxy_, notify_);
  }
  ++(*x.proxy_);
}

template<class T>
void object_inserter::serialize(const char *, has_one<T> &x, cascade_type cascade) {
  if (x.is_inserted() || (x.proxy_ && x.proxy_->obj() == nullptr)) {
    return;
  }
  x.is_inserted_ = true;
  x.cascade_ = cascade;
  x.owner_ = object_proxy_stack_.top();

  prototype_iterator foreign_node = ostore_.find<T>();
  if (foreign_node != ostore_.end()) {
    auto i = foreign_node->relation_field_endpoint_map_.find(foreign_node->type_index());
    if (i != foreign_node->relation_field_endpoint_map_.end()) {
      x.relation_info_ = i->second;
    }
  }

  // object was seen by inserter stop inserting
  if (!object_proxies_.insert(x.proxy_).second) {
    return;
  }

  if (!x.proxy_) {
    return;
  }

  if (x.id()) {
    // do the pointer count
    object_proxy_stack_.push(x.proxy_);
    matador::access::serialize(*this, *(T*)x.ptr());
    object_proxy_stack_.pop();
  } else {
    // new object
    ostore_.insert<T>(x.proxy_, notify_);
  }
  ++(*x.proxy_);
}

template<class T, template<class ...> class C>
void object_inserter::serialize(const char *, basic_has_many<T, C> &x, const char*, const char*)
{
  // initialize the has many relation
  // set identifier
  // relation table name
  // owner column name
  // item column name
  if (object_proxy_stack_.empty()) {
    throw object_exception("no owner for has many relation");
  }

  if (x.ostore_) {
    return;
  }
  object_proxy *proxy = object_proxy_stack_.top();
  x.owner_id_ = proxy->pk();
  x.owner_ = proxy;
  x.ostore_ = &ostore_;
  x.mark_modified_owner_ = modified_marker_;

  prototype_iterator foreign_node = ostore_.find<T>();
  if (foreign_node != ostore_.end()) {
    auto i = foreign_node->relation_field_endpoint_map_.find(foreign_node->type_index());
    if (i != foreign_node->relation_field_endpoint_map_.end()) {
      x.relation_info_ = i->second;
    }
  }
  typename basic_has_many<T, C>::iterator first = x.begin();
  typename basic_has_many<T, C>::iterator last = x.end();

  while (first != last) {
    typename basic_has_many<T, C>::relation_type i = (first++).relation_item();
    if (!i.is_inserted()) {
      // item is not in store, insert it
      ostore_.insert(i, false);
    }
  }
}

}
}