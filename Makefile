CC = clang++


GTEST_DIR =  /Users/sideboard/Code/googletest/googletest
GTEST_LIB_DIR = /Users/sideboard/Code/googletest/googletest/make

# Google Test libraries
GTEST_LIBS = libgtest.a libgtest_main.a

# All Google Test headers.  Usually you shouldn't change this
# definition.
GTEST_HEADERS = $(GTEST_DIR)/include/gtest/*.h \
                $(GTEST_DIR)/include/gtest/internal/*.h




# SRC = $(wildcard *.cpp) $(wildcard */*.cpp)
SRC = $(shell find src/ -type f -name '*.cpp')
OBJDIR = obj
# MAIN = src/main.cpp
OBJ = $(patsubst %.cpp, $(OBJDIR)/%.o, $(SRC))

LIBS=-lportaudio -lportmidi -lreadline -lm -lpthread -lsndfile -lprofiler -llo

ABLETONASIOINC=-I/Users/sideboard/Code/link/modules/asio-standalone/asio/include
INCDIRS=-I/Users/sideboard/homebrew/opt/readline/include -I/usr/local/include -Iinclude/interpreter -Iinclude -Iinclude/afx -Iinclude/stack -I/Users/sideboard/Code/link/include -I/Users/sideboard/homebrew/include -I${HOME}/Code/range-v3/include/
LIBDIR=/usr/local/lib
HOMEBREWLIBDIR=/Users/sideboard/homebrew/lib
#READLINELIBDIR=/Users/sideboard/homebrew/Cellar/readline/7.0.3_1/lib
#READLINEINCDIR=/Users/sideboard/homebrew/opt/readline/include
READLINELIBDIR=/Users/sideboard/homebrew/opt/readline/lib
WARNFLASGS = -Wall -Wextra -pedantic -Wstrict-prototypes -Wmissing-prototypes
# Flags passed to the preprocessor.
CPPFLAGS = -std=c++17 $(WARNFLAGS) -g $(INCDIRS) $(ABLETONASIOINC) -O0 -fsanitize=address -fno-omit-frame-pointer -isystem $(GTEST_DIR)/include
#CPPFLAGS = -std=c++17 $(WARNFLAGS) -g $(INCDIRS) $(ABLETONASIOINC) -O3 -fsanitize=address -fno-omit-frame-pointer -isystem $(GTEST_DIR)/include

$(OBJDIR)/%.o: %.cpp
	$(CC) -c -o $@ $< $(CPPFLAGS)

TARGET = sbsh
TEST_TARGET = sbsh_test

LEXER_TESTS =  tests/lexer_test.cpp
PARSER_TESTS = tests/parser_test.cpp
EVAL_TESTS =   tests/evaluator_test.cpp
OBJECT_TESTS = tests/object_test.cpp

.PHONE: depend clean

all: objdir $(TARGET)
	@ctags -R *
	@cscope -R -b
	@echo "\n\x1b[37mBoom! make some noise...\x1b[0m"

objdir:
	mkdir -p obj/src/fx obj/src/filterz obj/src/pattern_transformers obj/src/cmdloop obj/src/pattern_generators obj/src/value_generators obj/src/afx obj/src/interpreter obj/src/tests

$(TARGET): $(OBJ)
	$(CC) $(CPPFLAGS) -L$(READLINELIBDIR) -L$(LIBDIR) -L$(HOMEBREWLIBDIR) -o $@ $^ $(LIBS) $(INCDIRS)

$(TEST_TARGET): $(PARSER_TESTS) $(LEXER_TESTS) $(EVAL_TESTS) $(OBJECT_TESTS) $(GTEST_LIBS) $(OBJ)
	$(info $$OBJ is [${OBJ}])
	$(CC) $(CPPFLAGS) -L$(READLINELIBDIR) -L$(LIBDIR) -L$(HOMEBREWLIBDIR) -L$(GTEST_LIB_DIR) -lgtest -lpthread -o $@ $^ $(LIBS) $(INCDIRS)

clean:
	rm -f *.o *~ $(TARGET)
	rm -rf obj/*
	@echo "\n\x1b[37mCleannnnd..!\x1b[0m"

format:
	find . -type f -name "*.[ch]" -exec clang-format -i {} \;

# Builds gtest.a and gtest_main.a.

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
