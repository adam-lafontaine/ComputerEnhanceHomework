#include "../lib/lib.hpp"

#include <cstdio>
#include <cstring>
#include <cstdlib>


constexpr auto OUT_DIR = "out";


static void usage(char* name)
{
    printf("\nUsage:\n");
    printf("  %s \n", name);
    printf("  %s --generate n_pairs\n", name);
    printf("  %s --compare\n", name);
}


static void generate(char* argv[])
{
    if (strcmp(argv[1], "--generate") != 0)
    {
        usage(argv[0]);
        return;
    }
    
    auto str = argv[2];
    auto end = str;

    auto n_pairs = std::strtoul(str, &end, 10);

    if (*end != 0)
    {
        usage(argv[0]);
        return;
    }

    haversine_json(OUT_DIR, (u32)n_pairs);
    print_directory(OUT_DIR);
}


void compare(char* argv[])
{
    if (strcmp(argv[1], "--compare") != 0)
    {
        usage(argv[0]);
        return;
    }

    auto result = process_json("out/pairs.json");
    auto ref = process_bin("out/answers64.bin");

    print_results(result, ref);
}


void run()
{
    auto result = process_json_profile("out/pairs.json");
}


int main(int argc, char* argv[])
{    
    switch(argc)
    {    
    case 1:
        run();
        break;

    case 2:
        compare(argv);
        break;

    case 3:
        generate(argv);
        break;

    default:
        usage(argv[0]);
    }
}