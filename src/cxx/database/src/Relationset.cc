/**
 * @file database/src/Relationset.cc
 * @author <a href="mailto:andre.anjos@idiap.ch">Andre Anjos</a> 
 *
 * @brief Implementation of the Relationset class for Torch databases
 */

#include <list>
#include <algorithm>
#include <boost/make_shared.hpp>

#include "database/Relationset.h"
#include "database/Exception.h"
#include "database/Dataset.h"

namespace db = Torch::database;

db::Relationset::Relationset () :
  m_parent(0),
  m_relation(),
  m_rule()
{
}

db::Relationset::Relationset (const Relationset& other) :
  m_parent(other.m_parent),
  m_relation(other.m_relation),
  m_rule(other.m_rule)
{
}

db::Relationset::~Relationset() { }

db::Relationset& db::Relationset::operator= (const db::Relationset& other) {
  m_parent = other.m_parent;
  m_relation = other.m_relation;
  m_rule = other.m_rule;
  return *this;
}

size_t db::Relationset::getNextFreeId() const {
  if (!m_relation.size()) return 1;
  return m_relation.rbegin()->first + 1; //remember: std::map is sorted by key!
}

void db::Relationset::consolidateIds() {
  size_t id=1;
  for (std::map<size_t, boost::shared_ptr<db::Relation> >::iterator it = m_relation.begin(); it != m_relation.end(); ++it, ++id) {
    if (id != it->first) { //displaced value, reset
      m_relation[id] = it->second;
      m_relation.erase(it->first);
    }
  }
}

void db::Relationset::fillMemberMap(size_t id, std::map<std::string, std::vector<std::pair<size_t, size_t> > >& dictionary) const {
  if (!exists(id)) throw db::IndexError(id);
  const db::Relation& r = (*this)[id];

  checkRelation(r); //D.R.Y.

  for (std::list<std::pair<size_t,size_t> >::const_iterator it = r.members().begin(); it != r.members().end(); ++it) {
    //it->first == arrayset-id, it->second == array-id
    const db::Arrayset& arrayset = (*m_parent)[it->first];
    dictionary[arrayset.getRole()].push_back(std::make_pair(it->first, it->second));
  }
}

void db::Relationset::checkRelation(const db::Relation& r) const {
  if (!m_parent) throw db::Uninitialized();
  if (!m_rule.size()) throw db::Uninitialized();

  //stage 1: fill the role count map, check if the mentioned arrays do exist
  std::map<std::string, size_t> role_count;
  
  for (std::list<std::pair<size_t,size_t> >::const_iterator it = r.members().begin(); it != r.members().end(); ++it) {
    //it->first == arrayset-id, it->second == array-id
    if (!m_parent->exists(it->first)) throw db::IndexError(it->first);
    const db::Arrayset& arrayset = (*m_parent)[it->first];
    if (it->second) //specific array required, try get
      if (!arrayset.exists(it->second)) throw db::IndexError(it->second);

    //if you get to this point, the array exists, get roles and count
    if (role_count.find(arrayset.getRole()) == role_count.end())
      role_count[arrayset.getRole()] = 0;

    //if it is a single array:
    if (it->second) role_count[arrayset.getRole()] += 1;
    else role_count[arrayset.getRole()] += arrayset.getNSamples();
  }
   
  //stage 2: compare the role count with the rules - in this stage we consume
  //the role counts until all rules have been scanned.
  for (std::map<std::string, boost::shared_ptr<Rule> >::const_iterator it = m_rule.begin(); it != m_rule.end(); ++it) {
    if (role_count.find(it->first) == role_count.end()) 
      throw db::InvalidRelation(); //cannot find such role!
    if (role_count[it->first] < it->second->getMin())
      throw db::InvalidRelation(); //does not satisfy the minimum
    if (it->second->getMax() && (role_count[it->first]>it->second->getMax()))
      throw db::InvalidRelation(); //does not statisfy the maximum
    //if you got here the rule exists and it satisfies both min and maximum
    role_count.erase(it->first);
  }

  //well, after consuming the role count I cannot have anything left, or it
  //means I have uncovered members, what is an error:
  if (role_count.size()) throw db::InvalidRelation(); //something is missing!
}

size_t db::Relationset::add (const db::Relation& relation) {
  checkRelation(relation);
  m_relation[getNextFreeId()] = boost::make_shared<db::Relation>(relation);
  return m_relation.rbegin()->first;
}

