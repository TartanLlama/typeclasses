#include <memory>
#include <experimental/meta>

namespace meta = std::experimental::meta;

consteval bool should_generate_function_for(meta::info i) {
    return meta::is_member_function(i) and
           meta::is_normal(i) and 
           not meta::is_copy_assignment_operator(i) and
           not meta::is_move_assignment_operator(i);
}

template <class T>
class model {
public:
    // model will be inherited from by impl, so need virtual destructor
    virtual ~model() = default;

    // Generate pure virtual functions for every member function of the typeclass
    consteval{
    for (auto func : meta::range(reflexpr(T))) {
        if (should_generate_function_for(func)) {
            auto ret = meta::return_type_of(func);
            meta::range params(func);
            -> __fragment struct {
                virtual typename(ret) unqualid(func) (-> params) = 0; 
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
    consteval {
        for (auto func : meta::range(reflexpr(T))) {
            if (should_generate_function_for(func)) {
                auto ret = meta::return_type_of(func);
                meta::range params(func);
                -> __fragment struct { 
                    typename(ret) unqualid(func) (-> params) {
                        this->i.unqualid(func)(unqualid(...params));
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

consteval void gen_func(meta::info func) {
    auto ret = meta::return_type_of(func);
    meta::range params(func);
    -> __fragment struct { 
        typename(ret) unqualid(func) (-> params) {
            return this->m_model->unqualid(func)(unqualid(...params));
        }
    };
}

// Generate functions for every member function in the typeclass
// which forward calls on to the model
consteval void gen_funcs(meta::info interface) {
    for (auto func : meta::range(interface)) {
        if (should_generate_function_for(func)) {
            gen_func(func);
        }
    }
}


template <class T>
class typeclass_for {
private:
    std::unique_ptr<model<T>> m_model;

public:
    consteval{
        gen_funcs(reflexpr(T));
    }

    // Capture the type which is passed in and type-erase it
    template <class U> typeclass_for(const U& u) :
        m_model(new impl<T,U>{u}) {
    }
    // Capture the type which is passed in and type-erase it
    template <class U>
    typeclass_for& operator=(const U& u) {
        this->m_model.reset(new impl<T,U>{u});
    }
    // Virtual clone
    typeclass_for(const typeclass_for& rhs) { 
        this->m_model = rhs.m_model->clone(); 
    }
    typeclass_for(typeclass_for&&) = default;

    // Virtual clone
    typeclass_for& operator=(const typeclass_for& rhs) {
        this->m_model = rhs.m_model->clone();
        return *this;
    }

    typeclass_for& operator=(typeclass_for&&) = default;

    ~typeclass_for() = default;
};

consteval void typeclass(meta::info source) {
   gen_funcs(source);

   -> __fragment class X {
private:
    std::unique_ptr<model<X>> m_model;

public:
    // Capture the type which is passed in and type-erase it
    template <class U> X(const U& u) :
        m_model(new impl<X,U>{u}) {
    }
    // Capture the type which is passed in and type-erase it
    template <class U>
    X& operator=(const U& u) {
        this->m_model.reset(new impl<X,U>{u});
    }
    // Virtual clone
    X(const X& rhs) { 
        this->m_model = rhs.m_model->clone(); 
    }
    X(X&&) = default;

    // Virtual clone
    X& operator=(const X& rhs) {
        this->m_model = rhs.m_model->clone();
        return *this;
    }

    X& operator=(X&&) = default;

    ~X() = default;
   };
}
