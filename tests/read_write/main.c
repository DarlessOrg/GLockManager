
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "../../g_lock_manager.h"

/**
 * Define locks
 */
GLock *rw_lock = NULL;

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
 * Main function that will be called at the time of execution
 *
 * @param argc How many arguments were passed in
 * @param argv The command line arguments
 * @return On success 0 is returned, otherwise 1.
 */
int main(int argc, char **argv)
{
  _test_readwrite_lock();
}
