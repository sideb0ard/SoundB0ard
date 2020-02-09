CC = clang++

GTEST_DIR =  /Users/sideboard/Code/googletest/googletest

SRC = $(shell find src -type f -name '*.cpp')
OBJDIR = obj
OBJ = $(patsubst %.cpp, $(OBJDIR)/%.o, $(SRC))

LIBS=-lportaudio -lportmidi -lreadline -lm -lpthread -lsndfile -lprofiler -llo

ABLETONASIOINC=/Users/sideboard/Code/link/modules/asio-standalone/asio/include
READLINEINCDIR=/Users/sideboard/homebrew/opt/readline/include
INCDIRS=-I/usr/local/include -Iinclude/interpreter -Iinclude -Iinclude/afx -Iinclude/stack -I/Users/sideboard/Code/link/include -I/Users/sideboard/homebrew/include -I${HOME}/Code/range-v3/include/ -I${ABLETONASIOINC} -I${READLINEINCDIR}

HOMEBREWLIBDIR=/Users/sideboard/homebrew/lib
READLINELIBDIR=/Users/sideboard/homebrew/opt/readline/lib
LIBDIRS=-L/usr/local/lib -L${HOMEBREWLIBDIR} -L${READLINELIBDIR}

WARNFLAGS = -Wall -Wextra -pedantic -Wstrict-prototypes -Wmissing-prototypes -Wno-variadic-macros -Wno-c99-extensions -Wno-vla-extension -Wno-unused-parameter -Wno-four-char-constants
CPPFLAGS = -isystem $(GTEST_DIR)/include
# CXXFLAGS += -std=c++17 $(WARNFLAGS) -ggdb -O1 -fsanitize=address -fsanitize=thread -fno-omit-frame-pointer
//CXXFLAGS += -std=c++17 $(WARNFLAGS) -ggdb -O1 -fsanitize=thread -fno-omit-frame-pointer
//CXXFLAGS += -std=c++17 $(WARNFLAGS) -ggdb -O1 -fsanitize=thread -fno-omit-frame-pointer
CXXFLAGS += -std=c++17 $(WARNFLAGS) -ggdb -O0 -fno-omit-frame-pointer

$(OBJDIR)/%.o: %.cpp
	$(CC) -c -o $@ $< $(CXXFLAGS) $(INCDIRS)
	@echo

TARGET = sbsh

.PHONY: depend clean

all: objdir $(TARGET)
	@ctags -R *
	@cscope -R -b
	@echo "\n\x1b[37mBoom! make some noise...\x1b[0m"

objdir:
	find src -type d -exec mkdir -p obj/{} \;

$(TARGET): $(OBJ)
	$(CC) $(CPPFLAGS) $(INCDIRS) $(CXXFLAGS) $(LIBDIRS) $(LIBS) $^ -o $@

clean:
	@rm -f *.o *~ $(TARGET) $(TEST_TARGET)
	@rm -rf obj/*
	@rm -f libgtest.a libgtest_main.a
	@echo "\n\x1b[37mCleannnnd..!\x1b[0m"

format:
	find . -type f -name "*.[ch]" -exec clang-format -i {} \;


############### TESTzzz ################################################################
###########

TEST_TARGET = sbsh_test
TESTS = $(wildcard tests/*.cpp)
OBJ_MINUS_MAIN = $(filter-out obj/src/main.o, $(OBJ))

GTEST_LIB_DIR = .
GTEST_LIBS = libgtest.a libgtest_main.a

$(TEST_TARGET): $(TESTS) $(GTEST_LIBS) $(OBJ_MINUS_MAIN)
	$(CC) $(CPPFLAGS) $(INCDIRS) $(CXXFLAGS) $(LIBDIRS) $(LIBS) -L$(GTEST_LIB_DIR) -lgtest $^ -o $@

GTEST_HEADERS = $(GTEST_DIR)/include/gtest/*.h \
                $(GTEST_DIR)/include/gtest/internal/*.h

GTEST_SRCS_ = $(GTEST_DIR)/src/*.cc $(GTEST_DIR)/src/*.h $(GTEST_HEADERS)

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
