CXXFLAGS =	-O2 -g -Wall -fmessage-length=0

OBJS =		src/chp.o src/common.o src/instruction.o src/keyword.o src/message.o src/program.o src/preprocessor.o src/syntax/assignment.o src/syntax/channel.o src/syntax/composition.o src/syntax/constant.o src/syntax/control.o src/syntax/dot.o src/syntax/expression.o src/syntax/debug.o src/syntax/declaration.o src/syntax/variable_name.o src/syntax/type_name.o src/syntax/operator.o src/syntax/process.o src/syntax/record.o src/syntax/skip.o src/syntax/slice.o src/tokenizer.o src/syntax/instance.o src/variable_space.o src/variable.o

LIBS =

TARGET =	chp2hse

$(TARGET):	$(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS)

all:	$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
