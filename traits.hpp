#include <memory>
#include <cppx/compiler>
#include <cppx/meta>

template <class T>
class model {
public:
    // model will be inherited from by impl, so need virtual destructor
    virtual ~model() = default;

    // Generate pure virtual functions for every member function of the typeclass
    constexpr {
        for... (auto func : reflexpr(T).functions) {
                -> class { virtual func.type() func$ (func$ args) = 0; }
            }
    }
    // Virtual clone pattern for value semantics
    virtual std::unique_ptr<model> clone() = 0;
};

template <class T, class I>
class impl : public model<T> {
public:
    impl(const I& i) : i{i} {}

    // Generate overrides of the model functions which forward calls on
    // to the type erased object
    constexpr {
        for... (auto func : reflexpr(T).functions) {
                -> class {
                    func.type() (func$) (func$ args) {
                        return i.idexpr(func.name())(args...);
                    }
                }
            }
    }

    // Perform a full copy of i
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
    // Generate functions for every member function in the typeclass
    // which forward calls on to the model
    constexpr {
        for... (auto func : $typeclass.functions) {
                -> class { public:
                    auto (func$) (func$ args) {
                        return m_model->idexpr(func.name())(args...);
                    }
                }
            }
    }

    // Capture the type which is passed in and type-erase it
    template <class U>
    typeclass(const U& u) : m_model{ new impl<typeclass,U>{u} }
    {}

    // Needed for correct value semantics
    typeclass(const typeclass& rhs) : m_model{ rhs.m_model->clone(); };
    typeclass(typeclass&&) = default;    

    // Capture the type which is passed in and type-erase it
    template <class U>
    typeclass& operator=(const U& u) {
        m_model.reset(new impl<typeclass,U>{u});
    }

    // Needed for correct value semantics
    typeclass& operator=(const typeclass& rhs) {
        m_model.reset(rhs.m_model->clone());
    }
    
    typeclass& operator=(typeclass&&) = default;

    ~typeclass() = default;
};

