/*
 * A5MS1
 * The Table class
 * Cassie Zhang xzhan304
 * Tianai Yue tyue4
 */

#include <cassert>
#include "table.h"
#include "exceptions.h"
#include "guard.h"

Table::Table( const std::string &name )
  : m_name( name )
    // initialize additional member variables
{
    // implement
    pthread_mutex_init(&m_mutex, NULL);
}

Table::~Table()
{
    // implement
    pthread_mutex_destroy(&m_mutex);
}

void Table::lock()
{
    // implement
    pthread_mutex_lock(&m_mutex);
}

void Table::unlock()
{
    // implement
    pthread_mutex_unlock(&m_mutex);
}

bool Table::trylock()
{
    // implement
    return pthread_mutex_trylock(&m_mutex) == 0;
}

void Table::set( const std::string &key, const std::string &value )
{
    // implement
    m_temporary_changes[key] = value;
}

std::string Table::get( const std::string &key )
{
    // implement
    auto it = m_temporary_changes.find(key);
    //return value from temporary changes if present
    if (it != m_temporary_changes.end()) {
        return it->second;
    }

    //return value from main data if not in temporary changes
    it = m_data.find(key);
    if (it != m_data.end()) {
        return it->second;
    }

    throw OperationException("Cannot find key: " + key);
}

bool Table::has_key( const std::string &key )
{
    // implement
    return m_data.find(key) != m_data.end() || m_temporary_changes.find(key) != m_temporary_changes.end();
}

void Table::commit_changes()
{
    // implement
    for (const auto &pair : m_temporary_changes) {
        m_data[pair.first] = pair.second;
    }
    m_temporary_changes.clear();
}

void Table::rollback_changes()
{
    // implement
    m_temporary_changes.clear();
}

void Table::erase(const std::string &key) {
    if (m_data.find(key) != m_data.end()) {
        m_data.erase(key);
    }
}
