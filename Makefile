CXXFLAGS=`pkg-config --cflags sdl2` -std=c++17
LIBS=`pkg-config --libs sdl2`

sowon: main.cpp
	$(CXX) $(CXXFLAGS) -o sowon main.cpp $(LIBS)
