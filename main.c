
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "g_lock_manager.h"

GLock *lock1 = NULL;
GLock *lock2 = NULL;
GLock *basic_lock = NULL;
GLock *rec_lock = NULL;
GLock *rw_lock = NULL;
#define ITERATIONS 10
#define SLEEP_TIME 100000 // 100ms

/**
 * Basic thread which uses the mutex lock
 */
static void _basic_thread()
{
  G_LOCK_SESSION_START();
  for(int ix = 0; ix < ITERATIONS; ix++) {
    if(g_lock_start(session, basic_lock)) {
      printf("Doing some stuff with basic lock %d\n", ix);
      usleep(SLEEP_TIME);
      g_lock_end(session, basic_lock);
    }
  }
  G_LOCK_SESSION_END();
}

/**
 * Thread which uses the recursive lock
 *
 * Here we are locking 4 times which is completely fine
 * with the recursive lock as long as it is unlocked as
 * many times as it was locked.
 */
static void _rec_thread()
{
  GLockSession *session = g_lock_session_new();
  for(int ix = 0; ix < ITERATIONS; ix++) {
    g_lock_start(session, rec_lock);
    g_lock_start(session, rec_lock);
    g_lock_start(session, rec_lock);
    g_lock_start(session, rec_lock);
    printf("Doing some stuff with recursive lock %d\n", ix);
    usleep(SLEEP_TIME);
    g_lock_end(session, rec_lock);
    g_lock_end(session, rec_lock);
    g_lock_end(session, rec_lock);
    g_lock_end(session, rec_lock);
  }
  g_lock_session_free(session);
}

/**
 * Thread which uses the read portion of the rw lock
 */
static void _read_thread()
{
  sleep(1);
  GLockSession *session = g_lock_session_new();
  for(int ix = 0; ix < ITERATIONS; ix++) {
    if(g_lock_start_read(session, rw_lock)) {
      printf("Doing some stuff with (read) rw lock %d\n", ix);
      usleep(SLEEP_TIME);
      g_lock_end_read(session, rw_lock);
    }
  }
  g_lock_session_free(session);
}

/**
 * Thread which uses the write portion of the rw lock
 */
static void _write_thread()
{
  sleep(1);
  GLockSession *session = g_lock_session_new();
  for(int ix = 0; ix < ITERATIONS; ix++) {
    if(g_lock_start_write(session, rw_lock)) {
      printf("Doing some stuff with (write) rw lock %d\n", ix);
      usleep(SLEEP_TIME);
      g_lock_end_write(session, rw_lock);
    }
  }
  g_lock_session_free(session);
}

/**
 * Show the lock state
 */
static void _show_locks()
{
  for(int ix = 0; ix < 5; ix++) {
    g_lock_show_all();
    sleep(1);
  }
}

/**
 * Basic thread which uses the mutex lock
 */
static void _dl_thread1()
{
  GLockSession *session = g_lock_session_new();
  for(int ix = 0; ix < ITERATIONS; ix++) {
    if(g_lock_start(session, lock1)) {
      if(g_lock_start(session, lock2)) {
        printf("Doing some stuff with basic lock1, lock2 %d\n", ix);
        usleep(SLEEP_TIME);
        g_lock_end(session, lock2);
      }
      g_lock_end(session, lock1);
    }
  }
}

/**
 * Basic thread which uses the mutex lock
 */
static void _dl_thread2()
{
  GLockSession *session = g_lock_session_new();
  for(int ix = 0; ix < ITERATIONS; ix++) {
    if(g_lock_start(session, lock2)) {
      if(g_lock_start(session, lock1)) {
        printf("Doing some stuff with basic lock2, lock1 %d\n", ix);
        usleep(SLEEP_TIME);
        g_lock_end(session, lock1);
      }
      g_lock_end(session, lock2);
    }
  }
  g_lock_session_free(session);
}

/**
 * Test a basic lock
 */
void _test_basic_lock()
{
  basic_lock = g_lock_create_mutex("simpleton");

  GThread *basic1 = g_thread_new("basic1", (GThreadFunc)_basic_thread, NULL);
  GThread *basic2 = g_thread_new("basic2", (GThreadFunc)_basic_thread, NULL);

  _show_locks();
  g_thread_join(basic1);
  g_thread_join(basic2);
  g_lock_show_all();
  g_lock_free_all();
}

/**
 * Test recursive locks
 */
void _test_recursive_lock()
{
  rec_lock = g_lock_create_recursive("recursive");
  GThread *rec1 = g_thread_new("rec1", (GThreadFunc)_rec_thread, NULL);
  GThread *rec2 = g_thread_new("rec2", (GThreadFunc)_rec_thread, NULL);

  _show_locks();
  g_thread_join(rec1);
  g_thread_join(rec2);
  g_lock_show_all();
  g_lock_free_all();
}

/**
 * Test read/write locks
 */
void _test_readwrite_lock()
{
  rw_lock = g_lock_create_rw("i-can-do-things");

  GThread *read1 = g_thread_new("read1", (GThreadFunc)_read_thread, NULL);
  GThread *read2 = g_thread_new("read2", (GThreadFunc)_read_thread, NULL);
  GThread *read3 = g_thread_new("read3", (GThreadFunc)_read_thread, NULL);
  GThread *read4 = g_thread_new("read4", (GThreadFunc)_read_thread, NULL);
  GThread *write1 = g_thread_new("write1", (GThreadFunc)_write_thread, NULL);
  GThread *write2 = g_thread_new("write2", (GThreadFunc)_write_thread, NULL);

  _show_locks();

  // Wait for the threads to finish
  g_thread_join(read1);
  g_thread_join(read2);
  g_thread_join(read3);
  g_thread_join(read4);
  g_thread_join(write1);
  g_thread_join(write2);

  g_lock_show_all();
  g_lock_free_all();

}

/**
 * Test deadlocks
 */
void _test_deadlock()
{
  lock1 = g_lock_create_mutex("lock1");
  lock2 = g_lock_create_mutex("lock2");

  GThread *th1 = g_thread_new("th1", (GThreadFunc)_dl_thread1, NULL);
  GThread *th2 = g_thread_new("th2", (GThreadFunc)_dl_thread2, NULL);

  _show_locks();

  g_thread_join(th1);
  g_thread_join(th2);
  g_lock_show_all();
  g_lock_free_all();
}

/**
 * Print the usage
 *
 * @param argc Number of arguments
 * @param argv Arguments
 */
static void _print_usage(int argc, char **argv)
{
  if(!argv) {
    return;
  }
  printf("Usage:\n");
  printf("%s <action>\n\n", argv[0]);
  printf("basic     - Test basic locks\n");
  printf("recursive - Test recursive locks\n");
  printf("readwrite - Test readwrite locks\n");
  printf("deadlock  - Test deadlocks\n");
}

/**
 * Main function that will be called at the time of execution
 *
 * @param argc How many arguments were passed in
 * @param argv The command line arguments
 * @return On success 0 is returned, otherwise 1.
 */
int main(int argc, char **argv)
{
  if(argc <= 1) {
    _test_basic_lock();
    _test_recursive_lock();
    _test_readwrite_lock();
    _test_deadlock();
    return 0;
  } else {
    if(!strcmp(argv[1], "basic")) {
      _test_basic_lock();
    } else if(!strcmp(argv[1], "recursive")) {
      _test_recursive_lock();
    } else if(!strcmp(argv[1], "readwrite")) {
      _test_readwrite_lock();
    } else if(!strcmp(argv[1], "deadlock")) {
      _test_deadlock();
    } else {
      _print_usage(argc, argv);
    }
  }
  return 0;
}