size_t db::Relationset::add (boost::shared_ptr<const db::Relation> relation) {
  return add(*relation.get());
}

void db::Relationset::add (size_t id, const db::Relation& relation) {
  if (m_relation.find(id) != m_relation.end()) throw db::IndexError(id);
  checkRelation(relation);
  m_relation[id] = boost::make_shared<db::Relation>(relation);
}

void db::Relationset::add (size_t id, boost::shared_ptr<const db::Relation> relation) {
  add(id, *relation.get());
}

void db::Relationset::set (size_t id, const db::Relation& relation) {
  if (m_relation.find(id) == m_relation.end()) throw db::IndexError(id);
  checkRelation(relation);
  m_relation[id] = boost::make_shared<db::Relation>(relation);
}

void db::Relationset::set (size_t id, boost::shared_ptr<const db::Relation> relation) {
  set(id, *relation.get());
}

void db::Relationset::remove (size_t id) {
  m_relation.erase(id);
}

db::Relation& db::Relationset::operator[] (size_t id) {
  return *ptr(id).get();
}

const db::Relation& db::Relationset::operator[] (size_t id) const {
  return *ptr(id).get();
}

boost::shared_ptr<db::Relation> db::Relationset::ptr (size_t id) {
  std::map<size_t, boost::shared_ptr<db::Relation> >::iterator it = m_relation.find(id);
  if (it == m_relation.end()) throw db::IndexError(id);
  return it->second;
}

boost::shared_ptr<const db::Relation> db::Relationset::ptr (size_t id) const {
  std::map<size_t, boost::shared_ptr<db::Relation> >::const_iterator it = m_relation.find(id);
  if (it == m_relation.end()) throw db::IndexError(id);
  return it->second;
}

void db::Relationset::add (const std::string& role, const db::Rule& rule) {
  //if you want to remove a rule, you must first remove all relations that
  //follow that rule - otherwise, consistency cannot be guaranteed
  if (m_relation.size()) throw db::AlreadyHasRelations(m_relation.size());
  if (m_rule.find(role) != m_rule.end()) throw db::NameError(role);
  m_rule[role] = boost::make_shared<db::Rule>(rule);
}

void db::Relationset::add (const std::string& role, 
  boost::shared_ptr<const db::Rule> rule) {
  add(role, *rule.get());
}

void db::Relationset::set (const std::string& role, const db::Rule& rule) {
  //please note that when you reset a rule, we have to recompute all relations
  //consistency - we throw if a relation does not obey
  if (m_rule.find(role) == m_rule.end()) throw db::NameError(role);
  m_rule[role] = boost::make_shared<db::Rule>(rule);
  for (std::map<size_t, boost::shared_ptr<db::Relation> >::const_iterator
      it = m_relation.begin(); it != m_relation.end(); ++it) {
    checkRelation(*(it->second.get()));
  }
}

void db::Relationset::set (const std::string& role, 
  boost::shared_ptr<const db::Rule> rule) {
  set(role, *rule.get());
}

void db::Relationset::remove(const std::string& role) {
  if (m_rule.find(role) == m_rule.end()) throw db::NameError(role);
  m_rule.erase(role);
}

boost::shared_ptr<db::Rule> db::Relationset::ptr (const std::string& role) {
  std::map<std::string, boost::shared_ptr<db::Rule> >::iterator it = m_rule.find(role);
  if (it == m_rule.end()) throw db::NameError(role);
  return it->second;
}
      
boost::shared_ptr<const db::Rule> db::Relationset::ptr (const std::string& role) const {
  std::map<std::string, boost::shared_ptr<db::Rule> >::const_iterator it = m_rule.find(role);
  if (it == m_rule.end()) throw db::NameError(role);
  return it->second;
}

db::Rule& db::Relationset::operator[] (const std::string& role) {
  return *ptr(role).get();
}

const db::Rule& db::Relationset::operator[] (const std::string& role) const {
  return *ptr(role).get();
}

void db::Relationset::clearRules () {
  if (m_relation.size()) throw db::AlreadyHasRelations(m_relation.size());
}

bool db::Relationset::exists(size_t relation_id) const {
  return (m_relation.find(relation_id) != m_relation.end());
}

bool db::Relationset::exists(const std::string& rule_role) const {
  return (m_rule.find(rule_role) != m_rule.end());
}