
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "../../g_lock_manager.h"

/**
 * Define locks
 */
GLock *lock1 = NULL;
GLock *lock2 = NULL;

#define ITERATIONS 10
#define SLEEP_TIME 100000 // 100ms

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
 * Main function that will be called at the time of execution
 *
 * @param argc How many arguments were passed in
 * @param argv The command line arguments
 * @return On success 0 is returned, otherwise 1.
 */
int main(int argc, char **argv)
{
  _test_deadlock();
}
