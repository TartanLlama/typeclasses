# Typeclasses in C++

This is an idea of how Rust-like trait objects could be implemented in C++ using static reflection, code injection and metaclasses. This solution is very succinct, requiring no boilerplate or arcane tricks. It has been tested against the Clang fork at https://gitlab.com/lock3/clang.git 02eaac5aa06dfa0d19de95270bccd6311f11f5ba.

The idea is to use class definitions which have only declarations of members as typeclasses. For example, a typeclass which is satisfied by classes with a `print` function which returns nothing and takes no arguments can be defined as follows:

```cpp
class(typeclass) printable {
    void print();
};
```

The `typeclass` metaclass will generate a class which can be constructed from values of any type satisfying the interface. The given object will be type-erased and forwarding functions will be generated to call the relevant functions on the implementation.

Say that we have a couple of classes which fulfil this interface:

```cpp
struct foo {
    void print() { std::cout << "foo"; }
};

struct bar {
    void print() { std::cout << "bar"; }
};
```

We can type-erase these objects by using `printable`:

```cpp
printable a = foo{};
printable b = bar{};
a.print();
b.print();
```

We could even store them in a container:

```cpp
std::vector<printable> v;
v.push_back(foo{});
v.push_back(bar{});
v[0].print();
v[1].print();
```

Or reassign from different types:


```cpp
printable a = foo{};
a.print();
a = bar{};
a.print();
```

As support in the compiler progresses, I'll think about adding opt-in features, or C++11-concepts-style concept maps.

See Sean Parent's talks on [Inheritance is the Base Class of Evil](https://www.youtube.com/watch?v=bIhUE5uUFOA) and [Value Semantics and Concepts-based Polymorphism](https://www.youtube.com/watch?v=_BpMYeUFXv8) for some of the motivation behind this. See [Dyno](https://github.com/ldionne/dyno) and [Boost.TypeErasure](http://www.boost.org/doc/libs/1_55_0/doc/html/boost_typeerasure.html) for other implementations.
