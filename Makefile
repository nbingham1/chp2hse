CXXFLAGS	 =  -O2 -g -Wall -fmessage-length=0
SOURCES	    :=  $(shell find src -name '*.cpp')
OBJECTS	    :=  $(SOURCES:src/%.cpp=build/%.o)
DIRECTORIES :=  $(sort $(dir $(OBJECTS)))
LDFLAGS		 =  
TARGET		 =  chp2hse

all: build $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(OBJECTS) -o $(TARGET)

build/%.o: src/%.cpp 
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -c -o $@ $<
	
build:
	mkdir $(DIRECTORIES)

clean:
	rm -f $(OBJECTS) $(TARGET) $(TARGET).exe
