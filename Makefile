ZAJAL_DIR = src
BUILD_DIR = build
BIN_DIR = bin
BINARY = $(BIN_DIR)/zajal
LIBRARY = $(BIN_DIR)/libzajal.a


### 
CC = gcc
CXX = g++
override CXXFLAGS += -arch i386 -g -DHAVE_STRUCT_TIMEZONE -DHAVE_STRUCT_TIMESPEC
GIT=git


### openFrameworks
OF_DIR = lib/openframeworks
OF_LIB = $(OF_DIR)/libof.a
OF_INCLUDE_DIR = $(OF_DIR)/include
OF_FRAMEWORKS = $(addprefix -framework ,OpenGL Glut QuickTime CoreAudio Carbon)
OF_INCLUDES = $(addprefix -idirafter ,$(shell find $(OF_DIR)/include -type d))


### ruby
RUBY_DIR = lib/ruby
RUBY_LIB = $(RUBY_DIR)/libruby.a
RUBY_INCLUDE_DIR = $(RUBY_DIR)/include
RUBY_INCLUDES = $(addprefix -I ,$(shell find $(RUBY_DIR)/include -type d))


### zajal interperter
ZAJAL_GIT_BRANCH = $(shell $(GIT) name-rev --name-only HEAD)
ZAJAL_GIT_HASH = $(shell $(GIT) log -1 --pretty=format:%H)
ZAJAL_GIT_SHORT_HASH = $(shell $(GIT) log -1 --pretty=format:%h)

ZAJAL_INCLUDES = -I$(ZAJAL_DIR)
ZAJAL_SRC = $(shell find -E $(ZAJAL_DIR) -regex ".*\.cc?$$")
ZAJAL_OBJ = $(addprefix $(BUILD_DIR)/, $(addsuffix .o, $(notdir $(ZAJAL_SRC))))
ZAJAL_LIBRARIES = $(OF_LIB) $(RUBY_LIB)


.PHONY: all static clean configure docs $(BUILD_DIR)/version.c.o

all: $(BINARY)
	@echo "Built Zajal $(ZAJAL_GIT_SHORT_HASH) binary to $(BINARY)"

static: $(LIBRARY) $(ZAJAL_LIBRARIES)
	@echo "Built Zajal $(ZAJAL_GIT_SHORT_HASH) static library to $(LIBRARY)"

docs:
	yardoc -e util/docs/screenshots src

$(BINARY): $(ZAJAL_OBJ)
	@echo -n "Making binary..."
	@mkdir -p $(BIN_DIR)
	@$(CXX) $(CXXFLAGS) $(OF_INCLUDES) $(RUBY_INCLUDES) $(ZAJAL_INCLUDES) $(OF_FRAMEWORKS) $(ZAJAL_LIBRARIES) $(ZAJAL_OBJ) -o $(BINARY)
	@echo "OK"

$(LIBRARY): $(ZAJAL_OBJ)
	@echo -n "Making static library..."
	@mkdir -p $(BIN_DIR)
	@libtool -static -o $(LIBRARY) $(ZAJAL_OBJ) $(ZAJAL_LIBRARIES)
	@echo "OK"

configure: $(ZAJAL_DIR)/config.h

$(ZAJAL_DIR)/config.h:
	@echo -n "Generating configuration..."
	@rm -f $@
	@echo "/* Generated by the Zajal Makefile */" >> $@
	@echo "/* You probably shouldn't edit by hand */" >> $@
	@echo "" >> $@
	@echo "#ifndef CONFIG_H" >> $@
	@echo "#define CONFIG_H" >> $@
	@echo "" >> $@
	@echo "#define ZAJAL_RUBY_STDLIB_PATH  \"`pwd`/lib/ruby/stdlib\"" >> $@
	@echo "#define ZAJAL_LIBRARY_PATH      \"`pwd`/lib/zajal\"" >> $@
	@echo "" >> $@
	@echo "#endif /* CONFIG_H */" >> $@
	@echo "OK"

$(BUILD_DIR)/ZajalInterpreter.cpp.o: $(ZAJAL_DIR)/config.h

$(BUILD_DIR)/%.cpp.o: $(ZAJAL_DIR)/%.cpp
	@echo -n "Building $<..."
	@mkdir -p $(BUILD_DIR)
	@$(CXX) $(CXXFLAGS) $(OF_INCLUDES) $(RUBY_INCLUDES) $(ZAJAL_INCLUDES) -c -o $@ $<
	@echo "OK"

$(BUILD_DIR)/%.cc.o: $(ZAJAL_DIR)/%.cc
	@echo -n "Building $<..."
	@mkdir -p $(BUILD_DIR)
	@$(CXX) $(CXXFLAGS) $(OF_INCLUDES) $(RUBY_INCLUDES) $(ZAJAL_INCLUDES) -c -o $@ $<
	@echo "OK"

$(BUILD_DIR)/version.cpp.o: $(ZAJAL_DIR)/version.cpp
	@echo -n "Building in version information..."
	@mkdir -p $(BIN_DIR)
	@cp $< $<.bak
	@sed -i '' -Ee 's/zajal_hash\[\] = "[^"]*"/zajal_hash[] = "$(ZAJAL_GIT_HASH)"/' $<
	@sed -i '' -Ee 's/zajal_short_hash\[\] = "[^"]*"/zajal_short_hash[] = "$(ZAJAL_GIT_SHORT_HASH)"/' $<
	@sed -i '' -Ee 's/zajal_branch\[\] = "[^"]*"/zajal_branch[] = "$(ZAJAL_GIT_BRANCH)"/' $<
	@$(CXX) $(CXXFLAGS) $(OF_INCLUDES) $(RUBY_INCLUDES) $(ZAJAL_INCLUDES) -c -o $@ $<
	@mv $<.bak $<
	@echo "OK"

clean:
	@echo -n "Cleaning..."
	@rm $(BUILD_DIR)/*
	@rm $(ZAJAL_DIR)/config.h
	@echo "OK"