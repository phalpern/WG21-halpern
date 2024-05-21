In the summary for `optional<T>`, *in the Working Draft*, we need 2 overloads
of `value_or` (replacing the two that are there):

> `template <class U = remove_cvref_t<T>, class Self, class... Args>`
>     `U value_or(this Self&& self, Args&&... args);`
>
> `template <class U = remove_cvref_t<T>, class Self, class X, class... Args>`
>     `U value_or(this Self&& self, initializer_list<X> il, Args&&... args);`

In the `optional<T&>` wording *in P2988R4*, change `value_or` in
[optional.optional.general] to:

> `template <class U = remove_cv_t<T>, class... Args>`
>     `U value_or(Args&&...) const;`
> `template <class U = remove_cv_t<T>, class X, class... Args>`
>     `U constexpr value_or(initializer_list<X>, Args&&...) const;`


At the end of [optional.observe] *in the Working Draft*, replace the
definitions of `value_or`:

> `template <class U = remove_cvref_t<T>, class Self, class... Args>`
>     `U value_or(this Self&& self, Args&&... args);`

> *Mandates*: `is_constructible_v<U, decltype(*std::forward<Self>(self))> &&`
> `is_constructible_v<U, Args...>` \
> is `true`. If `U` is an lvalue reference type, then `sizeof...(Args)` is
> `1` and, for `A0` being \
> the single type in `Args`, `reference_constructs_from_temporary_v<U, A0>`
> is `false`.

> *Effects*: Equivalent to:

>> `return self.has_value() ? U(*std::forward<Self>(self)) : U(std::forward<Args>(args)...);`

> `template <class U = remove_cvref_t<T>, class Self, class X, class... Args>`
>     `U value_or(this Self&& self, initializer_list<X> il, Args&&... args);`

> *Mandates*: `! is_reference_v<U> &&`
> `is_constructible_v<U, initializer_list<X>, Args...> &&` \
> `is_constructible_v<U, decltype(*std::forward<Self>(self))>` is `true`.

> *Effects*: Equivalent to:

>> `return self.has_value() ? U(*std::forward<Self>(self)) : U(il, std::forward<Args>(args)...);`


In the `optional<T&>` wording *in P2988R4*, change the
definition of `value_or` to the following [optional.observe] :

> `template <class U = T, class... Args>`
>     `U value_or(Args&&...args) const;`

> *Mandates*: `is_constructible_v<U, decltype(**this)> &&`
> `is_constructible_v<U, Args...>` \
> is `true`. If `U` is an lvalue reference type, then `sizeof...(Args)` is
> `1` and, for `A0` being \
> the single type in `Args`, `reference_constructs_from_temporary_v<U, A0>`
> is `false`.

> *Effects*: Equivalent to:

>> `return has_value() ? U(**this) : U(std::forward<Args>(args)...);`

> `template <class U = remove_cv_t<T>, class X, class... Args>`
>     `U constexpr value_or(initializer_list<X> il, Args&&...) const;`

> *Mandates*: `! is_reference_v<U> &&`
> `is_constructible_v<U, initializer_list<X>, Args...> &&` \
> `is_constructible_v<U, decltype(**this)>` is `true`.

> *Effects*: Equivalent to:

>> `return has_value() ? U(**this) : U(il, std::forward<Args>(args)...);`
