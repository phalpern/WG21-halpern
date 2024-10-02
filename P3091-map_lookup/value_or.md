---
title: "Replacement wording for `value_or` in D1255"
document: value_or
date: <!-- $TimeStamp$ -->2024-10-02 13:53 EDT<!-- $ -->
audience: LEWGI
author:
  - name: Pablo Halpern
    email: <phalpern@halpernwightsoftware.com>
working-draft: N4971
---

Hi Steve,

In preparation for updating my paper on `map::get`, [@P3091R2], I looked at
[@D1255R14] to see the correct usage of `std::value_or`, and I discovered a
number of problems. Note that I reviewed only the *Maybe types* section, but if
I have time, I can look at the rest.

First and foremost, we had discussed making two enhancements to `value_or`,
first as part of `optional`, then, instead, as part of *Maybe types*: 1) Let
the user optionally specify the return type, so that you could construct an,
e.g., `string_view` from a character literal without creating a temporary
`std::string` that would then become a hanging reference:

```cpp
std::string *p = nullptr;
std::string_view sv1 = std::value_or(p, "hello");  // BUG: dangling reference
std::string_view sv2 = std::value_or<std::string_view>(p, "hello");  // OK
```

The second usage of `std::value_or` is not allowed in the current wording of
D1255.

The other desirable feature was to allow `value_or` to have a variadic
parameter list, so that you can, again, construct the return value efficiently
without creating unnecessary temporaries:

```cpp
std::optional<std::vector<int>> opt;       // disengaged
std::vector v = std::value_or(opt, 3, 2);  // Constructs vector{2, 2, 2}
```

The same consideration applies to `initializer_list` arguments.

The other problem I saw was wording inaccuracies, typos, and usability issues
with not only `value_or`, but with `reference_or` and `or_invoke`,
too:

* All of them have an optional parameter, `R`, that is pretty much useless.
  Because it is the last parameter, the only way to specify a type other than
  the default is to specify all of the other template arguments first.

* In all cases, `m` was used instead of `*std::forward<T>(m)`, resulting in an
  `optional` rvalue being treated as an lvalue. Similarly for `invocable`.

* In a number of places `T` was used instead of the type returned by
  dereferencing a value of type `T`.

* In a number of cases `iter_reference_t<T>` was used, which (weirdly) discards
  the value category of `T`. This means that if `T` is `optional<int>&&`,
  `iter_reference_t<T>` is `int&` instead of `int&&`, throwing many things
  off.  I replaced the use of `iter_reference_t<T>` with
  `decltype(*std::forward<T>(m))`.

I've taken the liberty of re-writing the entire "Maybe Types" section of
[@D1255R14] to add the `value_or` functionality and fix the other issues
mentioned above.

I had the following additional concerns, but didn't make the changes because I
felt that they would need more discussion.

1. For `reference_or` and `or_invoke`, it's not clear to me that the purpose of
   the `R` parameter was to allow the caller to override it with a specific
   type. If that is not the purpose, then `R` can be removed from the parameter
   list, thus simplifying the specification; the computed return type can
   just be put after the `->`, without needing another template parameter.

2. I'm not a concepts expert, but I wonder if restricting the deference
   operator of `maybe` types to `const T` is unnecessary. I can imagine, for
   example, an input iterator for which `operator*` is mutating.  Would the
   following not be better?

```cpp
    template <class T>
    concept maybe : requires(const T a, T b) {
      bool(a);
      *b;
    };
```

3. Shouldn't `or_invoke` be named `or_else`, for consistency with the monadic
   function in `std::optional`?

4. It seems to me that if `or_invoke` cannot find a common return type, it
   would still be useful if it returned `void`.

5. Shouldn't there be an `and_then` (not sure what the return type should be)?

