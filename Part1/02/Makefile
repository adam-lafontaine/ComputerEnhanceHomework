GPP := g++-11

build := ./build_files

exe := $(build)/decode


# main
main_dep :=

main_c := main_039.cpp
#main_c := main_040.cpp
main_o := $(build)/main.o
object_files := $(main_o)


LIBRARIES :=

CCFLAGS := -std=c++17
#CCFLAGS += -O3 -DNDEBUG

# build rules

$(main_o): $(main_c) $(main_dep)
	@echo "\n main"
	$(GPP) $(CCFLAGS) -o $@ -c $< $(LIBRARIES)


$(exe): $(object_files)
	@echo "\n exe"
	$(GPP) $(CCFLAGS) -o $@ $+ $(LIBRARIES)


build: $(exe)

run: build
	$(exe)

clean:
	rm -rfv $(build)/*

setup:
	mkdir -p $(build)