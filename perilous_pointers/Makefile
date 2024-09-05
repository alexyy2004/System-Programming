OBJS_DIR = .objs

# define all of student executables
EXE1=part1
EXE2=part2
EXES_STUDENT=$(EXE1) $(EXE2) $(EXE3)

# tests are defined a little differently for this assignment, see below

# list object file dependencies for each
OBJS_PART1=part1-functions.o part1-main.o
OBJS_PART2=part2-functions.o part2-main.o

# set up compiler
CC = clang
WARNINGS = -Wall -Wextra -Werror -Wno-error=unused-parameter -Wmissing-declarations -Wmissing-variable-declarations
INCLUDES=-I./includes/
CFLAGS_COMMON = $(INCLUDES) $(WARNINGS) -std=c99 -c -MMD -MP -D_GNU_SOURCE
CFLAGS_RELEASE = $(CFLAGS_COMMON) -O2
CFLAGS_DEBUG = $(CFLAGS_COMMON) -O0 -g -DDEBUG

# set up linker
LD = clang
LDFLAGS = -lm

# the string in grep must appear in the hostname, otherwise the Makefile will
# not allow the assignment to compile
IS_VM=$(shell hostname | grep "cs341")
VM_OVERRIDE=$(shell echo $$HOSTNAME)
ifeq ($(IS_VM),)
ifneq ($(VM_OVERRIDE),cs241grader)
$(error This assignment must be compiled on the CS341 VMs)
endif
endif


.PHONY: all
all: release

# build types
.PHONY: release
.PHONY: debug

release: integrity $(EXES_STUDENT)
debug: integrity clean $(EXES_STUDENT:%=%-debug)

# this one is a little different than others
# for the love of god make sure this target isn't first in the list
.PHONY: test
test:
	./test.sh

# Verifiy the checksum of ungraded files listed in checksums
# Warn student if any has been modified
# Generate checksums file by: md5sum <list of files to verify> > checksums
.PHONY: integrity
FILES_TO_VERIFY = $(shell cat checksums | grep -o " .*")
MODIFIED_MESSAGE = "\
\n\
=============================================================================================\n\
YOU HAVE MODIFIED THE HEADER FILES, the diff output above shows which ones.\n\
This may cause unexpected errors, as the autograder uses the ORIGINAL header files!\n\
You can find the original headers on Github.\n\
=============================================================================================\n\
"
integrity:
	@md5sum $(FILES_TO_VERIFY) | diff checksums - || echo $(MODIFIED_MESSAGE)

# include dependencies
-include $(OBJS_DIR)/*.d

$(OBJS_DIR):
	@mkdir -p $(OBJS_DIR)

# patterns to create objects
# keep the debug and release postfix for object files so that we can always
# separate them correctly
$(OBJS_DIR)/%-debug.o: %.c | $(OBJS_DIR)
	$(CC) $(CFLAGS_DEBUG) $< -o $@

$(OBJS_DIR)/%-release.o: %.c | $(OBJS_DIR)
	$(CC) $(CFLAGS_RELEASE) $< -o $@

# exes
# you will need a pair of exe and exe-debug targets for each exe
$(EXE1)-debug: $(OBJS_PART1:%.o=$(OBJS_DIR)/%-debug.o)
	$(LD) $^ $(LDFLAGS) -o $@

$(EXE1): $(OBJS_PART1:%.o=$(OBJS_DIR)/%-release.o)
	$(LD) $^ $(LDFLAGS) -o $@

$(EXE2)-debug: $(OBJS_PART2:%.o=$(OBJS_DIR)/%-debug.o)
	$(LD) $^ $(LDFLAGS) -o $@

$(EXE2): $(OBJS_PART2:%.o=$(OBJS_DIR)/%-release.o)
	$(LD) $^ $(LDFLAGS) -o $@

.PHONY: clean
clean:
	rm -rf .objs $(EXES_STUDENT) $(EXES_STUDENT:%=%-debug)