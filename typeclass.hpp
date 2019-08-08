/// Type erasure with metaclasses
/// Copyright Sy Brand

/*
This file contains an implementation of type-erasing objects automatically generated from a type declaration.

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

producer a = static_producer{};
producer b = dynamic_producer{12};
a.produce() + b.produce() <- 54
*/

#include <iostream>
#include <vector>
#include <array>
#include <memory>
#include <utility>
#include <type_traits>
#include <experimental/meta>

namespace meta = std::experimental::meta;

namespace tl {
//Only normal member functions should have wrappers generated for them
consteval bool should_generate_function_for(meta::info i) {
    return meta::is_normal(i) and 
           not meta::is_copy_assignment_operator(i) and
           not meta::is_move_assignment_operator(i);
}

//Loop over all the functions declared in the given typeclass and call f
//with each of them along with their return type and parameters
template <class F>
consteval void for_each_declared_function(meta::info typeclass, F&& f) {
    for (auto func : meta::member_fn_range(typeclass)) {
        if (should_generate_function_for(func)) {
            f(func, meta::return_type_of(func), meta::param_range(func));
        }
    }    
}

consteval std::size_t count_number_of_functions_to_generate(meta::info typeclass) {
    std::size_t count = 0;
    for_each_declared_function(typeclass, [&count](auto,auto,auto) { count++; });
    return count;
}

//This is the abstract model which will be filled in with information about the erased type.
//It's a collection of function pointers to the typeclass functions and special member functions.
//We're essentially generating our own vtable and storing it in-place. It could also be stored remotely;
//this implementation makes it easier to change later.
template <class Typeclass>
class model {
protected:
    //Generate a dispatch table declaration which will hold pointers to functions
    //to call based on the declarations in the typeclass
    consteval {
        auto n_functions = count_number_of_functions_to_generate(reflexpr(Typeclass));
        -> __fragment struct {public:
            std::array<void(*)(), n_functions> dispatch_table_; 
        };
    }
      
    //These function pointers will point at functions to destroy, clone, and move
    //objects of the erased type.
    //They'll be filled in by `impl` below
    void(*destructor_)(model&);
    std::unique_ptr<model>(*clone_)(model&);
    std::unique_ptr<model>(*move_clone_)(model&);

public:
    //Helper functions to access the function pointers
    ~model() { destructor_(*this); }
    std::unique_ptr<model> clone() { return clone_(*this); }
    std::unique_ptr<model> move_clone() { return move_clone_(*this); }

    model() = default;
    model(const model&) = delete;
    model& operator=(const model&) = delete;

    template<class Ret, class... Args>
    Ret call(std::size_t idx, Args... args) {
        using desired_type = Ret(*)(model&, Args... args);

        auto void_function = this->dispatch_table_[idx];
        auto correctly_typed_function = reinterpret_cast<desired_type>(void_function);

        return correctly_typed_function(*this, std::forward<Args>(args)...);
    }
};
template<class>struct TC;
//This is the implementation of a typeclasses model for a given concrete type.
//When constructed, it will fill in all the function pointers declared above
// and store an instance of the concrete type.
template <class Typeclass, class ConcreteType>
class impl : public model<Typeclass> {
public:
    //Fill in all of the function pointers.
    //Could potentially do this in the member initializer list,
    //but I can't work out if it's possible to inject a list of expressions there.
    template <class U>
    impl(U&& u) : ct(std::forward<U>(u)) {
        this->destructor_ = [](base& m){
            static_cast<impl&>(m).ct.~ConcreteType();
        };
        this->clone_ = [](base& m) -> std::unique_ptr<model<Typeclass>> {
            return std::make_unique<impl>(static_cast<impl&>(m).ct);
        };
        this->move_clone_ = [](base& m) -> std::unique_ptr<model<Typeclass>> {
            return std::make_unique<impl>(std::move(static_cast<impl&>(m).ct));
        };

        //Compiler bug workaround.
        auto& dispatch_table = this->dispatch_table_;

        //Fill in the dispatch table with pointers to static member functions which we'll generate below.
        consteval {
            std::size_t idx = 0;
            for (auto func : meta::member_fn_range(reflexpr(Typeclass))) {
                if (should_generate_function_for(func)) {
                    auto ret = meta::return_type_of(func);
                    meta::param_range params(func);
                    -> __fragment { 
                        auto l = +[](base& mod, ->params) constexpr {
                    return static_cast<impl&>(mod).ct.produce(unqualid(...params));
                };
                        dispatch_table[idx] = reinterpret_cast<void(*)()>(l);
                    };
                    idx++;
                }
            }  

        }
    }

private:
    using base = model<Typeclass>;

    //This is where the erased object finally ends up stored
    ConcreteType ct;
};

//A storage helper to wrap storing the erased object and making sure that calls to 
//special member functions are forwarded to the correct versions.
template <class T>
class storage {
public:
    //Type erase the given object
    template <class U> 
    storage(U&& u) :
        model_(std::make_unique<impl<T,std::decay_t<U>>>(std::forward<U>(u))) {
    }

    //Forward a call on to the model
    template <class Ret, class... Ts>
    Ret call(std::size_t idx, Ts... ts) {
        return model_->template call<Ret>(idx, std::forward<Ts>(ts)...);
    }

    //Virtual copies and moves
    storage(const storage& rhs) : model_(rhs.model_->clone()) {}
    storage(storage&& rhs) : model_(rhs.model_->move_clone()) {}
    storage& operator=(const storage& rhs) { model_ = rhs.model_->clone(); return *this; }
    storage& operator=(storage&& rhs) { model_ = rhs.model_->move_clone(); return *this; }

private:
    std::unique_ptr<model<T>> model_;

};

//For every function we want to support, generate a function which forwards the call on to the storage.
consteval void generate_call_forwarders(meta::info typeclass) {
    //for_each_declared_function doesn't work here for some reason: compiler bug?
    std::size_t idx = 0;
    for (auto func : meta::member_fn_range(typeclass)) {
        if (should_generate_function_for(func)) {
            meta::param_range params(func);
            auto ret = meta::return_type_of(func);
            -> __fragment class X { public:
                typename(ret) unqualid(func) (-> params) {
                    return this->storage_.template call<typename(ret)>(idx, unqualid(...params));
                }
            };
            ++idx;
        }
    }
}

//Metaclass which generates type-erasing objects for a given interface.
//E.g.
// class(tl::typeclass) printer { void print(); }
//Objects of type printer can store objects of any type which have a `print` member function,
//And you can call `print` on them.
consteval void typeclass(meta::info source) {
    -> __fragment class X {
public:
    storage<X> storage_;
    X() = delete;
    template <class U> X(U u) : storage_(std::move(u)) {}    
    };

    generate_call_forwarders(source);
}

//Generates a typeclass object given a declaration in case you can't modify your class.
//E.g. instead of writing 
// class(tl::typeclass) printer { void print(); };
// printer p = some_printer_type{};
//You'd write
// class printer { void print(); };
// typeclass_for<printer> = some_printer_type{};
template <class Typeclass>
class typeclass_for {
public:
    storage<Typeclass> storage_;
    typeclass_for() = delete;
    template <class U> typeclass_for(U u) : storage_(std::move(u)) {}

    consteval {
        generate_call_forwarders(reflexpr(Typeclass));
    }
};
}