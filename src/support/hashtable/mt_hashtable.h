/**  
 *   @file mt_hashtable.h
 *   
 *   @brief Prototypes for a thread-safe hashtable. 
 * 
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   $Revision: 791 $
 *   $Date: 2006-06-29 21:32:24 -0600 (Thu, 29 Jun 2006) $
 *
 */

#ifndef _MT_HASHTABLE_H_
#define _MT_HASHTABLE_H_

#include <pthread.h>
#include "hashtable.h"

/**
 * @brief Structure definition for a thread-safe hash table.
 *
 * Our implementation is layered on top of Christopher Clark's 
 * hashtable. We make our version thread safe by locking a mutex 
 * before calling the underlying method. 
 */
struct mt_hashtable {
	/** @brief the non-thread-safe hashtable.  */
	struct hashtable table;

	/** @brief the mutex used to synchronize access.  */
	pthread_mutex_t mutex; 
}; 

/**
 * @brief Create a thread-safe hash table.
   
 * @param   minsize         minimum initial size of hashtable
 * @param   hashfunction    function for hashing keys
 * @param   key_eq_fn       function for determining key equality
 * @param   table           the table to create
 * @return                  0 if successful, errno otherwise. 
 */
int create_mt_hashtable(unsigned int minsize,
                 unsigned int (*hashfunction) (void*),
                 int (*key_eq_fn) (void*,void*),
		 struct mt_hashtable *table);

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
int mt_hashtable_insert(struct mt_hashtable *h, void *k, void *v);

/**
 * @brief Search for an entry in the hash table. 
 *
 * @param   h   the hashtable to search
 * @param   k   the key to search for  - does not claim ownership
 * @return      the value associated with the key, or NULL if none found
 */
void *mt_hashtable_search(struct mt_hashtable *h, void *k);


/**
 * @brief Remove an entry from the hash table. 
 *
 * @name        hashtable_remove
 * @param   h   the hashtable to remove the item from
 * @param   k   the key to search for  - does not claim ownership
 * @return      the value associated with the key, or NULL if none found
 */
void * mt_hashtable_remove(struct mt_hashtable *h, void *k);


/**
 * @brief Count the number of items in the hashtable.
 *  
 * @param   h   the hashtable
 * @return      the number of items stored in the hashtable
 */
unsigned int
mt_hashtable_count(struct mt_hashtable *h);


/**
 * @brief Destroy a hashtable. 
 *  
 * @param   h            the hashtable
 * @param   dealloc_fn   function that de-allocates the data 
 *                       (set to NULL for no-op).
 */
void mt_hashtable_destroy(struct mt_hashtable *h, void (*dealloc_fn)(void *));


#define DEFINE_MT_HASHTABLE_INSERT(fnname, keytype, valuetype) \
int fnname (struct mt_hashtable *h, keytype *k, valuetype *v) \
{ \
    return mt_hashtable_insert(h,k,v); \
}
#define DEFINE_MT_HASHTABLE_SEARCH(fnname, keytype, valuetype) \
valuetype * fnname (struct mt_hashtable *h, keytype *k) \
{ \
    return (valuetype *) (mt_hashtable_search(h,k)); \
}
#define DEFINE_MT_HASHTABLE_REMOVE(fnname, keytype, valuetype) \
valuetype * fnname (struct mt_hashtable *h, keytype *k) \
{ \
    return (valuetype *) (mt_hashtable_remove(h,k)); \
}

#endif 