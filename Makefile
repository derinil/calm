UNAME_S = $(shell uname -s)

CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -Wstrict-aliasing
CFLAGS += -Wno-unused-parameter
CFLAGS += -pthread
CFLAGS += -Ilibs/curl/include -Ilibs/libwebsockets/build/include -Ilibs/libsodium/src/libsodium/include
LDFLAGS = -lpthread -Llibs/curl/lib/.libs -lcurl_local -lm -Llibs/libwebsockets/build/lib -lwebsockets -Llibs/libsodium/src/libsodium/.libs -lsodiumstatic
LDFLAGS += -pthread

CFLAGS += -O2
# CFLAGS += -g -ggdb3
# CFLAGS += -fsanitize=address
# LDFLAGS += -fsanitize=address

ifeq ($(UNAME_S),Linux)
	CFLAGS +=
	LDFLAGS += -L/usr/local/lib
endif
ifeq ($(UNAME_S),Darwin)
	CC = clang
	CURL_HOST+=--host=arm-apple-darwin
	CFLAGS += -Wno-incompatible-pointer-types-discards-qualifiers -Wno-gnu-binary-literal
	LDFLAGS += -framework CoreServices -framework SystemConfiguration
	CFLAGS += -g
	CFLAGS += -fsanitize=address
	LDFLAGS += -fsanitize=address
endif

SRC  = $(wildcard src/**/*.c) $(wildcard src/*.c) $(wildcard src/**/**/*.c) $(wildcard src/**/**/**/*.c)
OBJ  = $(SRC:.c=.o)
BIN  = bin
EXE  = exe

BENCHMARK_SRC := $(wildcard benchmark/*.c) $(wildcard src/**/*.c) $(wildcard src/*.c) $(wildcard src/**/**/*.c) $(wildcard src/**/**/**/*.c)
BENCHMARK_SRC := $(filter-out src/main.c, $(BENCHMARK_SRC))
BENCHMARK_OBJ  = $(BENCHMARK_SRC:.c=.o)

TEST_SRC := $(wildcard test/*.c) $(wildcard src/**/*.c) $(wildcard src/*.c) $(wildcard src/**/**/*.c) $(wildcard src/**/**/**/*.c)
TEST_SRC := $(filter-out src/main.c, $(TEST_SRC))
TEST_OBJ  = $(TEST_SRC:.c=.o)

.PHONY: all run clean libs libs-clean run-bench run-test

all: dirs build

run: build
	./bin/exe

dirs:
	mkdir -p ./$(BIN)

build: $(OBJ)
	$(CC) -o $(BIN)/$(EXE) $^ $(LDFLAGS)

libs: libsodium
	cd deps/glfw

libsodium:
	cd libs/libsodium && env $(LIBSODIUM_ENV_FLAGS) ./configure && make && make check
	cp libs/libsodium/src/libsodium/.libs/libsodium.a libs/libsodium/src/libsodium/.libs/libsodiumstatic.a

libs-clean:
	cd libs/curl && make clean
	cd libs/libwebsockets/build && make clean
	cd libs/libsodium && make clean

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -rf $(BIN)/exe $(BIN)/exe_bench $(BIN)/exe_test $(OBJ)

clean-logs:
	rm prices.log openorders.log events.log balances.log
