//
// Created by sascha on 12/10/15.
//

#include "object/basic_object_holder.hpp"
#include "object/object_proxy.hpp"
#include "object/object_exception.hpp"

namespace oos {

basic_object_holder::basic_object_holder(bool is_internal)
  : is_internal_(is_internal)
{}

basic_object_holder::basic_object_holder(const basic_object_holder &x)
  : proxy_(x.proxy_)
  , oid_(x.oid_)
{
  if (proxy_) {
    oid_ = proxy_->id();
    proxy_->add(this);
  }
}

basic_object_holder &
basic_object_holder::operator=(const basic_object_holder &x)
{
  if (this != &x && proxy_ != x.proxy_) {
    reset(x.proxy_, cascade_type::NONE);
  }
  return *this;
}

basic_object_holder::basic_object_holder(bool is_internal, object_proxy *op)
  : proxy_(op)
  , is_internal_(is_internal)
  , oid_(0)
{
  if (proxy_) {
    oid_ = proxy_->id();
    proxy_->add(this);
  }
}

basic_object_holder::~basic_object_holder()
{
  if (proxy_) {
    proxy_->remove(this);
    /*
     * if proxy was created temporary
     * we can delete it here
     */
    if (!proxy_->ostore() && proxy_->ptr_set_.empty()) {
      delete proxy_;
    }
  }
}

bool basic_object_holder::operator==(const basic_object_holder &x) const
{
  return x.proxy_ == proxy_;
}

bool basic_object_holder::operator!=(const basic_object_holder &x) const
{
  return !(x == *this);
}

void basic_object_holder::reset(object_proxy *proxy, cascade_type cascade)
{
  if (proxy_ == proxy) {
    return;
  }
  if (proxy_) {
    oid_ = 0;
    if (is_internal_ && is_inserted_ && proxy_->ostore_) {
      --(*proxy_);
//      if (cascade_ | cascade_type::DELETE) {
//        proxy_->unlink_ref();
//      } else {
//        proxy_->unlink_ptr();
//      }
    }
    proxy_->remove(this);
    /*
     * if proxy was created temporary
     * we can delete it here
     */
    if (!proxy_->ostore() && proxy_->ptr_set_.empty()) {
      delete proxy_;
    }
  }
  proxy_ = proxy;
  cascade_ = cascade;
  if (proxy_) {
    oid_ = proxy_->id();
    if (is_internal_ && is_inserted_ && proxy_->ostore_) {
      ++(*proxy_);
//      if (cascade_ | cascade_type::DELETE) {
//        proxy_->link_ref();
//      } else {
//        proxy_->link_ptr();
//      }
    }
    proxy_->add(this);
  }
}

void basic_object_holder::reset(const std::shared_ptr<basic_identifier> &id)
{
  if (proxy_ && !proxy_->pk()->is_same_type(*id)) {
    throw object_exception("identifier types are not equal");
  }
  reset(new object_proxy(id), cascade_type::NONE);
}

bool
basic_object_holder::is_loaded() const
{
  return (proxy_ && proxy_->obj());
}

unsigned long
basic_object_holder::id() const
{
  return (proxy_ ? proxy_->id() : oid_);
}

void basic_object_holder::id(unsigned long id)
{
  if (proxy_) {
    throw std::logic_error("proxy already set");
  } else {
    oid_ = id;
  }
}

object_store *basic_object_holder::store() const
{
  return (proxy_ ? proxy_->ostore() : nullptr);
}

void* basic_object_holder::ptr()
{
  return proxy_ ? proxy_->obj() : nullptr;
}

const void*basic_object_holder::ptr() const
{
  return lookup_object();
}

void* basic_object_holder::lookup_object()
{
  if (proxy_ && proxy_->obj()) {
    if (proxy_->ostore()) {
      // Todo: callback to object store
//      proxy_->ostore()->mark_modified(proxy_);
    }
    return proxy_->obj();
  } else {
    return nullptr;
  }
}

void* basic_object_holder::lookup_object() const
{
  return proxy_ ? proxy_->obj() : nullptr;
}

bool
basic_object_holder::is_internal() const
{
  return is_internal_;
}

bool basic_object_holder::is_inserted() const
{
  return is_inserted_;
}

bool basic_object_holder::has_primary_key() const
{
  return (proxy_ ? proxy_->has_identifier() : false);
}

std::shared_ptr<basic_identifier> basic_object_holder::primary_key() const
{
  return (proxy_ ? proxy_->pk() : nullptr);
}

unsigned long basic_object_holder::reference_count() const
{
  return (proxy_ ? proxy_->reference_counter_ : 0UL);
}

//unsigned long basic_object_holder::ref_count() const
//{
//  return (proxy_ ? proxy_->ref_count() : 0UL);
//}
//
//unsigned long basic_object_holder::ptr_count() const
//{
//  return (proxy_ ? proxy_->ptr_count() : 0UL);
//}

std::ostream& operator<<(std::ostream &out, const basic_object_holder &x)
{
  if (x.proxy_) {
    if (x.proxy_->obj()) {
      out << "loaded object " << x.proxy_->obj();
    } else {
      out << "unload object [" << x.id() << "]";
    }
  } else {
    out << "unknown object [" << 0 << "]";
  }
  return out;
}

}