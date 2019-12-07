CC = clang++

GTEST_DIR =  /Users/sideboard/Code/googletest/googletest



SRC = $(shell find src/ -type f -name '*.cpp')
OBJDIR = obj
OBJ = $(patsubst %.cpp, $(OBJDIR)/%.o, $(SRC))

LIBS=-lportaudio -lportmidi -lreadline -lm -lpthread -lsndfile -lprofiler -llo

ABLETONASIOINC=/Users/sideboard/Code/link/modules/asio-standalone/asio/include
READLINEINCDIR=/Users/sideboard/homebrew/opt/readline/include
INCDIRS=-I/usr/local/include -Iinclude/interpreter -Iinclude -Iinclude/afx -Iinclude/stack -I/Users/sideboard/Code/link/include -I/Users/sideboard/homebrew/include -I${HOME}/Code/range-v3/include/ -I${ABLETONASIOINC} -I${READLINEINCDIR}

HOMEBREWLIBDIR=/Users/sideboard/homebrew/lib
READLINELIBDIR=/Users/sideboard/homebrew/opt/readline/lib
LIBDIRS=-L/usr/local/lib -L${HOMEBREWLIBDIR} -L${READLINELIBDIR}

WARNFLASGS = -Wall -Wextra -pedantic -Wstrict-prototypes -Wmissing-prototypes
# Flags passed to the preprocessor.
CPPFLAGS = -std=c++17 $(WARNFLAGS) -g $(INCDIRS) -O0 -fsanitize=address -fno-omit-frame-pointer -isystem $(GTEST_DIR)/include
#CPPFLAGS = -std=c++17 $(WARNFLAGS) -g $(INCDIRS) $(ABLETONASIOINC) -O3 -fsanitize=address -fno-omit-frame-pointer -isystem $(GTEST_DIR)/include

$(OBJDIR)/%.o: %.cpp
	$(CC) -c -o $@ $< $(CPPFLAGS)

TARGET = sbsh

.PHONY: depend clean

all: objdir $(TARGET)
	@ctags -R *
	@cscope -R -b
	@echo "\n\x1b[37mBoom! make some noise...\x1b[0m"

objdir:
	mkdir -p obj/src/fx obj/src/filterz obj/src/pattern_transformers obj/src/cmdloop obj/src/pattern_generators obj/src/value_generators obj/src/afx obj/src/interpreter obj/src/tests

$(TARGET): $(OBJ)
	$(CC) $(CPPFLAGS) $(LIBDIRS) -o $@ $^ $(LIBS) $(INCDIRS)

clean:
	rm -f *.o *~ $(TARGET) $(TEST_TARGET)
	rm -rf obj/*
	@echo "\n\x1b[37mCleannnnd..!\x1b[0m"

format:
	find . -type f -name "*.[ch]" -exec clang-format -i {} \;


############### TESTzzz ################################################################
TEST_TARGET = sbsh_test
TESTS = $(wildcard tests/*.cpp)
OBJ_MINUS_MAIN = $(filter-out obj/src//main.o, $(OBJ))

GTEST_LIB_DIR = /Users/sideboard/Code/googletest/googletest/make
GTEST_LIBS = libgtest.a libgtest_main.a

#$(TEST_TARGET): $(TESTS) $(GTEST_LIBS) $(OBJ_MINUS_MAIN)
$(TEST_TARGET): tests/parsey_tests.cpp tests/evaluator_test.cpp $(GTEST_LIBS) $(OBJ_MINUS_MAIN)
	#$(info $$OBJ_MINUS_MAIN is [${OBJ_MINUS_MAIN}])
	$(info $$TEZZZTS is [${TESTS}])
	$(CC) $(CPPFLAGS) $(LIBDIRS) -L$(GTEST_LIB_DIR) -lgtest -lpthread -o $@ $^ $(LIBS) $(INCDIRS)

# All Google Test headers.  Usually you shouldn't change this
# definition.
GTEST_HEADERS = $(GTEST_DIR)/include/gtest/*.h \
                $(GTEST_DIR)/include/gtest/internal/*.h

# Builds gtest.a and gtest_main.a.
#
# Usually you shouldn't tweak such internal variables, indicated by a
# trailing _.
GTEST_SRCS_ = $(GTEST_DIR)/src/*.cc $(GTEST_DIR)/src/*.h $(GTEST_HEADERS)

# For simplicity and to avoid depending on Google Test's
# implementation details, the dependencies specified below are
# conservative and not optimized.  This is fine as Google Test
# compiles fast and for ordinary users its source rarely changes.
gtest-all.o : $(GTEST_SRCS_)
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) $(CXXFLAGS) -c \
            $(GTEST_DIR)/src/gtest-all.cc

gtest_main.o : $(GTEST_SRCS_)
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) $(CXXFLAGS) -c \
            $(GTEST_DIR)/src/gtest_main.cc

libgtest.a : gtest-all.o
	$(AR) $(ARFLAGS) $@ $^

libgtest_main.a : gtest-all.o gtest_main.o
	$(AR) $(ARFLAGS) $@ $^
