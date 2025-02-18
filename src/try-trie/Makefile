CC = gcc
CFLAGS = -Wall -Wextra -g

all: simplewrapper simpletrie simplelru

simplewrapper: test_wrapper.c wrapper_opt.c trie.c lru_cache.c
	$(CC) $(CFLAGS) -o simplewrapper test_wrapper.c wrapper_opt.c trie.c lru_cache.c

simpletrie: test_trie.c trie.c
	$(CC) $(CFLAGS) -o simpletrie test_trie.c trie.c

simplelru: test_lru.c lru_cache.c
	$(CC) $(CFLAGS) -o simplelru test_lru.c lru_cache.c

clean:
	rm -f simplewrapper simpletrie simplelru
