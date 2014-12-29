BUILDDIR=build
TARGET = $(BUILDDIR)/seafile_shell_ext.dll
DLL_PATH = $(shell pwd -W | sed -e 's|/|\\\\\\\\|g')\\\\$(TARGET)

MODULES = ext.cpp shell-ext.cpp class-factory.cpp context-menu.cpp log.cpp

ifeq (${MAKECMDGOALS}, x64)
# 64 bit, for server 2003 or above. We need to set _WIN32_WINNT for KEY_WOW64_64KEY.
	OBJECTS = ${MODULES:%.cpp=$(BUILDDIR)/%.x64.o}
	TARGET = $(BUILDDIR)/seafile_shell_ext64.dll
	WINVER = 0x0502
	CXX = x86_64-w64-mingw32-g++
	DLLWRAP = x86_64-w64-mingw32-dllwrap
	WINDRES = x86_64-w64-mingw32-windres
else
# 32 bit, for xp or above
	OBJECTS = ${MODULES:%.cpp=$(BUILDDIR)/%.o}
	WINVER = 0x0501
	CXX = g++
	DLLWRAP = dllwrap
	WINDRES = windres
endif


all: $(TARGET)

CXXFLAGS = -O -g -Wall \
	-I../common -D_WIN32_WINNT=${WINVER} -DWINVER=${WINVER}

DLLWRAPFLAGS = -Wl,--enable-stdcall-fixup

debug: all
x64: all

.deps: $(MODULES)
	$(CXX) $(CXXFLAGS) -MM $(MODULES) > $@

-include .deps

$(BUILDDIR)/%.o : %.cpp
	@mkdir -p $(BUILDDIR)
	$(CXX) $(CXXFLAGS) $< -c -o $@

$(BUILDDIR)/%.x64.o : %.cpp
	$(CXX) $(CXXFLAGS) $< -c -o $@

$(TARGET): $(OBJECTS) seafile_shell_ext.def .deps
	$(DLLWRAP) $(DLLWRAPFLAGS) --def seafile_shell_ext.def \
	$(OBJECTS) -o $@ \
	-luuid -loleaut32 -lole32 -lws2_32 -lpsapi -lshlwapi -luserenv -L. -lstdc++

clean:
	rm -f $(OBJECTS) $(BUILDDIR)/*.dll .deps

.PHONY: all clean
