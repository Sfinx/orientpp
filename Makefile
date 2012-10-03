
# Copyright (C) 2012, Rus V. Brushkoff, All rights reserved.  

BOOST_LIB_PATH		:= /usr/lib64
BOOST_INCLUDE_PATH	:= /usr/include
JSON_LIB_PATH		:= /usr/lib64
JSON_INCLUDE_PATH	:= /usr/local/include/json_spirit
# -Wsequence-point
# -Wstrict-aliasing
# -O2
CFLAGS = -g3 -Wall -Wno-strict-aliasing -Wno-sequence-point -Winvalid-pch -std=gnu++0x -D_REENTRANT -D_GNU_SOURCE -D_THREAD_SAFE -I$(BOOST_INCLUDE_PATH) -I$(JSON_INCLUDE_PATH) -I.
LDFLAGS = -L$(BOOST_LIB_PATH) -L$(JSON_LIB_PATH)
CXX	:= g++
EXE=orientpp
VERSION_FILE = version.cpp
BUILD_NUMBER := $(strip $(subst ;,,$(subst int OrientPP::ORIENTPP_BUILD_NUMBER =,,$(shell /usr/bin/grep "int OrientPP::ORIENTPP_BUILD_NUMBER = " $(VERSION_FILE)))))

LDFLAGS := $(LDFLAGS) -lboost_system -lboost_date_time -lboost_program_options -lboost_thread -lpthread -ljson_spirit
OBJS = test.o version.o log.o orient.o

$(EXE): $(OBJS)
	$(CXX) $(CFLAGS) $(OBJS) -o $@ -Wl,--start-group $(LDFLAGS) -Wl,--end-group

%.o: %.cpp
	$(CXX) -MD -c $(CFLAGS) $< -o $@

%.gch: %.h
	$(CXX) -MD -c $(CFLAGS) $< -o $@

version:
	@./scripts/version.sh

incr:
	@BLD=`expr $(BUILD_NUMBER) + 1`; \
	echo "OrientPP build number" $$BLD; \
	sed -i "s/ORIENTPP_BUILD_NUMBER[ = ]*[0-9][0-9]*/ORIENTPP_BUILD_NUMBER = $$BLD/" $(VERSION_FILE)

# cleanup by removing generated files
#
.PHONY:		clean
clean:
		rm -f *.o *.gch $(EXE) *.d DEADJOE out
dcp:
	@git diff
	@git commit -a
	@git push ssh://git@voip/home/git/orientpp
	@./scripts/version.sh

test:
	@$(EXE) 2 > tests/log.txt 2>&1; cat tests/log.txt
	
-include $(wildcard *.d)
