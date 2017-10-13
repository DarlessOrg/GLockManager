#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <syslog.h>
#include <pwd.h>

#include "g_lock_manager.h"

static GLockManager _manager;

/**
 * Initialize the manager structure
 */
void g_lock_manager_init()
{
  memset(&_manager, 0, sizeof(GLockManager));
  _manager.allow_wrong_order = false;
}

/**
 * Cleanup the manager
 */
void g_lock_manager_free()
{
  g_lock_free_all();
}

/**
 * Set debugging for the lock manager
 *
 * @param debug What to set the debug to
 */
void g_lock_manager_set_debug(bool debug)
{
  _manager.debug = debug;
}

/**
 * Change whether a lock can be taken out of order
 */
void g_lock_manager_allow_wrong_order(bool allow)
{
  _manager.allow_wrong_order = allow;

}

#define lock_log(...) _log(false, __FUNCTION__, __LINE__, __VA_ARGS__)
#define lock_debug(...) _log(true, __FUNCTION__, __LINE__, __VA_ARGS__)

/**
 * Log function
 *
 * @param is_debug Whether this is a debug log message
 * @param func The caller function
 * @param line The caller line number
 * @param message The message to log
 */
static void _log(
  bool is_debug,
  const char *func,
  uint32_t line,
  const char *message,
  ...
  )
{
  int done;
  va_list ap;
  size_t size = 0;
  char *buf = NULL;
  if(is_debug && !_manager.debug) {
    return;
  }
  va_start(ap, message);
  done = vsnprintf(NULL, 0, message, ap);
  va_end(ap);
  if(done < 0) {
    syslog(LOG_USER | LOG_ERR, "%s: vsnprintf failed!", __FUNCTION__);
    return;
  }
  size = done + 1;
  buf = (char *)malloc(size * sizeof(char));
  if(!buf) {
    syslog(LOG_USER | LOG_ERR, "%s: memory exhausted", __FUNCTION__);
    return;
  }
  va_start(ap, message);
  done = vsnprintf(buf, size, message, ap);
  va_end(ap);
  if(done >= 0) {
    syslog(LOG_USER | LOG_ERR,
      "%s: %s: %d: %s",
      is_debug? "DEBUG": "ERR",
      func, line, buf);
    printf("%s: %s: %d: %s\n",
      is_debug? "DEBUG": "ERR",
      func, line, buf);
  } else {
    syslog(LOG_USER | LOG_ERR, "%s: %d: vsnprintf failed",
      __FUNCTION__, __LINE__);
    printf("%s: %d: vsnprintf failed\n",
      __FUNCTION__, __LINE__);
  }
  free(buf);
}

/**
 * Lock the reader for the manager
 */
static void _manager_reader_lock()
{
  g_rw_lock_reader_lock(&_manager.manager_rw_lock);
}

/**
 * Unlock the reader for the manager
 */
static void _manager_reader_unlock()
{
  g_rw_lock_reader_unlock(&_manager.manager_rw_lock);
}

/**
 * Lock the writer for the manager
 */
static void _manager_writer_lock()
{
  g_rw_lock_writer_lock(&_manager.manager_rw_lock);
}

/**
 * Unlock the writer for the manager
 */
static void _manager_writer_unlock()
{
  g_rw_lock_writer_unlock(&_manager.manager_rw_lock);
}

/**
 * Create a new lock
 *
 * @param lock_name The name of the lock
 * @param type The lock type
 * @return The new lock instance or NULL on error.
 */
GLock *g_lock_create(
  const char *lock_name,
  enum g_lock_type type
  )
{
  if(!lock_name) {
    lock_log("No lock name provided");
    return NULL;
  }

  GLock *lock = calloc(1, sizeof(GLock));
  lock->name = strdup(lock_name);
  lock->type = type;

  // Initialize the main lock
  switch(type) {
    case G_LOCK_MUTEX:
      g_mutex_init(&lock->_lock.mutex);
      break;
    case G_LOCK_RECURSIVE:
      g_rec_mutex_init(&lock->_lock.rec_mutex);
      break;
    case G_LOCK_RW:
      g_rw_lock_init(&lock->_lock.rw_mutex);
      break;
  }

  // Initialize the stats lock
  g_mutex_init(&lock->stats_lock);

  // Add this lock to the global list of locks
  _manager_writer_lock();
  lock->index = _manager.lock_index++;
  _manager.locks = g_list_append(_manager.locks, lock);
  _manager_writer_unlock();
  return lock;
}

