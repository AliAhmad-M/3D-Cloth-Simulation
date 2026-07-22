#include "engine.h"

int main(int argc, char** argv) {
    engine_t engine;

    if (!engine_init(&engine, 800, 600, "Cloth Engine")) {
        return -1;
    }

    engine_run(&engine);
    engine_cleanup(&engine);

    return 0;
}