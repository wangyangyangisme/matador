#include "OrmReloadTestUnit.hpp"

#include "../Item.hpp"
#include "../has_many_list.hpp"

#include "matador/orm/persistence.hpp"
#include "matador/orm/session.hpp"

#include "matador/object/object_view.hpp"

using namespace hasmanylist;

OrmReloadTestUnit::OrmReloadTestUnit(const std::string &prefix, const std::string &dns)
  : unit_test(prefix + "_orm_reload", prefix + " orm test unit")
  , dns_(dns)
{
  add_test("load", std::bind(&OrmReloadTestUnit::test_load, this), "test load from table");
  add_test("load_has_one", std::bind(&OrmReloadTestUnit::test_load_has_one, this), "test load has one relation from table");
  add_test("load_has_many", std::bind(&OrmReloadTestUnit::test_load_has_many, this), "test load has many from table");
  add_test("load_has_many_int", std::bind(&OrmReloadTestUnit::test_load_has_many_int, this), "test load has many int from table");
}

void OrmReloadTestUnit::test_load()
{
  matador::persistence p(dns_);

  p.attach<person>("person");

  p.create();

  std::vector<std::string> names({"hans", "otto", "georg", "hilde", "ute", "manfred"});

  {
    // insert some persons
    matador::session s(p);

    for (std::string name : names) {
      auto pptr = s.insert(new person(name, matador::date(18, 5, 1980), 180));

      UNIT_EXPECT_GREATER(pptr->id(), 0UL, "is must be greater zero");
    }
  }

  p.clear();

  {
    // load persons from database
    matador::session s(p);

    s.load();

    typedef matador::object_view<person> t_person_view;
    t_person_view persons(s.store());

    UNIT_ASSERT_TRUE(!persons.empty(), "person view must not be empty");
    UNIT_ASSERT_EQUAL(persons.size(), 6UL, "thier must be 6 persons");

    t_person_view::iterator first = persons.begin();
    t_person_view::iterator last = persons.end();

    while (first != last) {
      auto pptr = *first++;
      names.erase(std::remove_if(std::begin(names), std::end(names), [pptr](const std::string &name) {
        return name == pptr->name();
      }), names.end());
    }
    UNIT_ASSERT_TRUE(names.empty(), "names must be empty");
  }

  p.drop();
}

void OrmReloadTestUnit::test_load_has_one()
{
  matador::persistence p(dns_);

  p.attach<master>("master");
  p.attach<child>("child");

  p.create();

  {
    // insert some persons
    matador::session s(p);

    auto c = s.insert(new child("child 1"));

    auto m = new master("master 1");
    m->children = c;
    s.insert(m);
  }

  p.clear();

  {
    // load persons from database
    matador::session s(p);

    s.load();

    typedef matador::object_view<master> t_master_view;
    t_master_view masters(s.store());

    UNIT_ASSERT_TRUE(!masters.empty(), "master view must not be empty");
    UNIT_ASSERT_EQUAL(masters.size(), 1UL, "their must be 1 master");

    auto mptr = masters.front();
    UNIT_ASSERT_NOT_NULL(mptr->children.get(), "child must be valid");
  }

  p.drop();
}

void OrmReloadTestUnit::test_load_has_many()
{
  matador::persistence p(dns_);

  p.attach<child>("child");
  p.attach<children_list>("children_list");

  p.create();

  {
    matador::session s(p);

    auto children = s.insert(new children_list("children list 1"));

    UNIT_ASSERT_GREATER(children->id, 0UL, "invalid children list");
    UNIT_ASSERT_TRUE(children->children.empty(), "children list must be empty");

    auto kid1 = s.insert(new child("kid 1"));
    auto kid2 = s.insert(new child("kid 2"));

    UNIT_ASSERT_GREATER(kid1->id, 0UL, "invalid child");
    UNIT_ASSERT_GREATER(kid2->id, 0UL, "invalid child");

    s.push_back(children->children, kid1);
    s.push_back(children->children, kid2);

    UNIT_ASSERT_FALSE(children->children.empty(), "children list couldn't be empty");
    UNIT_ASSERT_EQUAL(children->children.size(), 2UL, "invalid children list size");
  }

  p.clear();

  {
    matador::session s(p);

    s.load();

    typedef matador::object_view<children_list> t_children_list_view;
    t_children_list_view children_lists(s.store());

    UNIT_ASSERT_TRUE(!children_lists.empty(), "children lists view must not be empty");
    UNIT_ASSERT_EQUAL(children_lists.size(), 1UL, "their must be 1 children list");

    auto clptr = children_lists.front();

    UNIT_ASSERT_FALSE(clptr->children.empty(), "children list couldn't be empty");
    UNIT_ASSERT_EQUAL(clptr->children.size(), 2UL, "invalid children list size");

    std::vector<std::string> result_names({ "kid 1", "kid 2"});
    for (auto kid : clptr->children) {
      //std::cout << "kid " << kid->name << " (id: " << kid->id << ")\n";
      auto it = std::find(result_names.begin(), result_names.end(), kid->name);
      UNIT_EXPECT_FALSE(it == result_names.end(), "kid must be found");
    }
  }

  p.drop();
}

void OrmReloadTestUnit::test_load_has_many_int()
{
  matador::persistence p(dns_);

  p.attach<many_ints>("many_ints");

  p.create();

  {
    matador::session s(p);

    auto intlist = s.insert(new many_ints);

    UNIT_ASSERT_GREATER(intlist->id, 0UL, "invalid intlist list");
    UNIT_ASSERT_TRUE(intlist->elements.empty(), "intlist list must be empty");

    s.push_back(intlist->elements, 4);

    UNIT_ASSERT_EQUAL(intlist->elements.front(), 4, "first int must be 4");
    UNIT_ASSERT_EQUAL(intlist->elements.back(), 4, "last int must be 4");

    s.push_front(intlist->elements, 7);

    UNIT_ASSERT_EQUAL(intlist->elements.front(), 7, "first int must be 7");

    UNIT_ASSERT_FALSE(intlist->elements.empty(), "intlist list couldn't be empty");
    UNIT_ASSERT_EQUAL(intlist->elements.size(), 2UL, "invalid intlist list size");
  }

  p.clear();

  {
    matador::session s(p);

    s.load();

    typedef matador::object_view<many_ints> t_many_ints_view;
    t_many_ints_view ints_view(s.store());

    UNIT_ASSERT_TRUE(!ints_view.empty(), "many ints view must not be empty");
    UNIT_ASSERT_EQUAL(ints_view.size(), 1UL, "their must be 1 int in many ints list");

    auto intlist = ints_view.front();

    UNIT_ASSERT_FALSE(intlist->elements.empty(), "intlist list couldn't be empty");
    UNIT_ASSERT_EQUAL(intlist->elements.size(), 2UL, "invalid intlist list size");

    std::vector<int> result_ints({ 4, 7 });
    for (auto i : intlist->elements) {
      auto it = std::find(result_ints.begin(), result_ints.end(), i);
      UNIT_EXPECT_FALSE(it == result_ints.end(), "int must be found");
    }

  }

  p.drop();
}