/**
 * Free the caller data
 *
 * @param data The caller data passed in
 */
static void _lock_free_caller(gpointer data)
{
  struct g_lock_caller *elem = data;
  if(!elem) {
    lock_log("No caller to free");
    return;
  }
  if(elem->caller) {
    free(elem->caller);
  }
  // Free the element itself
  free(elem);
}

/**
 * Free the memory associated to a lock
 *
 * @param data The passed in lock pointer
 */
static void _free_lock_entry(gpointer data)
{
  GLock *lock = data;
  if(!lock) {
    lock_log("No lock provided");
    return;
  }
  if(lock->name) {
    free(lock->name);
  }
  // Clear the lock based on type
  switch(lock->type) {
    case G_LOCK_MUTEX:
      g_mutex_clear(&lock->_lock.mutex);
      break;
    case G_LOCK_RECURSIVE:
      g_rec_mutex_clear(&lock->_lock.rec_mutex);
      break;
    case G_LOCK_RW:
      g_rw_lock_clear(&lock->_lock.rw_mutex);
      break;
  };
  // Clear the stats lock
  g_mutex_clear(&lock->stats_lock);

  // Free the statistics for the lock
  if(lock->stats.call_list) {
    g_list_free_full(lock->stats.call_list, _lock_free_caller);
    lock->stats.call_list = NULL;
  }

  // Free the memory of the lock itself
  free(lock);
}

/**
 * Cleanup a single lock
 *
 * This will remove it from the manager, clear
 * it from being a lock and free the associated
 * memory for it.
 *
 * @param lock The lock to cleanup
 */
void g_lock_free(GLock *lock)
{
  if(!lock) {
    lock_log("No lock provided");
    return;
  }
  // Remove it from the list
  _manager_writer_lock();
  _manager.locks = g_list_remove(_manager.locks, lock);
  _manager_writer_unlock();

  // Clear the lock and free the memory
  _free_lock_entry(lock);
}

/**
 * Cleanup all the locks known to the manager
 */
void g_lock_free_all()
{
  _manager_writer_lock();
  g_list_free_full(_manager.locks, _free_lock_entry);
  _manager.locks = NULL;
  _manager_writer_unlock();
}

/**
 * Convert the lock type to be human readable
 *
 * @param type The lock type to convert
 * @return The string representation of the lock.
 */
static const char *_lock_type_to_str(enum g_lock_type type)
{
  switch(type) {
    case G_LOCK_MUTEX:
      return "MUTEX";
    case G_LOCK_RECURSIVE:
      return "RECURSIVE";
    case G_LOCK_RW:
      return "Read/Write";
  }
  return NULL;
}

/**
 * Print the lock statistics for a given lock
 *
 * @param lock The lock to print the statistics for
 */
static void _print_lock_stats(GLock *lock)
{
  if(!lock) {
    printf("No lock provided\n");
    return;
  }
  GList *elem;
  struct g_lock_caller *caller;
  printf("=====================================\n");
  printf("Lock: %s\n", lock->name);
  printf("Type: %s\n", _lock_type_to_str(lock->type));

  g_mutex_lock(&lock->stats_lock);
  printf("Count: %d\n", lock->stats.count);
  printf("Callers\n");
  printf("-----------------------------\n");
  for(elem = lock->stats.call_list; elem; elem = elem->next) {
    caller = elem->data;
    printf("Caller: %s - %d - Timestamp: %ld\n",
      caller->caller,
      caller->line,
      caller->timestamp);
  }
  g_mutex_unlock(&lock->stats_lock);
  printf("=====================================\n");
}

/**
 * Show the statistics for all the locks
 */
void g_lock_show_all()
{
  GList *elem;
  GLock *lock;
  _manager_reader_lock();
  for(elem = _manager.locks; elem; elem = elem->next) {
    lock = elem->data;
    _print_lock_stats(lock);
  }
  _manager_reader_unlock();
}

/**
 * Log the action for the lock
 *
 * @param action What action is being performed
 * @param name The name of the lock
 * @param action_subtype The subtype of action that is being performed.
 *                       This only applies to read/write locks.
 */
