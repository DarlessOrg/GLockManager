
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include "g_lock_manager.h"

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
  for(int ix = 0; ix < ITERATIONS; ix++) {
    if(g_lock_start_session(basic_lock)) {
      printf("Doing some stuff with basic lock %d\n", ix);
      usleep(SLEEP_TIME);
      g_lock_end_session(basic_lock);
    }
  }
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
  for(int ix = 0; ix < ITERATIONS; ix++) {
    g_lock_start_session(rec_lock);
    g_lock_start_session(rec_lock);
    g_lock_start_session(rec_lock);
    g_lock_start_session(rec_lock);
    printf("Doing some stuff with recursive lock %d\n", ix);
    usleep(SLEEP_TIME);
    g_lock_end_session(rec_lock);
    g_lock_end_session(rec_lock);
    g_lock_end_session(rec_lock);
    g_lock_end_session(rec_lock);
  }
}

/**
 * Thread which uses the read portion of the rw lock
 */
static void _read_thread()
{
  for(int ix = 0; ix < ITERATIONS; ix++) {
    if(g_lock_start_read_session(rw_lock)) {
      printf("Doing some stuff with (read) rw lock %d\n", ix);
      usleep(SLEEP_TIME);
      g_lock_end_read_session(rw_lock);
    }
  }
}

/**
 * Thread which uses the write portion of the rw lock
 */
static void _write_thread()
{
  for(int ix = 0; ix < ITERATIONS; ix++) {
    if(g_lock_start_write_session(rw_lock)) {
      printf("Doing some stuff with (write) rw lock %d\n", ix);
      usleep(SLEEP_TIME);
      g_lock_end_write_session(rw_lock);
    }
  }
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
  // Create a couple of different locks
  basic_lock = g_lock_create_mutex("simpleton");
  rec_lock = g_lock_create_recursive("recursive");
  rw_lock = g_lock_create_rw("i-can-do-things");
  g_lock_show_all();

  // Create a couple of threads for each lock
  GThread *basic1 = g_thread_new("basic1", (GThreadFunc)_basic_thread, NULL);
  GThread *basic2 = g_thread_new("basic2", (GThreadFunc)_basic_thread, NULL);

  GThread *rec1 = g_thread_new("rec1", (GThreadFunc)_rec_thread, NULL);
  GThread *rec2 = g_thread_new("rec2", (GThreadFunc)_rec_thread, NULL);

  GThread *read1 = g_thread_new("read1", (GThreadFunc)_read_thread, NULL);
  GThread *read2 = g_thread_new("read2", (GThreadFunc)_read_thread, NULL);
  GThread *read3 = g_thread_new("read3", (GThreadFunc)_read_thread, NULL);
  GThread *read4 = g_thread_new("read4", (GThreadFunc)_read_thread, NULL);

  GThread *write1 = g_thread_new("write1", (GThreadFunc)_write_thread, NULL);

  for(int ix = 0; ix < 5; ix++) {
    g_lock_show_all();
    sleep(1);
  }

  // Wait for the threads to finish
  g_thread_join(basic1);
  g_thread_join(basic2);
  g_thread_join(rec1);
  g_thread_join(rec2);
  g_thread_join(read1);
  g_thread_join(read2);
  g_thread_join(read3);
  g_thread_join(read4);
  g_thread_join(write1);

  g_lock_show_all();
  g_lock_free_all();
  return 0;
}
