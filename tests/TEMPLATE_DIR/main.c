
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "../../g_lock_manager.h"

/**
 * Define locks
 */

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
 * Main function that will be called at the time of execution
 *
 * @param argc How many arguments were passed in
 * @param argv The command line arguments
 * @return On success 0 is returned, otherwise 1.
 */
int main(int argc, char **argv)
{
  // DEFINE LOGIC
}
