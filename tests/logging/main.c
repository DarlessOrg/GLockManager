
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include "../../g_lock_manager.h"

/**
 * Define locks
 */
GLock *basic_lock = NULL;


#define ITERATIONS 1
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
 * Main function that will be called at the time of execution
 *
 * @param argc How many arguments were passed in
 * @param argv The command line arguments
 * @return On success 0 is returned, otherwise 1.
 */
int main(int argc, char **argv)
{
  // Test with normal logging
  basic_lock = g_lock_create_mutex("simpleton");
  GThread *basic1;

  basic1 = g_thread_new("basic1", (GThreadFunc)_basic_thread, NULL);
  g_thread_join(basic1);

  // Enable debug
  g_lock_manager_set_debug(true);
  basic1 = g_thread_new("basic1", (GThreadFunc)_basic_thread, NULL);
  g_thread_join(basic1);
}
