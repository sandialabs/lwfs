/**  
 *   @file mt_hashtable.h
 *   
 *   @brief A thread-safe hashtable. 
 *
 *   This implementation of a thread-safe hash table wraps pthread
 *   locks around Christopher Clark's hashtable implementation.
 * 
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   $Revision: 791 $
 *   $Date: 2006-06-29 21:32:24 -0600 (Thu, 29 Jun 2006) $
 *
 */

#include "mt_hashtable.h"
#include "hashtable.h"
#include "hashtable_itr.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

/* import the global error list */
/*
const char *sys_errlist[]; 
int sys_nerr;
int errno;
*/

/**
 * @brief Create a thread-safe hash table.
   
 * @param   minsize         minimum initial size of hashtable
 * @param   hashfunction    function for hashing keys
 * @param   key_eq_fn       function for determining key equality
 * @param   mt_table        the table to initialize
 * @return                  non-zero if successfull
 */
int 
create_mt_hashtable(unsigned int minsize,
		unsigned int (*hashfunction) (void*),
		int (*key_eq_fn) (void*,void*),
		struct mt_hashtable *mt_table)
{
	int rc = 1;  /* return code */

	/* initialize the mt_table */
	memset(mt_table, 0, sizeof(struct mt_hashtable));

	/* returns 1 on success */
	rc = create_hashtable(minsize, hashfunction, key_eq_fn, &mt_table->table); 
	if (rc != 1) {
		return 0;
	}

	/* create a pthread mutex for the table (use default attr). */
	rc = pthread_mutex_init(&mt_table->mutex, NULL); 
	if (rc != 0) {
		fprintf(stderr, "failed to create mutex (%s)\n", 
				strerror(rc));
		return 0; 
	}

	return 1; 
}

/**
 * @brief Insert into a thread-safe hashtable.
 *
 * @param   h   the hashtable to insert into
 * @param   k   the key - hashtable claims ownership and will free on removal
 * @param   v   the value - does not claim ownership
 * @return      non-zero for successful insertion
 *
 * This function will cause the table to expand if the insertion would take
 * the ratio of entries to table size over the maximum load factor.
 *
 * This function does not check for repeated insertions with a duplicate key.
 * The value returned when using a duplicate key is undefined -- when
 * the hashtable changes size, the order of retrieval of duplicate key
 * entries is reversed.
 * If in doubt, remove before insert.
 */
int mt_hashtable_insert(struct mt_hashtable *h, void *k, void *v)
{
	int rc; 

	/* lock the hashtable for the insert */
	rc = pthread_mutex_lock(&h->mutex); 
	if (rc != 0) {
		fprintf(stderr, "failed to lock hashtable: %s",
				strerror(rc));
		return 0; 
	}

	/* insert the item into the underlying table */
	rc = hashtable_insert(&h->table, k, v); 
	if (!rc) {
		fprintf(stderr, "failed to insert: rc=%d\n",rc);
		return rc; 
	}

	/* release the lock on the hashtable */
	rc = pthread_mutex_unlock(&h->mutex);
	if (rc != 0) {
		fprintf(stderr, "failed to unlock hashtable: %s\n",
				strerror(rc));
		return 0;
	}

	/* success */
	return 1;
}

/**
 * @brief Search for an entry in the hash table. 
 *
 * @comment We may want to add a function pointer to this command 
 * that executes the function on the object before we unlock the 
 * table.  For example, the user may want to lock access to the object
 * before unlocking the table. 
 *
 * @param   h   the hashtable to search
 * @param   k   the key to search for  - does not claim ownership
 * @return      the value associated with the key, or NULL if none found
 */
void *mt_hashtable_search(struct mt_hashtable *h, void *k)
{
	int rc; 
	void *data = NULL;

	/* lock the hashtable */
	rc = pthread_mutex_lock(&h->mutex); 
	if (rc != 0) {
		fprintf(stderr, "failed to lock hashtable: %s",
				strerror(rc));
		return NULL; 
	}

	/* search the underlying hashtable */
	data = hashtable_search(&h->table, k); 

	/* release the lock on the hashtable */
	rc = pthread_mutex_unlock(&h->mutex);
	if (rc != 0) {
		fprintf(stderr, "failed to unlock hashtable: %s\n",
				strerror(rc));
		return NULL;
	}

	return data;
}


/**
 * @brief Remove an entry from the hash table. 
 *
 * This function does not de-allocate the memory for the 
 * data in the entry. 
 *
 * @name        hashtable_remove
 * @param   h   the hashtable to remove the item from
 * @param   k   the key to search for  - does not claim ownership
 * @return      the value associated with the key, or NULL if none found
 */
void * mt_hashtable_remove(struct mt_hashtable *h, void *k)
{
	int rc; 
	void *data = NULL;

	/* lock the hashtable */
	rc = pthread_mutex_lock(&h->mutex); 
	if (rc != 0) {
		fprintf(stderr, "failed to lock hashtable: %s",
				strerror(rc));
		return NULL; 
	}

	data = hashtable_remove(&h->table, k); 

	/* release the lock on the hashtable */
	rc = pthread_mutex_unlock(&h->mutex);
	if (rc != 0) {
		fprintf(stderr, "failed to unlock hashtable: %s\n",
				strerror(rc));
		return NULL;
	}

	/* success */
	return data;
}

/**
 * @brief Count the number of items in the hashtable.
 *  
 * @param   h   the hashtable
 * @return      the number of items stored in the hashtable
 */
unsigned int
mt_hashtable_count(struct mt_hashtable *h)
{
	int rc; 
	int count; 

	/* lock the hashtable */
	rc = pthread_mutex_lock(&h->mutex); 
	if (rc != 0) {
		fprintf(stderr, "failed to lock hashtable: %s",
				strerror(rc));
		return -1; 
	}

	count = hashtable_count(&h->table); 
	
	/* release the lock on the hashtable */
	rc = pthread_mutex_unlock(&h->mutex);
	if (rc != 0) {
		fprintf(stderr, "failed to unlock hashtable: %s\n",
				strerror(rc));
		return -1;
	}

	/* success */
	return count;
}

/**
 * @brief Destroy a hashtable. 
 *  
 * @param   h            the hashtable
 * @param   dealloc_fn   function that de-allocates the data 
 *                       (set to NULL for no-op).
 */
void mt_hashtable_destroy(struct mt_hashtable *h, void (*dealloc_fn)(void *))
{
	int rc; 
	struct hashtable_itr *itr; 
	void *key; 
	void *val; 
	
	/* lock the hashtable */
	rc = pthread_mutex_lock(&h->mutex); 
	if (rc != 0) {
		fprintf(stderr, "failed to lock hashtable: %s",
				strerror(rc));
		return; 
	}

	{
		/* iterate through the hashtable removing each item individually */
		itr = hashtable_iterator(&h->table); 
		if (hashtable_count(&h->table) > 0) {
			do {
				key = hashtable_iterator_key(itr);
				val = hashtable_iterator_value(itr);

				/* free the value by calling the supplied func */
				if (dealloc_fn != NULL) dealloc_fn(val);

				/* the remove function also advances the iterator */
			} while (hashtable_iterator_remove(itr));
		}

		hashtable_destroy(&h->table, 0);

		// ADDED BY RON
		free(itr);
	}
	
	/* release the lock on the hashtable */
	rc = pthread_mutex_unlock(&h->mutex);
	if (rc != 0) {
		fprintf(stderr, "failed to unlock hashtable: %s\n",
				strerror(rc));
		return;
	}

	/* success */
	return;
}
