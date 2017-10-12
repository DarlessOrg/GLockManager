# GLockManager
Manage your GLib locks easier in C.

## Purpose

There are many different locks out there and different use cases for them,
but how can you manage them easily and know the state of any of
them at any given time.

## Lock Variations
* `pthread_mutex_t`
* GMutex
* GRecMutex
* GRWLock (multiple readers / 1 writer)

## Better Approach
I love GLIB but why isn't there a GLock, heck maybe there is but I haven't seen
it. Either way these types of things are good to play with. The way this works:

1. Create your lock based on the create function
2. Start a session which will keep track of lock order in a given thread
3. Take a lock
4. Unlock
5. End the sesison
6. Rinse and repeat
7. Eventually when the program ends ask the manager to cleanup all the locks.

Each lock is of type GLock and the functionality to create/end a session is pretty
similar except for the read/write but even in those cases its pretty close.
Since all the different locks still use GLock as a structure the work for the developer
is easier since you do not need to remember how to lock/unlock.

## Benefits
Since there is a manager which keeps track of the locks, you can at any point ask the
manager for information about the locks:
* What are the locks?
* How many callers are currently using a lock
* Show me the caller function/line numbers.
* Due to lock order deadlocks should not happen

Ability to see which callers have what locks is a great benefit compared
to looking at gdb at the time. In a production environment the likelyhood
that GDB is on a given instance is potentially low, compared to allowing
your code to support signal processing to print lock information.

## Lock Order
Additionally when locks are defined, each lock gets an index which gets
incremented on creation. This defines an implicit order of the locks.
When a session takes 5 locks the manager will only accept if they are
taken based on the implicit order. If locks are taken out of order this
will by default cause an abort unless `G_LOCK_ORDER_ABORT` is set to 0.
In general during development it is a good idea to abort in such cases since
if the issue can be caught during development its much better than in
production.

1. Define lock1
2. Define lock2
2. Define lock3

A thread which wants to take lock1 and lock2, will have to take it in
the specific order of lock1 then lock2. Attempting to take lock2 then lock1
will cause an abort.

Let's say a thread wants to only take lock2 and lock3. This is fine as long
as it takes lock2 then lock3.

Additionally for all lock types except for recursive locks there is an
additional sanity check to ensure you are not trying to take the same lock
you have already taken in a given session which would be a silly deadlock mistake.

## Examples
Look at the tests folder for example usage for different types of locks.