static void _lock_log_action(
  const char *action,
  const char *name,
  enum g_lock_action action_subtype
  )
{
  char subtype[32];
  subtype[0] = '\0';
  switch(action_subtype) {
    case G_LOCK_ACTION_READ:
      g_strlcpy(subtype, " (RW-READ) ", sizeof(subtype));
      break;
    case G_LOCK_ACTION_WRITE:
      g_strlcpy(subtype, " (RW-WRITE) ", sizeof(subtype));
      break;
    default:
      subtype[0] = '\0';
      break;
  }
  lock_debug("%s%s: %s", action, subtype, name);
}

/**
 * Add the lock index to the session
 *
 * @param session Lock session object
 * @param lock The lock being taken
 */
static bool g_lock_session_add_lock(
  GLockSession *session,
  GLock *lock
  )
{
  if(!session) {
    lock_log("No session provided");
    return false;
  }
  if(!lock) {
    lock_log("No lock provided");
    return false;
  }
  session->lock_list = g_list_append(
    session->lock_list,
    GINT_TO_POINTER(lock->index));
  return true;
}

/**
 * If abort_lock_order is enabled then abort
 */
static void _g_lock_abort()
{
  if(!_manager.allow_wrong_order) {
    lock_log("CRITICAL: Aborting");
    abort();
  }
}

/**
 * Check if a lock is valid within a session
 *
 * @param session The session to check
 * @param lock The lock that the caller wants to take
 * @param action The action the caller is taking
 * @return If all is ok then return true otherwise false
 */
static bool _g_lock_session_check_lock(
  GLockSession *session,
  GLock *lock,
  enum g_lock_action action
  )
{
  GList *tmpl = NULL;
  char *lock_name = NULL;
  uint32_t cur_index;
  bool test_same = true;

  if(!session || !lock) {
    return false;
  }

  switch(lock->type) {
    case G_LOCK_RECURSIVE:
      test_same = false;
    default:
      break;
  };


  for(tmpl = session->lock_list; tmpl; tmpl = tmpl->next) {
    cur_index = GPOINTER_TO_INT(tmpl->data);
    if(test_same) {
      if(lock->index == cur_index) {
        lock_name = g_lock_name_by_index(cur_index);
        lock_log(
          "CRITICAL: [LOCK ORDER] "
          "Attempting to take lock index %u (%s) which has already "
          "been taken in this session.",
          lock->index, lock->name);
        if(lock_name) {
          free(lock_name);
        }
        _g_lock_abort();
        return false;
      }
    }
    // Check if the lock was taken out of order
    if(lock->index < cur_index) {
      lock_name = g_lock_name_by_index(cur_index);
      lock_log(
        "CRITICAL: [LOCK_ORDER] "
        "Attempting to take lock index %u (%s) which is less "
        "then already taken lock index %u (%s).",
        lock->index, lock->name,
        cur_index, lock_name);
      if(lock_name) {
        free(lock_name);
      }
      _g_lock_abort();
      return false;
    }
  }
  return true;
}

/**
 * Start a new session for the lock
 *
 * @param session The lock session
 * @param lock The lock to create the session for
 * @param action The action to perform (for read/write locks)
 * @param caller_func The caller's function name
 * @param caller_line The caller's line number
 * @return On success true is returned otherwise false.
 */
bool _g_lock_start(
  GLockSession *session,
  GLock *lock,
  enum g_lock_action action,
  const char *caller_func,
  uint32_t caller_line
  )
{
  if(!session) {
    lock_log("No session provided");
    return false;
  }
  if(!lock) {
    lock_log("No lock provided");
    return false;
  }
  struct g_lock_caller *caller = calloc(1, sizeof(struct g_lock_caller));
  if(!caller) {
    lock_log("Failed to create caller");
    return false;
  }
  caller->caller = strdup(caller_func);
  caller->line = caller_line;
  caller->timestamp = time(NULL);
  caller->session = session;

  // Check if we're taking a lock out of order
  if(!_g_lock_session_check_lock(session, lock, action)) {
    return false;
  }

  // Update the statistics for the lock
  g_mutex_lock(&lock->stats_lock);
  lock->stats.count++;
  lock->stats.call_list = g_list_append(lock->stats.call_list, caller);
  g_mutex_unlock(&lock->stats_lock);

  // Update our session information with this lock
  g_lock_session_add_lock(session, lock);

  // Perform the lock based on the action
  _lock_log_action("LOCKING", lock->name, action);
  switch(lock->type) {
    case G_LOCK_MUTEX:
      g_mutex_lock(&lock->_lock.mutex);
      break;
    case G_LOCK_RECURSIVE:
      g_rec_mutex_lock(&lock->_lock.rec_mutex);
      break;
    case G_LOCK_RW:
      if(action == G_LOCK_ACTION_READ) {
        g_rw_lock_reader_lock(&lock->_lock.rw_mutex);
      } else {
        g_rw_lock_writer_lock(&lock->_lock.rw_mutex);
      }
      break;
  };
  _lock_log_action("LOCKED", lock->name, action);
  return true;
}

