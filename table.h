/*
 * A5MS1
 * The Table class header file
 * Cassie Zhang xzhan304
 * Tianai Yue tyue4
 */

#ifndef TABLE_H
#define TABLE_H

#include <map>
#include <string>
#include <pthread.h>

class Table {
private:
  std::string m_name;
  // TODO: add member variables
  std::map<std::string, std::string> m_data;
  std::map<std::string, std::string> m_temporary_changes;
  pthread_mutex_t m_mutex;
  // copy constructor and assignment operator are prohibited
  Table( const Table & );
  Table &operator=( const Table & );

public:
  Table( const std::string &name );
  ~Table();
  Table() : m_name("default"), m_mutex(PTHREAD_MUTEX_INITIALIZER) { }

  std::string get_name() const { return m_name; }

  void lock();
  void unlock();
  bool trylock();

  // Note: these functions should only be called while the
  // table's lock is held!
  void set( const std::string &key, const std::string &value );
  bool has_key( const std::string &key );
  std::string get( const std::string &key );
  void commit_changes();
  void rollback_changes();
  void erase(const std::string &key);
};

#endif // TABLE_H
