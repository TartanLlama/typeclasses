#include <memory>
#include <cppx/compiler>
#include <cppx/meta>

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

$class typeclass {
private:
    std::unique_ptr<model<typeclass>> m_model;

public:
    constexpr {
        for... (auto func : $typeclass.functions) {
                -> class { public:
                    auto (func$) (func$ args) {
                        return m_model->idexpr(func.name())(args...);
                    }
                }
            }
    }

    template <class U>
    typeclass(const U& u) : m_model{ new impl<typeclass,U>{u} }
    {}

    template <class U>
    typeclass& operator=(const U& u) {
        m_model.reset(new impl<typeclass,U>{u});
    }

};

