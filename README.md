# GLockManager
Manager your GLib locks easier in C.

Purpose
=======

There are many different locks out there and different use cases for them, but how can you manage
them easily and know the state of any of them at any given time.

Please note that this is here for learning purposes. If you do decide to play around with this
please let me know your thoughts and suggestions.

Lock Variations
---------------
* pthread_mutex_t
* GMutex
* GRecMutex
* GRWLock (multiple readers / 1 writer)

Better Approach
---------------
I love GLIB but why isn't there a GLock, heck maybe there is but I haven't seen
it. Either ways these types of things are good to play with. The way this works:

1. Create your lock based on the create function
2. Start a session (lock it)
3. End a session (unlock it)
4. Rinse and repeat
5. Eventually when the program ends ask the manager to cleanup all the locks.

Each lock is of type GLock and the functionality to create/end a session is pretty
similar except for the read/write but even in those cases its pretty close.
Since all the different locks still use GLock as a structure the work for the developer
is easier since you do not need to remember how to lock/unlock.

```c
  GLock *basic_lock = g_lock_create_mutex("simpleton");
  if(g_lock_start_session(basic_lock)) {
    printf("Lonely..I'm mister lonely...\n");
    g_lock_end_session(basic_lock);
  }
  g_lock_free_all();
```

Even for the read/write lock its pretty simple
```c
  GLock *rw_lock = g_lock_create_rw("time-sensitive");
  if(g_lock_start_read_session(rw_lock)) {
    printf("I'm just a reader, let me read!\n");
    g_lock_end_read_session(rw_lock);
  }
  if(g_lock_start_write_session(rw_lock)) {
    printf("Doing important actions with memory.\n");
    g_lock_end_write_session(rw_lock);
  }
```

Benefits
--------
Since there is a manager which keeps track of the locks, you can at any point ask the
manager for information about the locks:
* What are the locks?
* How many callers are currently using a lock
* Show me the caller function/line numbers.

This is realistically the main benefit because if you're in a deadlock scenario instead
of looking at gdb and potentially having to track down all the threads and locks to see
who is potentially the initial owner you can ask the manager and it will inform you which
lock is busy.

Additionally when defining a lock you can specify how many callers can be waiting for a
lock before declaring a deadlock. Once a deadlock has been reached then no
new sessions can be created. At the moment this will not correct a deadlock but
it will also not allow it to spiral out of control where you might have 500 callers.

Examples
--------
Look at main.c for basic usage of this.
