## CHANGE THESE FOR YOUR OWN USE:

ABLETONASIOINCDIR=${HOME}/Code/link/modules/asio-standalone/asio/include
READLINEINCDIR=${HOME}/homebrew/opt/readline/include
HOMEBREWINCDIR=${HOME}/homebrew/include
PERLININCDIR=${HOME}/Code/PerlinNoise/

GTEST_DIR =  ${HOME}/Code/googletest/googletest
INCDIRS=-I/usr/local/include \
				-Iinclude \
				-I${HOME}/Code/link/include \
				-I${HOMEBREWINCDIR} \
				-I${ABLETONASIOINCDIR} \
				-I${READLINEINCDIR} \
				-I${PERLININCDIR}


HOMEBREWLIBDIR=${HOME}/homebrew/lib
READLINELIBDIR=${HOME}/homebrew/opt/readline/lib

LIBDIRS=-L/usr/local/lib -L${HOMEBREWLIBDIR} -L${READLINELIBDIR}

## I *THINK* THATS THE END OF THINGS YOU NEED TO CHANGE!

CC = clang++


SRC = $(shell find src -type f -name '*.cpp')
OBJDIR = obj
OBJ = $(patsubst %.cpp, $(OBJDIR)/%.o, $(SRC))

LIBS=-lportaudio -lreadline -lm -lpthread -lsndfile -latomic #-lprofiler

WARNFLAGS = -Wall -Wextra -pedantic -Wstrict-prototypes -Wmissing-prototypes -Wno-variadic-macros -Wno-c99-extensions -Wno-vla-extension -Wno-unused-parameter -Wno-four-char-constants -Weffc++ -Wuninitialized
CPPFLAGS = -isystem $(GTEST_DIR)/include
#CXXFLAGS += -std=c++17 $(WARNFLAGS) -glldb -O3 -fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls
#CXXFLAGS += -std=c++17 $(WARNFLAGS) -ggdb -O1 -fsanitize=thread -fno-omit-frame-pointer
#CXXFLAGS += -std=c++17 $(WARNFLAGS) -ggdb -O1 -fsanitize=thread -fno-omit-frame-pointer
CXXFLAGS += -std=c++20 $(WARNFLAGS) -glldb -O3


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
