
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "../../g_lock_manager.h"

/**
 * Define locks
 */
GLock *rec_lock = NULL;

#define ITERATIONS 10
#define SLEEP_TIME 100000 // 100ms

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
 * Main function that will be called at the time of execution
 *
 * @param argc How many arguments were passed in
 * @param argv The command line arguments
 * @return On success 0 is returned, otherwise 1.
 */
int main(int argc, char **argv)
{
  _test_recursive_lock();
}
