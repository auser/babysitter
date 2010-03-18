#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _HashTable_ {
	HashTableEntry **table;
	int table_size;
	HashTableHashFunc hash_func;
	HashTableEqualFunc equal_func;
	HashTableKeyFreeFunc key_free_func;
	HashTableValueFreeFunc value_free_func;
	int entries;
	int prime_index;
} HashTable;

static const unsigned int primes_for_hash_table[] = { 193, 389, 769, 1543, 3079, 6151, 12289, 24593, 49157, 98317, 196613, 393241, 786433, 
  1572869, 3145739, 6291469, 12582917, 25165843, 50331653, 100663319, 201326611, 402653189, 805306457, 1610612741,
};
static const int hash_table_num_primes = sizeof(primes_for_hash_table) / sizeof(int);
	
typedef struct _HashTableIterator_ {
  HashTable *hash_table;
  HashTableEntry *next_entry;
  int next_chain;
} HashTableIterator;
typedef struct _HashTableEntry_ {
  HashTableKey key;
	HashTableValue value;
	HashTableEntry *next;
} HashTableEntry;

typedef void *HashTableKey;
typedef void *HashTableValue;

#define HASH_TABLE_NULL ((void *) 0)

typedef unsigned long (*HashTableHashFunc)(HashTableKey value);
typedef int   (*HashTableEqualFunc)(HashTableKey value1, HashTableKey value2);
typedef void  (*HashTableKeyFreeFunc)(HashTableKey value);
typedef void (*HashTableValueFreeFunc)(HashTableValue value);

static int hash_table_allocate_table(HashTable *hash_table)
{
	int new_table_size;
	if (hash_table->prime_index < hash_table_num_primes)
		new_table_size = primes_for_hash_table[hash_table->prime_index];
	else
		new_table_size = hash_table->entries * 10;

	hash_table->table_size = new_table_size;

	hash_table->table = calloc(hash_table->table_size, sizeof(HashTableEntry *));

	return hash_table->table != NULL;
}

// Create a new hash table
static inline HashTable *hash_table_new(HashTableHashFunc hash_func, HashTableEqualFunc equal_func)
{
  HashTable *hash_table = (HashTable *) malloc(sizeof(HashTable));

	if (hash_table == NULL) {
		return NULL;
	}

	hash_table->hash_func = hash_func;
	hash_table->equal_func = equal_func;
	hash_table->key_free_func = NULL;
	hash_table->value_free_func = NULL;
	hash_table->entries = 0;
	hash_table->prime_index = 0;

	if (!hash_table_allocate_table(hash_table)) {
		free(hash_table);
		return NULL;
	}

	return hash_table;
}

static inline void hash_table_free(HashTable *hash_table)
{
  HashTableEntry *rover;
	HashTableEntry *next;
	int i;

	for (i=0; i<hash_table->table_size; ++i) {
		r = hash_table->table[i];
		while (r != NULL) {
			next = r->next;
			hash_table_free_entry(hash_table, r);
			r = next;
		}
	}
	free(hash_table->table);
	free(hash_table);
}
static inline void hash_table_register_free_functions(HashTable *hash_table, HashTableKeyFreeFunc key_free_func, HashTableValueFreeFunc value_free_func)
{
  hash_table->key_free_func = key_free_func;
	hash_table->value_free_func = value_free_func;
}

// Insert key into the hash_table
static inline int hash_table_insert(HashTable *hash_table, HashTableKey key, HashTableValue value)
{
  HashTableEntry *rover;
	HashTableEntry *newentry;
	int index;

	if ((hash_table->entries * 3) / hash_table->table_size > 0) {
		if (!hash_table_enlarge(hash_table)) {
			return 0;
		}
	}

	index = hash_table->hash_func(key) % hash_table->table_size;

	rover = hash_table->table[index];

	while (rover != NULL) {
		if (hash_table->equal_func(rover->key, key) != 0) {
			if (hash_table->value_free_func != NULL) {
				hash_table->value_free_func(rover->value);
			}

			if (hash_table->key_free_func != NULL) {
				hash_table->key_free_func(rover->key);
			}

			rover->key = key;
			rover->value = value;

			return 1;
		}
		rover = rover->next;
	}

	newentry = (HashTableEntry *) malloc(sizeof(HashTableEntry));

	if (newentry == NULL) {
		return 0;
	}

	newentry->key = key;
	newentry->value = value;

	newentry->next = hash_table->table[index];
	hash_table->table[index] = newentry;

	++hash_table->entries;

	return 1;
}

