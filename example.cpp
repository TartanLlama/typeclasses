#include <iostream>
#include "traits.hpp"

typeclass printable {
    void print();
};

struct foo {
    void print() { std::cout << "foo"; }
};

struct bar {
    void print() { std::cout << "bar"; }
};

int main() {
    std::vector<printable> v;
    v.push_back(foo{});
    v.push_back(bar{});
    v[0].print();
    v[1].print();
}

