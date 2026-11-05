CXX ?= g++
CXXFLAGS ?= -O2 -std=c++17 -Wall -Wextra
CPPFLAGS += -Ivendor $(shell pkg-config --cflags libpq libcurl openssl)
LDLIBS += $(shell pkg-config --libs libpq libcurl openssl) -pthread
SERVER_MODULES := main core auth cars records stats telegram recovery
SERVER_OBJECTS := $(addprefix build/,$(addsuffix .o,$(SERVER_MODULES)))

.PHONY: all desktop clean
all: build/carlog-server build/carlog-import

build/%.o: cpp/%.cpp cpp/core.hpp vendor/httplib.h vendor/json.hpp
	@mkdir -p build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -pthread -c $< -o $@

build/carlog-server: $(SERVER_OBJECTS)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

build/carlog-import: build/import.o build/core.o
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -lsqlite3 -o $@

desktop:
	mkdir -p build/client
	cd build/client && qmake ../../client/client.pro && $(MAKE)

clean:
	$(RM) -r build