static inline HashTableValue hash_table_lookup(HashTable *hash_table, HashTableKey key)
{
  HashTableEntry *rover;
	int index = hash_table->hash_func(key) % hash_table->table_size;

	rover = hash_table->table[index];

	while (rover != NULL) {
		if (hash_table->equal_func(key, rover->key) != 0) return rover->value;
		rover = rover->next;
	}
	return HASH_TABLE_NULL;
}

static inline int hash_table_remove(HashTable *hash_table, HashTableKey key)
{
  HashTableEntry **rover;
	HashTableEntry *entry;
	int index = hash_table->hash_func(key) % hash_table->table_size;
  int result = 0;

	rover = &hash_table->table[index];

	while (*rover != NULL) {
		if (hash_table->equal_func(key, (*rover)->key) != 0) {
			entry = *rover;
			*rover = entry->next;
			hash_table_free_entry(hash_table, entry);
			--hash_table->entries;
			result = 1;
			break;
		}
		rover = &((*rover)->next);
	}

	return result;
}

static inline int hash_table_num_entries(HashTable *hash_table) {return hash_table->entries;}

static inline void hash_table_iterate(HashTable *hash_table, HashTableIterator *iter)
{
  int chain;
	iterator->hash_table = hash_table;
	iterator->next_entry = NULL;

	for (chain=0; chain<hash_table->table_size; ++chain) {
		if (hash_table->table[chain] != NULL) {
			iterator->next_entry = hash_table->table[chain];
			iterator->next_chain = chain;
			break;
		}
	}
}

static inline int hash_table_more_to_iterate(HashTableIterator *iterator) { return iterator->next_entry != NULL; }

static inline HashTableValue hash_table_iter_next(HashTableIterator *iterator)
{
  HashTableEntry *current_entry;
	HashTable *hash_table;
	HashTableValue result;
	int chain;

	hash_table = iterator->hash_table;
	
	if (iterator->next_entry == NULL) return HASH_TABLE_NULL;
	
	current_entry = iterator->next_entry;
	result = current_entry->value;

	if (current_entry->next != NULL) {
		iterator->next_entry = current_entry->next;
	} else {
		chain = iterator->next_chain + 1;
		iterator->next_entry = NULL;

		while (chain < hash_table->table_size) {
			if (hash_table->table[chain] != NULL) {
				iterator->next_entry = hash_table->table[chain];
				break;
			}
			++chain;
		}

		iterator->next_chain = chain;
	}

	return result;
}

static inline HashTableEntry *hash_table_iter_next_entry(HashTableIterator *iterator)
{
  HashTableEntry *current_entry;
	HashTable *hash_table;
	HashTableValue result;
	int chain;

	hash_table = iterator->hash_table;

	if (iterator->next_entry == NULL) return HASH_TABLE_NULL;

	current_entry = iterator->next_entry;
	result = current_entry->value;

	if (current_entry->next != NULL) {
		iterator->next_entry = current_entry->next;
	} else {
		chain = iterator->next_chain + 1;
		iterator->next_entry = NULL;

		while (chain < hash_table->table_size) {

			if (hash_table->table[chain] != NULL) {
				iterator->next_entry = hash_table->table[chain];
				break;
			}

			++chain;
		}
		iterator->next_chain = chain;
	}
	return current_entry;
}

#ifdef __cplusplus
}
#endif


#endif