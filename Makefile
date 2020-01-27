
TEST_OBJECTS := TestProgram.o
TEST_PROGRAM := BinaryManipulatorTestSuite
OBJS := $(TEST_OBJECTS)
PROGS := $(TEST_PROGRAM)
CXXFLAGS += -std=c++2a


all: $(PROGS)

$(TEST_PROGRAM): $(TEST_OBJECTS)
	@echo LD ${TEST_PROGRAM}
	@${CXX} ${LDFLAGS} -o ${TEST_PROGRAM} ${TEST_OBJECTS}

.cc.o :
	@echo CXX $<
	@${CXX} ${CXXFLAGS} -c $< -o $@

clean: 
	@echo Cleaning...
	@rm -f ${OBJS} ${PROGS}



# generated via g++ -MM -std=c++17 *.cc *.h

TestProgram.o: TestProgram.cc BinaryManipulation.h

