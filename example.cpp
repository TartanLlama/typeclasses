#include <iostream>
#include "traits.hpp"

typeclass printable {
    void print();
};

struct foo_printable {
    void print() { std::cout << "foo"; }
};

struct bar_printable {
    void print() { std::cout << "bar"; }
};

int main() {
    std::vector<typeclass_value<printable>> v;
    v.push_back(foo{});
    v.push_back(bar{});
    v[0].print();
    v[1].print();
}

