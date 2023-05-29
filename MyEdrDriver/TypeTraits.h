#pragma once

template <bool _Test, class _Ty = void>
struct enable_if {}; // no member "type" when !_Test

template <class _Ty>
struct enable_if<true, _Ty> { // type is _Ty for _Test
    using type = _Ty;
};

template <bool _Test, class _Ty = void>
using enable_if_t = typename enable_if<_Test, _Ty>::type;

template <class _Ty, _Ty _Val>
struct integral_constant {
    static constexpr _Ty value = _Val;

    using value_type = _Ty;
    using type = integral_constant;

    constexpr operator value_type() const noexcept {
        return value;
    }

    [[nodiscard]] constexpr value_type operator()() const noexcept {
        return value;
    }
};

template <bool _Val>
using bool_constant = integral_constant<bool, _Val>;

template <class, class>
constexpr bool is_same_v = false; // determine whether arguments are the same type
template <class _Ty>
constexpr bool is_same_v<_Ty, _Ty> = true;

template <class _Ty>
struct remove_cv { // remove top-level const and volatile qualifiers
    using type = _Ty;

    template <template <class> class _Fn>
    using _Apply = _Fn<_Ty>; // apply cv-qualifiers from the class template argument to _Fn<_Ty>
};

template <class _Ty>
struct remove_cv<const _Ty> {
    using type = _Ty;

    template <template <class> class _Fn>
    using _Apply = const _Fn<_Ty>;
};

template <class _Ty>
struct remove_cv<volatile _Ty> {
    using type = _Ty;

    template <template <class> class _Fn>
    using _Apply = volatile _Fn<_Ty>;
};

template <class _Ty>
struct remove_cv<const volatile _Ty> {
    using type = _Ty;

    template <template <class> class _Fn>
    using _Apply = const volatile _Fn<_Ty>;
};

template <class _Ty>
using remove_cv_t = typename remove_cv<_Ty>::type;

template <class _Ty>
constexpr bool is_void_v = is_same_v<remove_cv_t<_Ty>, void>;

template <class _Ty>
struct is_void : bool_constant<is_void_v<_Ty>> {};

template <class _Ty>
constexpr bool is_function_v = // only function types and reference types can't be const qualified
!is_const_v<const _Ty> && !is_reference_v<_Ty>;

template <class>
constexpr bool is_const_v = false; // determine whether type argument is const qualified

template <class _Ty>
constexpr bool is_const_v<const _Ty> = true;

template <class>
constexpr bool is_reference_v = false; // determine whether type argument is a reference

template <class _Ty>
struct is_function : bool_constant<is_function_v<_Ty>> {};
