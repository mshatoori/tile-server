CXX = g++
# Use pkg-config to get compiler and linker flags
MAPNIK_CFLAGS := $(shell pkg-config --cflags mapnik)
MAPNIK_LIBS := $(shell pkg-config --libs mapnik)

# Add Boost flags (pkg-config can often find these too if libboost-dev installs .pc files)
# Or add them manually if needed, e.g. -lboost_system -lboost_thread etc.
BOOST_CFLAGS := # $(shell pkg-config --cflags boost_system boost_thread boost_filesystem boost_program_options boost_date_time)
BOOST_LIBS := # $(shell pkg-config --libs boost_system boost_thread boost_filesystem boost_program_options boost_date_time)
# For Boost, often it's simpler to just link:
BOOST_LIBS_MANUAL := -lboost_system -lboost_thread -lboost_filesystem -lboost_program_options -lboost_date_time

# Your project settings
TARGET = osm_mapnik_server
SRCDIR = src
SOURCES = $(wildcard $(SRCDIR)/*.cpp)
OBJECTS = $(SOURCES:.cpp=.o)
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -I$(SRCDIR) $(MAPNIK_CFLAGS) $(BOOST_CFLAGS)
LDFLAGS = $(MAPNIK_LIBS) $(BOOST_LIBS) $(BOOST_LIBS_MANUAL) -pthread

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

$(SRCDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: all clean