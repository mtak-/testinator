// Copyright (c) 2015 Tyler Kopf
// This code is distributed under the MIT license. See LICENSE for details.

#pragma once

#include "prettyprint.h"

namespace testinator {

  namespace detail {
    struct else_tag {};
    
    template<class... BoolArg>
    struct if_else;
    
    template<>
    struct if_else<>
    { using type = else_tag; };
    
    template<class T, class Arg, class... BoolArgs>
    struct if_else<T, Arg, BoolArgs...>
    {
      using type = std::conditional_t<
        T{},
        Arg,
        typename if_else<BoolArgs...>::type>;
    };
  }
  
  template<class... BoolArgs>
  using if_else_t = typename detail::if_else<BoolArgs...>::type;
  
  // tool to unqualify a type, apply a metafunction, and then qualify its result to match the input
  namespace detail {
    template<template<class, class...> class F, class T>
    struct qualified_fmap_impl : F<T> {};
    
    template<template<class, class...> class F, class T>
    struct qualified_fmap_impl<F, const T>
    { using type = const typename qualified_fmap_impl<F, T>::type; };
    
    template<template<class, class...> class F, class T>
    struct qualified_fmap_impl<F, volatile T>
    { using type = volatile typename qualified_fmap_impl<F, T>::type; };
    
    template<template<class, class...> class F, class T>
    struct qualified_fmap_impl<F, const volatile T>
    { using type = const volatile typename qualified_fmap_impl<F, T>::type; };
    
    template<template<class, class...> class F, class T>
    struct qualified_fmap_impl<F, T&>
    { using type = typename qualified_fmap_impl<F, T>::type&; };
    
    template<template<class, class...> class F, class T>
    struct qualified_fmap_impl<F, T&&>
    { using type = typename qualified_fmap_impl<F, T>::type&&; };
    
    template<template<class, class...> class F, class T>
    struct qualified_fmap_impl<F, T*>
    { using type = typename qualified_fmap_impl<F, T>::type*; };
  }
  
  template<template<class, class...> class F, class T>
  using qualified_fmap = typename detail::qualified_fmap_impl<F, T>::type;

  template<class Constructor>
  struct Generatable;
  
  // used to get the original type out of a generatable or identity if it isn't a generatable
  namespace detail {
    template<class T>
    struct get_original_type_impl
    { using type = T; };
    
    template<class T, class... Args>
    struct get_original_type_impl<Generatable<T(Args...)>>
    { using type = T; };
    
    template<template<class...> class C, class... Args>
    struct get_original_type_impl<C<Args...>>
    { using type = C<qualified_fmap<detail::get_original_type_impl, Args>...>; };
  }
  
  template<class T>
  using get_original_type = qualified_fmap<detail::get_original_type_impl, T>;
  
  // converts proxy types to actual types
  // ex. Generatable<Type(Args...)> -> Type
  //     std::tuple<Generatable<Type(Args)>> -> std::tuple<Type>
  template<class Proxy>
  decltype(auto) proxy_to_t(Proxy&& p);
  
  namespace detail {
    struct no_conversion_tag {};
    struct iterable_conversion_tag {};
    
    template<class From>
    using convert_switch =
      if_else_t<
        std::is_same<From, get_original_type<From>>,
          no_conversion_tag,
        detail::is_iterable<From>,
          iterable_conversion_tag>;
    
    template<class From,
             class = convert_switch<From>>
    struct convert_proxy;
    
    template<class To>
    struct convert_proxy<To, no_conversion_tag> {
      template<class In>
      static In&& get(In&& from) noexcept
      { return std::forward<In>(from); }
    };
    
    template<class From>
    struct convert_proxy<From, iterable_conversion_tag> {
      using result_type = get_original_type<From>;
      
      static result_type get(const From& from)
      {
        auto result = result_type{};
        auto insert_iter = std::inserter(result, std::end(result));
        std::transform(
          std::begin(from),
          std::end(from),
          insert_iter,
          [](auto&& item)
          { return proxy_to_t(std::forward<decltype(item)>(item)); });
        
        return result;
      }
      
      static result_type get(From&& from)
      {
        auto result = result_type{};
        auto insert_iter = std::inserter(result, std::end(result));
        
        std::transform(
          std::make_move_iterator(std::begin(from)),
          std::make_move_iterator(std::end(from)),
          insert_iter,
          [](auto&& item)
          { return proxy_to_t(std::forward<decltype(item)>(item)); });
        
        return result;
      }
    };
    
    // specialization for tuple
    template<class T>
    using keep_lvalue_reference = std::conditional_t<std::is_lvalue_reference<T>{},
                                                    T&,
                                                    std::remove_reference_t<T>>;
    
    template<class... Args1>
    struct convert_proxy<std::tuple<Args1...>, else_tag> {
      using result_type = get_original_type<std::tuple<Args1...>>;
      
      template<std::size_t I, class In>
      static decltype(auto) tuple_convert_i(In&& from)
      { return proxy_to_t(std::get<I>(std::forward<In>(from))); }
      
      template <class... Ts>
      static auto forward_as_tuple(Ts&&... ts)
      { return std::tuple<keep_lvalue_reference<Ts>...>(std::forward<Ts>(ts)...); }
    
      template<class In, std::size_t... Is>
      static decltype(auto) tuple_convert(In&& from, std::integer_sequence<std::size_t, Is...>)
      { return forward_as_tuple(tuple_convert_i<Is>(std::forward<In>(from))...); }
      
      template<class In>
      static decltype(auto) get(In&& from)
      { return tuple_convert(std::forward<In>(from), std::make_index_sequence<sizeof...(Args1)>{}); }
    };
    
    template<class... Args1>
    struct convert_proxy<std::pair<Args1...>, else_tag>
      : convert_proxy<std::tuple<Args1...>, else_tag> {};
    
    template<class From, class... Args>
    struct convert_proxy<Generatable<From(Args...)>, else_tag> {
      template<class In>
      static decltype(auto) get(In&& from)
      { return proxy_to_t(std::forward<In>(from).get()); }
    };
    
    // handle implicit conversion to iterable type
    template<class From, class... Args>
    struct convert_proxy<Generatable<From(Args...)>, iterable_conversion_tag>
      : convert_proxy<Generatable<From(Args...)>, else_tag> {};
    
  }
  
  template<class Proxy>
  decltype(auto) proxy_to_t(Proxy&& p)
  { return detail::convert_proxy<std::decay_t<Proxy>>::get(std::forward<Proxy>(p)); }
  
  template<class T>
  using proxy_to_t_type = decltype(proxy_to_t(std::declval<T>()));
  
}