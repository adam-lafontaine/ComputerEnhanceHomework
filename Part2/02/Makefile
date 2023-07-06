GPP := g++-11

build := ./build_files
lib := ../lib

exe := $(build)/haversine


# main
main_dep :=

main_c := main.cpp
main_o := $(build)/main.o
object_files := $(main_o)

lib_dep := $(lib)/lib.hpp
lib_dep += $(lib)/types.hpp
lib_dep += $(lib)/listing_0065_haversine_formula.cpp
lib_dep += $(lib)/json_write.cpp
lib_dep += $(lib)/json_read.cpp
lib_dep += $(lib)/bin_read.cpp

lib_c := $(lib)/lib.cpp
lib_o := $(build)/lib.o
object_files += $(lib_o)


LIBRARIES :=

CCFLAGS := -std=c++17
#CCFLAGS += -O3 -DNDEBUG

# build rules

$(main_o): $(main_c) $(main_dep)
	@echo "\n main"
	$(GPP) $(CCFLAGS) -o $@ -c $< $(LIBRARIES)

$(lib_o): $(lib_c) $(lib_dep)
	@echo "\n lib"
	$(GPP) $(CCFLAGS) -o $@ -c $< $(LIBRARIES)


$(exe): $(object_files)
	@echo "\n exe"
	$(GPP) $(CCFLAGS) -o $@ $+ $(LIBRARIES)


build: $(exe)

run: build
	$(exe)

gen: build
	$(exe) --generate 1000000

comp: build
	$(exe) --compare

clean:
	rm -rfv $(build)/*

setup:
	mkdir -p $(build)