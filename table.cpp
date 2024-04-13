#include <cassert>
#include "table.h"
#include "exceptions.h"
#include "guard.h"

Table::Table( const std::string &name )
  : m_name( name )
  // TODO: initialize additional member variables
{
  // TODO: implement
  pthread_mutex_init(&m_mutex, NULL);
}

Table::~Table()
{
  // TODO: implement
  pthread_mutex_destroy(&m_mutex);
}

void Table::lock()
{
  // TODO: implement
  pthread_mutex_lock(&m_mutex);
}

void Table::unlock()
{
  // TODO: implement
  pthread_mutex_unlock(&m_mutex);
}

bool Table::trylock()
{
  // TODO: implement
  return pthread_mutex_trylock(&m_mutex) == 0;
}

void Table::set( const std::string &key, const std::string &value )
{
  // TODO: implement
  m_temporary_changes[key] = value;
}

std::string Table::get( const std::string &key )
{
  // TODO: implement
  auto it = m_temporary_changes.find(key);
  if (it != m_temporary_changes.end()) {
    return it->second;
  }

  it = m_data.find(key);
  if (it != m_data.end()) {
    return it->second;
  }

  throw OperationException("Cannot find key: " + key);
}

bool Table::has_key( const std::string &key )
{
  // TODO: implement
  return m_data.find(key) != m_data.end() || m_temporary_changes.find(key) != m_temporary_changes.end();
}

void Table::commit_changes()
{
  // TODO: implement
  for (const auto &pair : m_temporary_changes) {
    m_data[pair.first] = pair.second;
  }
  m_temporary_changes.clear();
}

void Table::rollback_changes()
{
  // TODO: implement
  m_temporary_changes.clear();
}