/**
 * Search for a caller to remove from the lock's caller list
 *
 * @param session The current caller's session
 * @param lock The lock that is being unlocked
 */
static void _g_lock_remove_caller(
  GLockSession *session,
  GLock *lock
  )
{
  if(!session) {
    lock_log("session is NULL");
    return;
  }
  if(!lock) {
    lock_log("lock is NULL");
    return;
  }
  if(lock->stats.call_list == NULL) {
    return;
  }

  GList *tmpl = NULL;
  struct g_lock_caller *callerp = NULL;
  for(tmpl = lock->stats.call_list; tmpl; tmpl = tmpl->next) {
    callerp = tmpl->data;
    if(callerp->session == session) {
      lock_debug("Found matching caller. %p == %p",
          session,
          callerp->session);
      _lock_free_caller(tmpl->data);
      lock->stats.call_list = g_list_delete_link(
        lock->stats.call_list, tmpl);
      return;
    }
  }
  lock_log("ERROR: Did not find matching caller session %p", session);
}

/**
 * End the session for a given lock
 *
 * @param session The lock session
 * @param lock The lock in question
 * @param action The action to perform (for read/write locks)
 * @param caller_func The caller's function
 * @param caller_line The caller's line number
 */
void _g_lock_end(
  GLockSession *session,
  GLock *lock,
  enum g_lock_action action,
  const char *caller_func,
  uint32_t caller_line
  )
{
  if(!session) {
    lock_log("No session provided");
    return;
  }
  if(!lock) {
    lock_log("No lock provided");
    return;
  }

  // Update the statistics for the lock
  g_mutex_lock(&lock->stats_lock);

  lock->stats.count--;
  _g_lock_remove_caller(session, lock);

  g_mutex_unlock(&lock->stats_lock);

  _lock_log_action("UNLOCKING", lock->name, action);

  // Perform the lock based on the action
  switch(lock->type) {
    case G_LOCK_MUTEX:
      g_mutex_unlock(&lock->_lock.mutex);
      break;
    case G_LOCK_RECURSIVE:
      g_rec_mutex_unlock(&lock->_lock.rec_mutex);
      break;
    case G_LOCK_RW:
      if(action == G_LOCK_ACTION_READ) {
        g_rw_lock_reader_unlock(&lock->_lock.rw_mutex);
      } else {
        g_rw_lock_writer_unlock(&lock->_lock.rw_mutex);
      }
      break;
  };

  // Pop the last lock from the session object
  session->lock_list = g_list_remove_link(
    session->lock_list,
    g_list_last(session->lock_list));

  _lock_log_action("UNLOCKED", lock->name, action);
}

/**
 * Get lock name based on lock index
 *
 * The caller must free the result
 *
 * @param index The index to look for
 * @return Allocated name of the lock if found otherwise NULL
 */
char *g_lock_name_by_index(uint32_t index)
{
  char *name = NULL;
  GLock *lock;
  GList *tmpl = NULL;
  _manager_reader_lock();
  for(tmpl = _manager.locks; tmpl; tmpl = tmpl->next) {
    lock = tmpl->data;
    if(lock->index == index) {
      name = strdup(lock->name);
    }
  }
  _manager_reader_unlock();
  return name;
}

/**
 * Create a session
 *
 * @return New instance of GLockSession or NULL if we are out of memory
 */
GLockSession *g_lock_session_new()
{
  GLockSession *session = calloc(1, sizeof(GLockSession));
  return session;
}

/**
 * Free a session
 *
 * @param session The session to free
 */
void g_lock_session_free(GLockSession *session)
{
  if(session) {
    if(session->lock_list) {
      g_list_free(session->lock_list);
      session->lock_list = NULL;
    }
    free(session);
  }
}
