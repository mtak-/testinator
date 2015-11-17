// Copyright (c) 2015 Tyler Kopf
// This code is distributed under the MIT license. See LICENSE for details.

#pragma once

// see TESTINATOR_REGISTER_CONSTRUCTOR's comments for proper usage

#include "arbitrary.h"
#include "arbitrary_proxy_to_t.h"
#include "prettyprint.h"

#define CONCEPT_REQUIRES_(...)                                        \
        bool TESTINATOR_CONCEPT##__LINE__ = false,                    \
        typename std::enable_if<                                      \
            (TESTINATOR_CONCEPT##__LINE__ == true) || (__VA_ARGS__),  \
            bool                                                      \
        >::type = false

#define CONCEPT_REQUIRES(...)                                         \
    template<CONCEPT_REQUIRES_(__VA_ARGS__)>

#define SFINAE_DETECT(name, expr)                                     \
  template <typename T>                                               \
  using name##_t = decltype(expr);                                    \
  template <typename T, typename = void>                              \
  struct has_##name : public std::false_type {};                      \
  template <typename T>                                               \
  struct has_##name<T, detail::void_t<name##_t<T>>>                   \
    : public std::true_type {};

#define OPERATOR_DETECT(name, op)                                     \
  SFINAE_DETECT(name, (std::declval<const T&>())                      \
                      op                                              \
                      (std::declval<const T&>()))                     \

#define OPERATOR_FORWARD(name, op)                                    \
  CONCEPT_REQUIRES(concepts::has_##name<T>{})                         \
  decltype(auto) operator op (const T& rhs) const                     \
  { return static_cast<const_ref_type>(*this) op rhs; }

namespace testinator {

  namespace concepts {
    OPERATOR_DETECT(less,          <);
    OPERATOR_DETECT(less_equal,    <=);
    OPERATOR_DETECT(equal,         ==);
    OPERATOR_DETECT(greater_equal, >=);
    OPERATOR_DETECT(greater,       >);
    SFINAE_DETECT(hash, std::hash<T>{}(std::declval<T>()));
  }
  
  namespace detail {
    template<bool... Bs>
    struct all : std::true_type {};
    
    template<bool... Bs>
    struct all<true, Bs...> : all<Bs...> {};
    
    template<bool... Bs>
    struct all<false, Bs...> : std::false_type {};
  }
  
  namespace detail {
    struct no_copy_construct
    {
      constexpr no_copy_construct() noexcept = default;
      constexpr no_copy_construct(const no_copy_construct&) = delete;
      constexpr no_copy_construct(no_copy_construct&&) noexcept = default;
      no_copy_construct& operator=(const no_copy_construct&) noexcept = default;
      no_copy_construct& operator=(no_copy_construct&&) noexcept = default;
    protected:
      ~no_copy_construct() noexcept = default;
    };
    struct no_copy_assign
    {
      constexpr no_copy_assign() noexcept = default;
      constexpr no_copy_assign(const no_copy_assign&) noexcept = default;
      constexpr no_copy_assign(no_copy_assign&&) noexcept = default;
      no_copy_assign& operator=(const no_copy_assign&) noexcept = delete;
      no_copy_assign& operator=(no_copy_assign&&) noexcept = default;
    protected:
      ~no_copy_assign() noexcept = default;
    };
    struct no_move_construct
    {
      constexpr no_move_construct() noexcept = default;
      constexpr no_move_construct(const no_move_construct&) noexcept = default;
      constexpr no_move_construct(no_move_construct&&) noexcept = delete;
      no_move_construct& operator=(const no_move_construct&) noexcept = default;
      no_move_construct& operator=(no_move_construct&&) noexcept = default;
    protected:
      ~no_move_construct() noexcept = default;
    };
    struct no_move_assign
    {
      constexpr no_move_assign() noexcept = default;
      constexpr no_move_assign(const no_move_assign&) noexcept = default;
      constexpr no_move_assign(no_move_assign&&) noexcept = default;
      no_move_assign& operator=(const no_move_assign&) noexcept = default;
      no_move_assign& operator=(no_move_assign&&) noexcept = delete;
    protected:
      ~no_move_assign() noexcept = default;
    };
    
    template<bool B, class Conditional, class Always>
    struct inherit_if : Always {};
    
    template<class Conditional, class Always>
    struct inherit_if<true, Conditional, Always>
      : Conditional, Always {};
    
    template<class... BoolBases>
    struct inherit_all_true {};
    
    template<class Bool, class Base, class... BoolBases>
    struct inherit_all_true<Bool, Base, BoolBases...>
      : private inherit_if<Bool{}, Base, inherit_all_true<BoolBases...>> {};
    
    template<class T>
    using constructible_switch =
      inherit_all_true<
        std::integral_constant<bool, !std::is_copy_constructible<T>{}>,
          no_copy_construct,
        std::integral_constant<bool, !std::is_copy_assignable<T>{}>,
          no_copy_assign,
        std::integral_constant<bool, !std::is_move_constructible<T>{}>,
          no_copy_construct,
        std::integral_constant<bool, !std::is_move_assignable<T>{}>,
          no_copy_assign>;
    
    struct no_move_no_copy
      : private no_copy_construct
      , private no_move_construct
      , private no_copy_assign
      , private no_move_assign
    {
      no_move_no_copy() = default;
    protected:
      ~no_move_no_copy() = default;
    };
    
    template<class T, class = void>
    struct uptr : std::unique_ptr<T>
    {
      // to get noexcept
      uptr(std::unique_ptr<T>&& p) noexcept
        : std::unique_ptr<T>(std::move(p)) {}
      uptr(uptr&& rhs) noexcept = default;
      uptr& operator=(uptr&& rhs) noexcept = default;
      
      uptr(const uptr& rhs)
        : std::unique_ptr<T>(std::make_unique<T>(*rhs))
      {}
      uptr& operator=(const uptr& rhs)
      { return this->std::unique_ptr<T>::operator=(std::make_unique<T>(*rhs)); }
    };
    
    template<class T>
    struct uptr<T, std::enable_if_t<!std::is_copy_constructible<T>{} &&
                                    !std::is_copy_assignable<T>{}>>
      : std::unique_ptr<T>
    { using std::unique_ptr<T>::unique_ptr; };
  }
  
  template<class Constructor>
  struct ArbitraryConstructible_impl;

  template<class Constructor>
  struct Generatable;

  // Generatable holds a value of type T, and the value of the arguments used to construct it
  template<class T, class... Args>
  struct Generatable<T(Args...)>
    : detail::constructible_switch<T>
  {
    static_assert(!std::is_const<T>{} &&
                  !std::is_reference<T>{} &&
                  std::is_class<T>{},
                  "Invalid Registered Constructor. Types must be classes/structs");
    static_assert(std::is_constructible<T, proxy_to_t_type<const Args&>...>{},
                  "Invalid Registered Constructor");
    static_assert(detail::all<(std::is_const<std::remove_reference_t<Args>>{} ||
                               !std::is_reference<Args>{})...>{},
                  "Invalid Registered Constructor. "
                  "Constructor cannot have non-const rvalue/lvalue parameters");
  
    using tuple_type = std::tuple<std::decay_t<Args>...>;
    using this_type = Generatable<T(Args...)>;
    using ref_type = T&;
    using rvalue_ref_type = T&&;
    using const_ref_type = const T&;
    using const_rvalue_ref_type = const T&&;

  private:
    // attempt to be flexible with move only/non-copyable types/non-default/etc types
    // by delaying construction
    struct value_storage_t
    {
      alignas(T) char value[sizeof(T)];
    
      template<std::size_t... Is>
      void construct_impl(const tuple_type& tuple, std::integer_sequence<std::size_t, Is...>)
      { new (&value[0]) T(proxy_to_t(std::get<Is>(tuple))...); }
      
      void construct(const tuple_type& tuple)
      { construct_impl(tuple, std::make_index_sequence<sizeof...(Args)>{}); }
      
      value_storage_t(const tuple_type& tuple)
      { construct(tuple); }
      
      T& get() &
      { return *static_cast<T*>(static_cast<void*>(value)); }
      
      T&& get() &&
      { return std::move(*static_cast<T*>(static_cast<void*>(value))); }
      
      const T& get() const &
      { return *static_cast<const T*>(static_cast<const void*>(value)); }
      
      const T&& get() const &&
      { return std::move(*static_cast<const T*>(static_cast<const void*>(value))); }
      
      value_storage_t(const value_storage_t& rhs)
      { new (&value[0]) T(rhs.get()); }
      
      value_storage_t(value_storage_t&& rhs)
      { new (&value[0]) T(std::move(rhs.get())); }
      
      value_storage_t& operator=(const value_storage_t& rhs)
      { get() = rhs.get(); }
      
      value_storage_t& operator=(value_storage_t&& rhs)
      { get() = std::move(rhs.get()); }
      
      ~value_storage_t()
      { get().~T(); }
    };
    
    struct data_t {
      value_storage_t storage;
      tuple_type args;
      
      CONCEPT_REQUIRES(std::is_copy_constructible<tuple_type>{})
      data_t(tuple_type&& args2)
        : storage(args2)
        , args(std::move(args2))
      {}
    };
    
    detail::uptr<data_t> data;
    
    tuple_type&& get_args() &&
    { return data->args; }
    
    const tuple_type& get_args() const &
    { return data->args; }
    
    ref_type get() &
    { return data->storage.get(); }
    
    const_ref_type get() const &
    { return data->storage.get(); }
    
    rvalue_ref_type get() &&
    { return std::move(data->storage.get()); }
    
    const_rvalue_ref_type get() const &&
    { return std::move(data->storage.get()); }
    
  public:
    Generatable(tuple_type&& tuple)
      : data(std::make_unique<data_t>(std::move(tuple)))
    {}

    Generatable(const Generatable&) = default;
    Generatable(Generatable&&) noexcept = default;

    Generatable& operator=(const Generatable&) = default;
    Generatable& operator=(Generatable&&) noexcept = default;
    
    operator T() const &
    { return data->storage.get(); }
    
    operator T() &&
    { return std::move(data->storage.get()); }
    
    operator ref_type() &
    { return data->storage.get(); }
    
    operator rvalue_ref_type() &&
    { return std::move(data->storage.get()); }
    
    operator const_ref_type() const
    { return data->storage.get(); }
    
    operator const_rvalue_ref_type() const &&
    { return std::move(data->storage.get()); }
    
    // useful for rebinding maps/sets
    OPERATOR_FORWARD(less, <)
    OPERATOR_FORWARD(less_equal, <=)
    OPERATOR_FORWARD(equal, ==)
    OPERATOR_FORWARD(greater_equal, >=)
    OPERATOR_FORWARD(greater, >=)
    
  private:
    template<class>
    friend struct ArbitraryConstructible_impl;
    
    template<class, class>
    friend struct detail::convert_proxy;
    
    template<class>
    friend struct ::std::hash;
    
    // prints out the type name, and the arguments used to construct it
    template<class Constructor>
    friend inline std::ostream& operator<<(std::ostream& os, const Generatable<Constructor>& rhs)
    {
      return os << Arbitrary<Generatable<Constructor>>::get_name()
                << prettyprint(rhs.get_args());
    }
  };
  
  // manages Generation/shrinking of generatable types
  template<class T, class... Args>
  struct ArbitraryConstructible_impl<Generatable<T(Args...)>>
  {
    using generatable_type = Generatable<T(Args...)>;
    using tuple_type = typename generatable_type::tuple_type;
  
    static generatable_type generate(std::size_t generation, unsigned long int randomSeed)
    { return generatable_type(Arbitrary<tuple_type>::generate(generation, randomSeed)); }

    static generatable_type generate_n(std::size_t generation, unsigned long int randomSeed)
    { return generatable_type(Arbitrary<tuple_type>::generate_n(generation, randomSeed)); }
    
    static std::vector<generatable_type> shrink(const generatable_type& t)
    {
      std::vector<generatable_type> result{};
      auto shrunk_tuple = Arbitrary<tuple_type>::shrink(t.get_args());
      result.reserve(shrunk_tuple.size());

      for(auto&& elem : shrunk_tuple)
        result.emplace_back(std::move(elem));

      return result;
    }
  };
}

// specialization for std::hash if the type T is hashable
namespace std {
  template<class T, class... Args>
  struct hash<testinator::Generatable<T(Args...)>>
  {
    CONCEPT_REQUIRES(testinator::concepts::has_hash<T>{})
    size_t operator()(const testinator::Generatable<T(Args...)>& n) const
    { return std::hash<T>{}(n.get()); }
  };
}

namespace testinator {
  // registered_ArbitraryConstructible is specialized for every type
  // that has registered a constructor through the testinator macro
  template<class T>
  struct registered_ArbitraryConstructible;
  
  template<class T, class = void>
  struct is_registered_ArbitraryConstructible : std::false_type {};
  
  template<class T>
  struct is_registered_ArbitraryConstructible<
        T,
        detail::void_t<typename registered_ArbitraryConstructible<T>::generatable_type>>
    : std::true_type {};
  
  // recursively converts types to Generatable versions of that type
  namespace detail {
    struct arbitrary_constructible_tag {};
    
    template<class T>
    using to_generatable_switch =
      if_else_t<
        is_registered_ArbitraryConstructible<T>,
          arbitrary_constructible_tag>;
    
    template<class T, class = to_generatable_switch<T>>
    struct to_generatable_impl
    { using type = T; };
    
    template<class T>
    struct to_generatable_impl<T, arbitrary_constructible_tag>
    { using type = typename registered_ArbitraryConstructible<T>::generatable_type; };
    
    template<template<class...> class C, class... Args>
    struct to_generatable_impl<C<Args...>, else_tag>
    { using type = C<qualified_fmap<detail::to_generatable_impl, Args>...>; };
  }
  
  template<class T>
  using to_generatable = qualified_fmap<detail::to_generatable_impl, T>;
  
  // manufactures a new Generatable type
  namespace detail {
    template<class Constructor>
    struct create_gen_type_impl;
    
    template<class T, class... Args>
    struct create_gen_type_impl<T(Args...)>
    { using type = Generatable<T(to_generatable<Args>...)>; };
  }
  
  template<class Constructor>
  using create_gen_type = typename detail::create_gen_type_impl<Constructor>::type;
  
  // Gives a uniform interface to building Arbitrary<>'s
  template<class T>
  using ArbitraryBuilder = Arbitrary<to_generatable<T>>;
}

#undef SFINAE_DETECT
#undef OPERATOR_DETECT
#undef OPERATOR_FORWARD
#undef CONCEPT_REQUIRES

// Macro to register a constructor for a type that can be constructed from other types that
// testinator knows about (either registered, or have explicit Arbitrary specializations)

// must be used in global namespace
// type = the type to be constructed
// __VA_ARGS__ = the types of the arguments to the constructor
#define TESTINATOR_REGISTER_CONSTRUCTOR(type, ...)                              \
  namespace testinator {                                                        \
    template<>                                                                  \
    struct Arbitrary<create_gen_type<type ( __VA_ARGS__ )>>                     \
      : ArbitraryConstructible_impl<create_gen_type<type ( __VA_ARGS__ )>>      \
    {                                                                           \
      static constexpr const char* get_name() noexcept                          \
      { return #type ; }                                                        \
    };                                                                          \
                                                                                \
    template<>                                                                  \
    struct registered_ArbitraryConstructible<type>                              \
      : Arbitrary<create_gen_type<type ( __VA_ARGS__ )>>                        \
    {};                                                                         \
  }
