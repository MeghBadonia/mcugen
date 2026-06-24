#include "cli/args.h"

int main(int argc, char **argv) {
    Args a;
    parse_args(argc, argv, &a);
    dispatch(&a);
    return 0;
}
