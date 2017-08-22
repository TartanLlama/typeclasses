#include <memory>
#include <cppx/compiler>
#include <cppx/meta>

$class typeclass{};

template <class T>
class model {
public:
    constexpr {
        for... (auto func : reflexpr(T).functions) {
                -> class { virtual func.type() func$ (func$ args) = 0; }
            }
    }
};

template <class T, class I>
class impl : public model<T> {
public:
    impl(const I& i) : i{i} {}

    constexpr {
        for... (auto func : reflexpr(T).functions) {
                -> class {
                    func.type() (func$) (func$ args) {
                        return i.idexpr(func.name())(args...);
                    }
                }
            }
    }
private:
    I i;
};

template <class T>
class typeclass_value {
private:
    std::unique_ptr<model<T>> m_model;

public:
    constexpr {
        for... (auto func : reflexpr(T).functions) {
                -> class { public:
                    auto (func$) (func$ args) {
                        return m_model->idexpr(func.name())(args...);
                    }
                }
            }
    }

    template <class U>
    typeclass_object(const U& u) : m_model{ new impl<T,U>{u} }
    {}
};

