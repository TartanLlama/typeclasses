#include <memory>
#include <cppx/compiler>
#include <cppx/meta>

template <class T>
class model {
public:
    virtual ~model() = default;
    constexpr {
        for... (auto func : reflexpr(T).functions) {
                -> class { virtual func.type() func$ (func$ args) = 0; }
            }
    }
    virtual std::unique_ptr<model> clone() = 0;
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

    std::unique_ptr<model> clone() {
        return std::make_unique<impl>(i);
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

    typeclass(const typeclass& rhs) : m_model{ rhs.m_model->clone(); };
    typeclass(typeclass&&) = default;    

    template <class U>
    typeclass& operator=(const U& u) {
        m_model.reset(new impl<typeclass,U>{u});
    }

    typeclass& operator=(const typeclass& rhs) {
        m_model.reset(rhs.m_model->clone());
    }
    
    typeclass& operator=(typeclass&&) = default;

    ~typeclass() = default;
};

