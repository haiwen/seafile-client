TARGET = seafile_shell_ext.dll
DLL_PATH = $(shell pwd -W | sed -e 's|/|\\\\\\\\|g')\\\\$(TARGET)

MODULES = dll.cpp shell-ext.cpp class-factory.cpp context-menu.cpp log.cpp \
	applet-connection.cpp commands.cpp ext-utils.cpp

OBJECTS = ${MODULES:%.cpp=%.o}

ifeq (${MAKECMDGOALS}, x64)
# 64 bit, for server 2003 or above
	TARGET = seafile_shell_ext64.dll
	WINVER = 0x0502
	CXX = x86_64-w64-mingw32-g++
	DLLWRAP = x86_64-w64-mingw32-dllwrap
	WINDRES = x86_64-w64-mingw32-windres
else
# 32 bit, for xp or above
	WINVER = 0x0501
	CXX = g++
	DLLWRAP = dllwrap
	WINDRES = windres
endif


all: $(TARGET)

CXXFLAGS = -O -g -Wall -std=c++0x \
	-I../common -D_WIN32_WINNT=${WINVER} -DWINVER=${WINVER}

DLLWRAPFLAGS = -Wl,--enable-stdcall-fixup

debug: all
x64: all

.deps: $(MODULES)
	$(CXX) $(CXXFLAGS) -MM $(MODULES) > $@

-include .deps

%.o : %.cpp
	$(CXX) $(CXXFLAGS) $< -c -o $@

$(TARGET): $(OBJECTS) seafile_shell_ext.def .deps
	$(DLLWRAP) $(DLLWRAPFLAGS) --def seafile_shell_ext.def \
	$(OBJECTS) -o $@ \
	-luuid -loleaut32 -lole32 -lws2_32 -lpsapi -lshlwapi -luserenv  -static -lstdc++

clean:
	rm -rf *.o *.dll .deps

.PHONY: all clean
