#include <memory>
#include <cppx/compiler>
#include <cppx/meta>

template <class T>
class model {
public:
    // model will be inherited from by impl, so need virtual destructor
    virtual ~model() = default;

    // Generate pure virtual functions for every member function of the typeclass
    constexpr{
    for... (auto func : reflexpr(T).functions()) {
        auto ret = func.return_type();
        if (func.is_normal() && !func.is_copy_assign() && !func.is_move_assign()) {
        __generate __fragment struct {
            virtual typename(ret) idexpr(func) (__inject(func.parameters()) args) = 0; 
        };
        }
    }
    }
    
    // Virtual clone pattern for value semantics
    virtual std::unique_ptr<model> clone() = 0;
};

template <class T, class I>
class impl : public model<T> {
private:
    I i;

public:
    impl(const I& i) : i{i} {}

    // Generate overrides of the model functions which forward calls on
    // to the type erased object
    constexpr {
        for... (auto func : reflexpr(T).functions()) {
                auto ret = func.return_type();
                if (func.is_normal() && !func.is_copy_assign() && !func.is_move_assign()) {
                __generate __fragment struct { 
                    typename(ret) idexpr(func) (__inject(func.parameters()) args) {
                        return this->i.idexpr(func)(args...);
                    }
                };
                }
            }
    }

    // Perform a full copy of i
    std::unique_ptr<model<T>> clone() {
        return std::make_unique<impl>(i);
    }
    
};


template <class T>
constexpr void typeclass (T source) {
    __generate __fragment class X { private:
        std::unique_ptr<model<X>> m_model;
    };


    // Generate functions for every member function in the typeclass
    // which forward calls on to the model
    for... (auto func : source.functions()) {
        auto ret = func.return_type();
        __generate __fragment struct { 
            typename(ret) idexpr(func) (__inject(func.parameters()) args) {
                return this->m_model->idexpr(func)();
            }
        };
    }

    // THIS ICES
/*
    __generate __fragment struct X {
    // Capture the type which is passed in and type-erase it
    template <class U> X(const U& u) {
        this->m_model = new impl<X,U>{u};
    }

// Capture the type which is passed in and type-erase it
    template <class U>
    X& operator=(const U& u) {
        m_model.reset(new impl<X,U>{u});
    }
    };
    */


    __generate __fragment class X { public:
    // Virtual clone
    X(const X& rhs) { 
        this->m_model = rhs.m_model->clone(); 
    }
    X(X&&) = default;

    // Virtual clone
    X& operator=(const X& rhs) {
        m_model = rhs.m_model->clone();
    }

    X& operator=(X&&) = default;

    ~X() = default;
    };
}
