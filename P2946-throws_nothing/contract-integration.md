Future Integration with Contracts
=================================

The proposed Contracts facility (see
[P2900](http:://wg21.link/P2900)) and the `[[throws_nothing]]` facility
overlap in a number of ways, and suggestions have emerged whereby these
features can be harmonized,
resulting in a more powerful and consistent Standard.

* `[[throws_nothing]]` can be considered a special type of contract check ---
  similar to the Contracts proposal's `pre` and `post` annotations --- but
  evaluated at a distinct time, i.e., when an exception tries to escape. The
  proposed Contracts facility gives the programmer control, via a
  user-replaceable violation handler, over how detected logic errors are
  managed, e.g., logging and terminating, logging and somehow continuing, and
  so on.  Affording the user the same level of control
  over how an escaping exception is handled by a `[[throws_nothing]]`
  function would be a powerful feature. Thus, an integrated facility would
  allow the contract-violation handler to be called prior to terminating or
  propagating the exception.

* Integration between `[[throws_nothing]]` and contract annotations would be
  more seamless if their syntaxes were harmonized. If the Contracts facility
  ends up using a nonattribute syntax, then `[[throws_nothing]]` might be
  changed to a similar nonattribute syntax (assuming the two features first
  ship in the same Standard). As described earlier, `[[throws_nothing]]`
  cannot, in today's C++, appear at the end of a function declaration (the
  author's preferred position) and still appertain to the function.
  Pre- and post-condition annotations will almost certainly go after the
  function declaration, however, so `[[throws_nothing]]` could conceivably
  benefit from any change to the standard that makes it possible to put
  contract annotations there.

* In a build mode in which contract-checks are enabled and a throwing
  contract-violation handler is installed, the programmer would want
  contract-violation exceptions to be allowed to escape a `[[throws_nothing]]`
  function
  rather than terminating. If we integrate `[[throws_nothing]]` with Contracts,
  the compiler could automatically determine that we are using a
  contract-checking build mode and allow exceptions to escape a
  `[[throws_nothing]]` function.

* If contract annotations are given additional controls, many of those controls
  might also apply to `[[throws_nothing]]` (shown here using a straw-man syntax
  borrowed from an earlier contracts paper).

```C++
     void f(int) [[ throws_nothing ]];          // Obey global build mode.
     void g(int) [[ throws_nothing audit ]];    // observed only in audit build mode
     void h(int) [[ throws_nothing enforce ]];  // always enforced
     void j(int) [[ throws_nothing assume ]];   // Assume nonthrowing; don't check.
```

> The final line above (function `j`) would generate the smallest code because
neither the caller nor callee would need to generate any exception-handling
code. This line is also the least safe because, just as in the case of other
contract
annotations, assuming something that turns out to be false would result in
(language) undefined behavior.
