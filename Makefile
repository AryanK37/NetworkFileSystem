CC=gcc
EXTRA_CFLAGS= -lvlc

# release build, with max optimization flag
rel: CFLAGS=-O3
rel: client nserver sserver

# debug build, with flags to catch issues
dev: CFLAGS=-Wall -Wextra -Werror -g -fsanitize=address,undefined -fno-omit-frame-pointer
dev: client nserver sserver

client: src/client.c src/sockutils.c src/pathutils.c src/audiostream.c src/try-trie/wrapper_opt.c src/try-trie/trie.c src/try-trie/lru_cache.c
	$(CC) $(CFLAGS) $^ -o $@ $(EXTRA_CFLAGS)

nserver: src/nserver.c src/sockutils.c src/pathutils.c src/try-trie/wrapper_opt.c src/try-trie/trie.c src/try-trie/lru_cache.c
	$(CC) $(CFLAGS) $^ -o $@ $(EXTRA_CFLAGS)

sserver: src/sserver.c src/sockutils.c src/pathutils.c src/audiostream.c src/try-trie/wrapper_opt.c src/try-trie/trie.c src/try-trie/lru_cache.c
	$(CC) $(CFLAGS) $^ -o $@ $(EXTRA_CFLAGS)

# clean command
clean:
	rm client nserver sserver
