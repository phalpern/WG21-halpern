In the summary for `optional<T>`, we need 4 overloads of `value_or`:

> `template <class U = remove_cvref_t<T>, class... Args>`
>     `U constexpr value_or(Args&&...);`
> `template <class U = remove_cvref_t<T>, class... Args>`
>     `U constexpr value_or(Args&&...) const;`
> `template <class U = remove_cvref_t<T>, class X, class... Args>`
>     `U constexpr value_or(initializer_list<X>, Args&&...);`
> `template <class U = remove_cvref_t<T>, class X, class... Args>`
>     `U constexpr value_or(initializer_list<X>, Args&&...) const;`

In the summary for `optional<T&>`, we need 2 overloads of `value_or`:

> `template<class U> constexpr` [`T&`]{.rm} [*see below*]{.add} `value_or(U&&) const;`
> `template <class U = remove_cv_t<T>, class... Args>`
>     `U value_or(Args&&...) const;`
> `template <class U = remove_cv_t<T>, class X, class... Args>`
>     `U constexpr value_or(initializer_list<X>, Args&&...) const;`

In the details for `optional<T>`, change `value_or` to this:

> `template <class U = remove_cvref_t<T>, class... Args>`
>     `constexpr U value_or(Args&&... args);`
> `template <class U = remove_cvref_t<T>, class... Args>`
>     `constexpr U value_or(Args&&... args) const;`

> *Mandates*: `is_constructible_v<U, decltype(**this)> &&`
> `is_constructible_v<U, Args...>` \
> is `true`. If `U` is an lvalue reference type, then `sizeof...(Args)` is
> `1` and, for `A0` being \
> the single type in `Args`, `reference_constructs_from_temporary_v<U, A0>`
> is `false`.

> *Effects*: Equivalent to:

>> `return has_value() ? U(**this) : U(std::forward<Args>(args)...);`

> `template <class U = remove_cvref_t<T>, class X, class... Args>`
>     `constexpr U value_or(initializer_list<X> il, Args&&...);`
> `template <class U = remove_cvref_t<T>, class X, class... Args>`
>     `constexpr U value_or(initializer_list<X> il, Args&&...) const;`

> *Mandates*: `! is_reference_v<U> &&`
> `is_constructible_v<U, initializer_list<X>, Args...> &&` \
> `is_constructible_v<U, decltype(**this)>` is `true`.

> *Effects*: Equivalent to:

>> `return has_value() ? U(**this) : U(il, std::forward<Args>(args)...);`

In the details `optional<T&>`, make `value_or` be this:

> `template <class U = remove_cvref_t<T>, class... Args>`
>     `constexpr U value_or(Args&&...args) const;`

> *Mandates*: `is_constructible_v<U, decltype(**this)> &&`
> `is_constructible_v<U, Args...>` \
> is `true`. If `U` is an lvalue reference type, then `sizeof...(Args)` is
> `1` and, for `A0` being \
> the single type in `Args`, `reference_constructs_from_temporary_v<U, A0>`
> is `false`.

> *Effects*: Equivalent to:

>> `return has_value() ? U(**this) : U(std::forward<Args>(args)...);`

> `template <class U = remove_cv_t<T>, class X, class... Args>`
>     `constexpr U value_or(initializer_list<X> il, Args&&...) const;`

> *Mandates*: `! is_reference_v<U> &&`
> `is_constructible_v<U, initializer_list<X>, Args...> &&` \
> `is_constructible_v<U, decltype(**this)>` is `true`.

> *Effects*: Equivalent to:

>> `return has_value() ? U(**this) : U(il, std::forward<Args>(args)...);`