The diffs below are relative to the latest copy of [@D1255R14] to which I have
access.  This interface is implemented [here](https://github.com/phalpern/WG21-halpern/blob/P3091/P3091-map_lookup/xoptional.h#L176):

> **??.1 Maybe Types [maybe]**

> **??.??.1 In general [maybe.general]**

> Subclause ??.1 describes the concept `maybe` that represents types that model
> optionality and functions that operate on such types.


> **??.??.2 Header `<maybe>` synopsis [maybe.syn]**

>| `namespace std {`
>|
>| `// ??, ` *[class template]{.rm} [concept]{.add} `maybe`*
>| `template <class T>`
>| `concept maybe = requires(const T t) {`
>|     `bool(t);`
>|     `*(t);`
>| `};`

::: rm
>| `template <maybe T, class U, class R>`
>|     `constexpr auto value_or(T&& m, U&& u) -> R;`
>|
>| `template <maybe T, class U, class R>`
>|     `constexpr auto reference_or(T&& m, U&& u) -> R;`
>|
>| `template <maybe T, class I, class R>`
>|     `constexpr auto or_invoke(T&& m, I&& invocable) -> R;`
:::

::: add
>| `template <class R = void, maybe T, class... U>`
>|     `constexpr auto value_or(T&& m, U&&... u) -> ` *see below*`;`
>|
>| `template <class R = void, maybe T, class U>`
>|     `constexpr auto reference_or(T&& m, U&& u) -> ` *see below*`;`
>|
>| `template <class R = void, maybe T, class I>`
>|     `constexpr auto or_invoke(T&& m, I&& invocable) -> ` *see below*`;`
:::

>| `}`

::: rm
>| `template <maybe T, class U, class R = common_type_t<iter_reference_t<T>, U&&>>>`
>|     `constexpr auto value_or(T&& m, U&& u) -> R;`

>> *Mandates*: `T` models `maybe` `common_type_t<iter_reference_t<T>, U&&>::type` is defined

>> *Returns*: `bool(m) ? static_cast<R>(*m) : static_cast<R>(forward<U>(u)).`

>| `template <maybe T, class U, class R = common_reference_t<iter_reference_t<T>, U&&>>`
>| `constexpr auto smd::reference_or(T&& m, U&& u) -> R;`

>> *Mandates*: `T` models `maybe` `common_reference<iter_reference_t<T>, U&&>::type` is defined

>> *Constraints*: `!reference_constructs_from_temporary_v<R, U>`
>> `!reference_constructs_from_temporary_v<R, T&>`

>> *Returns*: `bool(m) ? static_cast<R>(*m) : static_cast<R>((U&&)u)`

>| `template <maybe T, class I,`
>|                       `class R = common_type_t<iter_reference_t<T>, invoke_result_t<I>>>`
>| `constexpr auto or_invoke(T&& m, I&& invocable) -> R`

>> *Mandates*: `T` models `maybe` `common_type<iter_reference_t<T>`,
>> `invoke_result_t<I>>::type` is defined

>> *Returns*: `bool(m) ? static_cast<R>(*m) : static_cast<R>(invocable())`
:::

::: add
> **Functions on `maybe` types [maybe.func]**

>| `template <class R = void, maybe T, class... U>`
>|     `constexpr auto value_or(T&& m, U&&... u) -> ` *see below*`;`

>> If `R` is not `void`, the return type, `RT` is `R`. Otherwise, if
>> `sizeof...(U)` is 1, `RT` is
>> `common_type_t<remove_cvref_t<decltype(*std::forward<T>(m))>>`. Otherwise,
>> `RT` is `remove_cvref_t<decltype(*std::forward<T>(m))>`.

>> *Mandates*: `RT`, is a valid type. `is_constructible_v<RT,`
>> `decltype(*std::forward<T>(m))> && is_constructible_v<RT, U...>` is
>> `true`. If `RT` is a reference type, then `sizeof(U...)` is 1 and
>> `reference_constructs_from_temporary_v<RT, decltype(*std::forward<T>(m))>`
>> `|| reference_constructs_from_temporary_v<RT, U...>` is `false`.

>> *Returns*: `return bool(m) ? static_cast<RT>(*forward<T>(m)) : RT(forward<U>(u)...);`

>| `template <class R = void, maybe T, class IT, class... U>`
>|     `constexpr auto value_or(T&& m, initializer_list<IT> il, U&&... u) -> ` *see below*`;`

>> If `R` is not `void`, the return type, `RT` is `R`, otherwise `RT` is
>> `remove_cvref_t<decltype(*std::forward<T>(m))>`.

>> *Mandates*: `is_constructible_v<RT, decltype(*std::forward<T>(m))> &&`
>> `is_constructible_v<RT, initializer_list<IT> il, U...>` is `true`.

>> *Returns*: `return bool(m) ? static_cast<RT>(*forward<T>(m)) : RT(il, forward<U>(u)...);`

>| `template <class R = void, maybe T, class U>`
>|     `constexpr auto reference_or(T&& m, U&& u) -> ` *see below*`;`

>> If `R` is not `void`, the return type, `RT` is `R`, otherwise `RT` is
>> `common_reference_t<decltype(*std::forward<T>(m)), U&&>`.

>> *Mandates*: `RT` is a valid type.
>> `reference_constructs_from_temporary_v<RT, U> ||`
>> `reference_constructs_from_temporary_v<RT, decltype(*std::forward<T>(m)>`
>> is `false`.

>> *Returns*: `bool(m) ? static_cast<RT>(*std::forward<T>(m)) : static_cast<RT>(std::forward<U>(u))`

>| `template <class R = void, maybe T, class I>`
>|     `constexpr auto or_invoke(T&& m, I&& invocable) -> ` *see below*`;`

>> If `R` is not `void`, the return type, `RT` is `R`, otherwise `RT` is
>> `common_type_t<decltype(*std::forward<T>(m)), invoke_result_t<I>>`.

>> *Mandates*: `RT` is a valid type.

>> *Returns*: `bool(m) ? static_cast<RT>(*m) : static_cast<RT>(std::forward<I>(invocable)());`
:::
