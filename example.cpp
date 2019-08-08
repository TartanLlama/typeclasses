#include <iostream>
#include "typeclass.hpp"

class(tl::typeclass) the_typeclass {
    void f();
    void g();
};

struct an_instance {
    void f(){}
    void g(){}
};

class(tl::typeclass) producer {
    int produce();
};

struct static_producer {
    int produce() {
        return 42;
    }
};

struct dynamic_producer {
    int i;
    int produce() { 
        return i;
    }
};

int main() {
    producer a = static_producer{};
    producer b = dynamic_producer{12};
    return a.produce() + b.produce();
}

