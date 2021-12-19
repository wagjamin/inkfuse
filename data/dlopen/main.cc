#include <cstdlib>
#include <iostream>
#include <dlfcn.h>

using namespace std;

int main(int argc, char *argv[]) {
    void* handle = dlopen(argv[1], RTLD_NOW);
    if (!handle) {
        cerr << "error: " << dlerror() << endl;
        exit(1);
    }

    auto fn=reinterpret_cast<int (*)(int)>(dlsym(handle, "foo"));
    if (!fn) {
        cerr << "error: " << dlerror() << endl;
        exit(1);
    }

    cout << fn(atoi(argv[2])) << endl;

    if (dlclose(handle)) {
        cerr << "error: " << dlerror() << endl;
        exit(1);
    }

    return 0;
}
