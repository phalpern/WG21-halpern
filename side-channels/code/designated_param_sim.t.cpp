/* designated_param_sim.t.cpp                                         -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#include <designated_param_sim.h>
#include <functional>
#include <iostream>
#include <string>
#include <memory>

/******************************************************************************
 *                              TESTING FRAMEWORK
 *****************************************************************************/

static int error_count = 0;

void test_assert_failure(const char* file, int line, const char* expr)
{
  ++error_count;
  std::cout << file << ':' << line << ": test assertion failed: " << expr
            << std::endl;
}

#define TEST_ASSERT(C)                                                  \
  if (C) (void) 0; else test_assert_failure(__FILE__, __LINE__, #C)

#define TEST_EQ(V1, V2)                                         \
  if ((V1) == (V2)) (void) 0;                                   \
  else test_assert_failure(__FILE__, __LINE__, #V1 " == " #V2)

#define TEST_SAME(T1, T2)                                               \
  static_assert(std::is_same_v<T1, T2>,                                 \
                "Types " #T1 " and " #T2 " should be the same")

#define TEST_TYPE_IS(VAL, T) TEST_SAME(decltype(VAL), T)

// Infrastructure for printing a type name.

/// Default `type_base_name` implementation uses `typeid`.
template <class T>
constexpr const char* base_type_name(const T&) { return typeid(T).name(); }

// This macro generates an overload of `base_type_name` that returns the string
// "T" when invoked with an object of type `T`. The use of `type_identity`
// treats `T` as a single type rather than a sequence of tokens, thus
// preventing syntax errors when `T` is a complex type such as `const char*` or
// `int []`.
#define BASE_TYPE_NAME(T) \
  constexpr const char* base_type_name(const std::type_identity_t<T>&) { return #T; }

BASE_TYPE_NAME(char)
BASE_TYPE_NAME(char*)
BASE_TYPE_NAME(const char*)
BASE_TYPE_NAME(short)
BASE_TYPE_NAME(int)
BASE_TYPE_NAME(long)
BASE_TYPE_NAME(float)
BASE_TYPE_NAME(double)
BASE_TYPE_NAME(std::string)
BASE_TYPE_NAME(std::nullptr_t)
BASE_TYPE_NAME(char[])

/// Used during debugging to print a type's name and value.
template <class T>
void print_type_and_value(const T& v)
{
  std::cout << '(';
  if constexpr (std::is_const_v<std::remove_reference_t<T>>)
    std::cout << "const ";

  std::cout << base_type_name(v);

  if constexpr (std::is_lvalue_reference_v<T>)
    std::cout << "&";
  else if constexpr (std::is_rvalue_reference_v<T>)
    std::cout << "&&";

  std::cout << ") " << v << std::endl;
}

template <class T>
void print_type_and_value(T&& v)
{
  print_type_and_value<T>(v);
}

#define PTV(V) do {                                                     \
    std::cout << #V << " = "; print_type_and_value<decltype(V)>(V);     \
  } while (false)

namespace dp = designated_params;

/******************************************************************************
 *                              USAGE EXAMPLES
 *****************************************************************************/

namespace usage1
{




// First simple example in paper

// Support types:
struct Square {
  double x, y, width, height;
  bool operator==(const Square&) const = default;
};
using Shape = Square;
using Transform = std::function<void (Shape&)>;
struct Point { double x, y; };

// Define a signature object for the following function:
// ```
// template <class Shape, class Transform>
// Shape munge(const Shape&     shape,       // normal parameter
//             const Transform& ..transform, // designated positional param
//             double           .scale     = 1.0,
//             const Point&     .translate = {0.0, 0.0});
// ```
// Note that the `shape` parameter and `transform` parameters cannot be fully
// emulated because the library not support deduced parameters. They are
// substituted here by known types.
dp::func_signature munge_sig(dp::param<const Shape&>(),
                             dp::param<Transform, ".transform">(),
                             dp::param<double, "scale">(1.0),
                             dp::param<const Point&,
                                       "translate">({ 0.0, 0.0 }));

// Ddefine `munge` function constrained by the above signature.
template <class... Args>
requires (munge_sig.is_viable<Args...>())
Shape munge(Args&&... args)
{
  auto [ shape, transform, scale, translate ] =
    munge_sig.get_param_values(std::forward<Args>(args)...);

  TEST_TYPE_IS(shape    , const Shape&);
  TEST_TYPE_IS(transform, Transform);
  TEST_TYPE_IS(scale    , double);
  TEST_TYPE_IS(translate, const Point&);

  Shape ret = shape;
  ret.width  *= scale;
  ret.height *= scale;
  ret.x      += translate.x;
  ret.y      += translate.y;
  transform(ret);

  return ret;
}

// Call `munge`
void run()
{
  Square myShape{ 1., 2., 3., 4. };

  // Make a call equivalent to
  // ```
  // auto newShape = munge(myShape,                     // bound to `shape`
  //                       [](Shape& s){ s.x -= 1.; },  // bound to `transform`
  //                       .scale     = 1.5);           // required
  // ```
  auto newShape = munge(myShape,                     // bound to `shape`
                        [](Shape& s){ s.x -= 1.; },  // bound to `transform`
                        dp::arg<"scale">(1.5));      // bound to `scale`

  TEST_EQ(newShape, Square(0., 2., 4.5, 6.));
}

} // close namespace usage1

namespace usage2 {

// `makeWindow` motivating uses case in paper:

// Supporting types
struct Color {
  unsigned char m_red, m_green, m_blue;

  static const Color black;
  static const Color white;
  static const Color grey50;

  bool operator==(const Color&) const = default;
};

constexpr inline Color Color::black  = {   0,   0,   0 };
constexpr inline Color Color::white  = { 255, 255, 255 };
constexpr inline Color Color::grey50 = { 128, 128, 128 };

struct WinAttr {
  int width, height, left, top;
  Color foreground, background;

  bool operator==(const WinAttr&) const = default;
};

using WinHandle = std::unique_ptr<WinAttr>;

// Signature for `makeWindow`.
dp::func_signature makeWindow_sig(
  dp::param<int, ".width">(), dp::param<int, ".height">(),
  dp::param<int, ".left">(INT_MIN), dp::param<int, ".top">(INT_MIN),
  dp::param<Color, ".background">(Color::white),
  dp::param<Color, ".foreground">(Color::black));

template <class... Args>
requires (makeWindow_sig.is_viable<Args...>())
WinHandle makeWindow(Args&&... args)
{
  auto [ width, height, left, top, background, foreground ] =
    makeWindow_sig.get_param_values(std::forward<Args>(args)...);

  return WinHandle(new WinAttr(width, height, left, top,
                               background, foreground));
}

void run()
{
  int top    = 100,  left  = 150;
  int height = 1100, width = 1000;

  auto w = makeWindow(dp::arg<"top">(top),       dp::arg<"left">(left),
                      dp::arg<"height">(height), dp::arg<"width">(width));
  TEST_EQ(*w, WinAttr(1000, 1100, 150, 100, Color::white, Color::black));

  w = makeWindow(width, height, dp::arg<"foreground">(Color::grey50));
  TEST_EQ(*w, WinAttr(1000, 1100, INT_MIN, INT_MIN,
                      Color::white, Color::grey50));

  w = makeWindow(dp::arg<"width">(1000), dp::arg<"height">(1500),
                 dp::arg<"left">(10),    dp::arg<"top">(20));
  TEST_EQ(*w, WinAttr(1000, 1500, 10, 20, Color::white, Color::black));
}

} // close namespace usage2

namespace usage3 {

// `calculate` motivating use case from paper

struct ThreadPool
{
  int m_count = 0;
  // ...
  template <class F>
  void schedule(F&& task) { ++m_count; std::forward<F>(task)(); }
};

ThreadPool defaultThreadPool;

// This library emulation does not handle variadic parameters, so we use a
// fixed parameter list.
dp::func_signature calculate_sig(dp::param<std::string>(""),
                                 dp::param<double>(0.0),
                                 dp::param<ThreadPool&,
                                           "threadpool">(defaultThreadPool));
template <class T, class... Args>
T calculate(T x = {}, Args&&... args)
{
  auto [ varg1, varg2, threadpool ]
    = calculate_sig.get_param_values(std::forward<Args>(args)...);
  threadpool.schedule([&]{ x += varg2; });
  return x;
}

void run()
{
  ThreadPool myThreadPool;

  TEST_EQ(0, defaultThreadPool.m_count);
  TEST_EQ(0, myThreadPool.m_count);

  auto r1 = calculate(19.4, "encode", 1.0);
  TEST_EQ(20.4, r1);
  TEST_EQ(1, defaultThreadPool.m_count);
  TEST_EQ(0, myThreadPool.m_count);

  auto r2 = calculate(19.4, "encode", 1.0,
                      dp::arg<"threadpool">(myThreadPool));
  TEST_EQ(20.4, r2);
  TEST_EQ(1, defaultThreadPool.m_count);
  TEST_EQ(1, myThreadPool.m_count);
}

} // close namespace usage3

namespace usage4 {

// `Point` example from paper.
// Note: This is probably not a particularly useful way to define a
// cartesian/polar point, but it gets the point across

dp::func_signature cartesian_ctor_sig(dp::param<double, ".x">(),
                                      dp::param<double, ".y">());
dp::func_signature polar_ctor_sig(dp::param<double, "radius">(),
                                  dp::param<double, "angle">());

class Point
{
public:
  enum CoordSystem { cartesian, polar };

private:
  CoordSystem m_coord_system;

  union {
    struct {
      double x;
      double y;
    } m_cartesian;
    struct {
      double radius;
      double angle;
    } m_polar;
  };

public:
  template <class... Args>
  requires (cartesian_ctor_sig.is_viable<Args...>())
  constexpr Point(Args... args) // Pass args by value
    : m_coord_system(cartesian)
  {
    auto [ x, y ] = cartesian_ctor_sig.get_param_values(args...);
    m_cartesian.x = x;
    m_cartesian.y = y;
  }

  template <class... Args>
  requires (polar_ctor_sig.is_viable<Args...>())
  constexpr Point(Args... args) // Pass args by value
    : m_coord_system(polar)
  {
    auto [ radius, angle ] = polar_ctor_sig.get_param_values(args...);
    m_polar.radius = radius;
    m_polar.angle  = angle;
  }

  constexpr CoordSystem coord_system() const { return m_coord_system; }
  constexpr double x() const { return m_cartesian.x; }
  constexpr double y() const { return m_cartesian.y; }
  constexpr double radius() const { return m_polar.radius; }
  constexpr double angle()  const { return m_polar.angle;  }
};

void run()
{
  Point p1(4.0, 3.0);
  TEST_EQ(p1.coord_system(), Point::cartesian);
  TEST_EQ(p1.x(), 4.0);
  TEST_EQ(p1.y(), 3.0);

  Point p2(dp::arg<"radius">(5.0), dp::arg<"angle">(0.5));
  TEST_EQ(p2.coord_system(), Point::polar);
  TEST_EQ(p2.x(), 5.0);
  TEST_EQ(p2.y(), 0.5);

  // Weird, but legal.  Note that it works as a `constexpr` definition.
  constexpr Point p3(4.0, dp::arg<"y">(3.0));
  TEST_EQ(p3.coord_system(), Point::cartesian);
  TEST_EQ(p3.x(), 4.0);
  TEST_EQ(p3.y(), 3.0);

  // Reversing order of arguments works.
  constexpr Point p4(dp::arg<"y">(3.0), dp::arg<"x">(4.0));
  TEST_EQ(p4.coord_system(), Point::cartesian);
  TEST_EQ(p4.x(), 4.0);
  TEST_EQ(p4.y(), 3.0);

  // Putting non-positiona arg second doesn't work.
  // constexpr Point p5( dp::arg<"y">(3.0), 4.0);
}

} // close namespace usage4

namespace usage5 {

// Variation of `Point` example from paper that avoids mixing positional and
// designated parmaters.

dp::func_signature cartesian_ctor_sig(dp::param<double, "x">(),
                                      dp::param<double, "y">());
dp::func_signature polar_ctor_sig(dp::param<double, "radius">(),
                                  dp::param<double, "angle">());

class Point
{
public:
  enum CoordSystem { cartesian, polar };

private:
  CoordSystem m_coord_system;

  union {
    struct {
      double x;
      double y;
    } m_cartesian;
    struct {
      double radius;
      double angle;
    } m_polar;
  };

public:
  constexpr Point(double x, double y) : m_coord_system(cartesian)
  {
    m_cartesian.x = x;
    m_cartesian.y = y;
  }

  template <class... Args>
  requires (cartesian_ctor_sig.is_viable<Args...>())
  constexpr Point(Args... args) // Pass args by value
    : m_coord_system(cartesian)
  {
    auto [ x, y ] = cartesian_ctor_sig.get_param_values(args...);
    m_cartesian.x = x;
    m_cartesian.y = y;
  }

  template <class... Args>
  requires (polar_ctor_sig.is_viable<Args...>())
  constexpr Point(Args... args) // Pass args by value
    : m_coord_system(polar)
  {
    auto [ radius, angle ] = polar_ctor_sig.get_param_values(args...);
    m_polar.radius = radius;
    m_polar.angle  = angle;
  }

  constexpr CoordSystem coord_system() const { return m_coord_system; }
  constexpr double x() const { return m_cartesian.x; }
  constexpr double y() const { return m_cartesian.y; }
  constexpr double radius() const { return m_polar.radius; }
  constexpr double angle()  const { return m_polar.angle;  }
};

void run()
{
  Point p1(4.0, 3.0);
  TEST_EQ(p1.coord_system(), Point::cartesian);
  TEST_EQ(p1.x(), 4.0);
  TEST_EQ(p1.y(), 3.0);

  Point p2(dp::arg<"radius">(5.0), dp::arg<"angle">(0.5));
  TEST_EQ(p2.coord_system(), Point::polar);
  TEST_EQ(p2.x(), 5.0);
  TEST_EQ(p2.y(), 0.5);

  // Mixed positional/designated no longer finds a matching constructor.
  // constexpr Point p3(4.0, dp::arg<"y">(3.0));
}

} // close namespace usage5

namespace usage6 {

// Revisit `makeWindow` to explore designated parameters that are never
// positional.

using usage2::Color;
using usage2::WinAttr;
using usage2::WinHandle;

dp::func_signature makeWindow_sig(
  dp::param<int, "width">(),       dp::param<int, "height">(),
  dp::param<int, "left">(INT_MIN), dp::param<int, "top">(INT_MIN),
  dp::param<Color, "background">(Color::white),
  dp::param<Color, "foreground">(Color::black),
  dp::param<bool, "keep_on_top">(false));

WinHandle makeWindow(int   width,                     int   height,
                     int   left,                      int   top = INT_MIN,
                     Color background = Color::white, Color foreground = Color::black)
{
  return WinHandle(new WinAttr(width, height, left, top,
                               background, foreground));
}

template <class... Args>
requires (makeWindow_sig.is_viable<Args...>())
WinHandle makeWindow(Args&&... args)
{
  auto [ width, height, left, top, background, foreground, keep_on_top ] =
    makeWindow_sig.get_param_values(std::forward<Args>(args)...);

  (void) keep_on_top;

  return WinHandle(new WinAttr(width, height, left, top,
                               background, foreground));
}

void run()
{
  int t = 100,  l = 150;
  int h = 1100, w = 1000;

  auto w1 = makeWindow(w, h, l, t);
  TEST_EQ(*w1, WinAttr(1000, 1100, 150, 100, Color::white, Color::black));

  auto w2 = makeWindow(dp::arg<"width">(w), dp::arg<"height">(h),
                       dp::arg<"left">(l),  dp::arg<"top">(t),
                       dp::arg<"keep_on_top">(true));
  TEST_EQ(*w2, WinAttr(1000, 1100, 150, 100, Color::white, Color::black));
}

} // close namespace usage6

namespace usage7 {

// Revisit `makeWindow` to explore designated parameters that are never
// positional.

using usage2::Color;
using usage2::WinAttr;
using usage2::WinHandle;

dp::func_signature makeWindow_sig(
  dp::param<int, ".width">(), dp::param<int, ".height">(),
  dp::param<int, ".left">(INT_MIN), dp::param<int, ".top">(INT_MIN),
  dp::param<Color, ".background">(Color::white),
  dp::param<Color, ".foreground">(Color::black),
  dp::param<bool, "keep_on_top">(false));

template <class... Args>
requires (makeWindow_sig.is_viable<Args...>())
WinHandle makeWindow(Args&&... args)
{
  auto [ width, height, left, top, background, foreground, keep_on_top ] =
    makeWindow_sig.get_param_values(std::forward<Args>(args)...);

  (void) keep_on_top;

  return WinHandle(new WinAttr(width, height, left, top,
                               background, foreground));
}

void run()
{
  int t = 100,  l = 150;
  int h = 1100, w = 1000;

  auto w1 = makeWindow(w, h, l, t);
  TEST_EQ(*w1, WinAttr(1000, 1100, 150, 100, Color::white, Color::black));

  auto w2 = makeWindow(w, h, l, t, dp::arg<"keep_on_top">(true));
  TEST_EQ(*w2, WinAttr(1000, 1100, 150, 100, Color::white, Color::black));
}

} // close namespace usage7

namespace usage8 {

// Examples from "Parameter and Argument Order"

dp::func_signature f1_sig(dp::param<int>(), dp::param<int>(0),
                          dp::param<double, "scale">(1.0));

template <class... Args>
requires (f1_sig.is_viable<Args...>())
void f1(Args&&...)
{
}

// Designated-parameter simulation does not support variadics
// template <class... V>
// void f2(int x, int .y = 0, int z = 0, V&&...v, ., int .origin, int .offset = 0);

void run()
{
  f1(9, 8, dp::arg<"scale">(2.0)); // All arguments are explicitly specified
  f1(9, dp::arg<"scale">(1.5));    // y = 0 by default
  f1(9, 8);                        // scale = 1.0 by default

  // Ill formed: undesignated arg after designated arg:
  // f1(9, dp::arg<"scale">(.5), 8);
}

} // close namespace usage8

namespace usage9 {

// Interaction with Overload Resolution

class Stream
{
public:
  enum class Mode { READ, WRITE, APPEND };

  static constexpr inline dp::func_signature
  open1_sig{dp::param<Mode, ".mode">(), dp::param<std::string, ".filename">()};

  static constexpr inline dp::func_signature
  open2_sig{dp::param<Mode, ".mode">(), dp::param<std::string, ".url">()};

  // distinct overloads of `open`
  template <class... Args>
  requires (open1_sig.is_viable<Args...>())
  int open(Args&&...) { return 1; } // overload #1

  template <class... Args>
  requires (open2_sig.is_viable<Args...>())
  int open(Args&&...) { return 2; } // overload #2
};

void run()
{
  Stream s;

  s.open(Stream::Mode::READ, dp::arg<"filename">("myinput.txt"));    // OK
  // s.open(Stream::Mode::READ, "https::/isocpp.org/myinput"); // Error: ambiguous
}

} // close namespace usage9

namespace usage10 {

// Interaction with Parameter Packs
template <class F, class... Args>
auto verbose_call(F&& f, Args&&... args) -> decltype(auto) {
  std::cout << "Prologue\n";
  auto on_scope_exit = [](const char* s){ std::cout << s; };
  std::unique_ptr<const char, decltype(on_scope_exit)> e("Epilogue\n",
                                                         on_scope_exit);
  return std::forward<F>(f)(std::forward<Args>(args)...);
}

dp::func_signature func1_sig(dp::param<int>(),
                             dp::param<const char*, "name">());

dp::func_signature func2_sig(dp::param<const char*>(),
                             dp::param<double, "angle">(),
                             dp::param<double, "radius">());


// In the paper, `func` is a non-template function. The designated-parameter
// library requires that the callee be a function template, so we wrap it in a
// `struct` to convert it into a (non-template) functor.
struct {
  template <class...Args>
  requires (func1_sig.is_viable<Args...>())
  static void operator()(Args&&... args)
  {
    auto [ i, name ] = func1_sig.get_param_values(std::forward<Args>(args)...);
    std::cout << "  func: i = " << i << ", name = \"" << name << "\"\n";
  }

  template <class...Args>
  requires (func2_sig.is_viable<Args...>())
  static void operator()(Args&&... args)
  {
    auto [ s, angle, radius ] =
      func2_sig.get_param_values(std::forward<Args>(args)...);
    PTV(s);
    PTV(angle);
    PTV(radius);
  }

} constexpr func;

dp::func_signature W_ctor_sig(dp::param<int, "value">(), dp::param<int, "offset">());

struct W
{
  int m_value;
  int m_offset;

  template <class... Args>
  requires (W_ctor_sig.is_viable<Args...>())
  W(Args&&... args) {
    std::tie(m_value, m_offset) =
      W_ctor_sig.get_param_values(std::forward<Args>(args)...);
  }
};

void run()
{
  verbose_call(func, 5, dp::arg<"name">("Fred"));

  constexpr  int x = 5;
  std::pair<int, W> p(std::piecewise_construct,
                      std::forward_as_tuple(99),
                      std::forward_as_tuple(dp::arg<"value">(x),
                                            dp::arg<"offset">(-1)));
  TEST_EQ(p.first, 99);
  TEST_EQ(p.second.m_value, x);
  TEST_EQ(p.second.m_offset, -1);

  auto t = std::make_tuple("Hello",
                           dp::arg<"angle">(30), dp::arg<"radius">(2.5));
  std::apply(func, t);
}

} // close namespace usage10

/******************************************************************************
 *                              END USAGE EXAMPLES
 *****************************************************************************/

// Test `designator_from_raw`
constexpr auto h_des = dp::designator_from_raw_v<"hello">;
constexpr auto x_des = dp::designator_from_raw_v<"x">;
constexpr auto y_des = dp::designator_from_raw_v<".y">;
static_assert(h_des == dp::designator_t<'h','e','l','l','o'>{});
static_assert(x_des == dp::designator_t<'x'>{});
static_assert(y_des == dp::designator_t<'y'>{});

// Parameter declarations
constexpr auto pp1    = dp::param<const double&>();      // const double& a
constexpr auto pp2    = dp::param<const char*>(nullptr); // const char *b =nullptr
constexpr auto xparam = dp::param<int, "x">();           // int .x
// TBD: Workound for inability to use default arguments that would materialize
// temporaries when used.
struct Five { operator int&&() const {
  static int r = 5; return std::move(r); }
} five;
constexpr auto yparam = dp::param<int&&, "y">(five);  // int&& .y = 5
//constexpr auto yparam = dp::param<int&&, "y">(5);   // int&& .y = 5
constexpr auto zparam = dp::param<std::string, ".z">( // string .z = "hello"
  [] -> std::string { return "hello"; });

using xparam_t = decltype(xparam);
using yparam_t = decltype(yparam);
using pp1_t    = decltype(pp1   );
using pp2_t    = decltype(pp2   );
using zparam_t = decltype(zparam);

// Designated-argument types
auto xarg   = dp::arg<"x">(short(9));   // .x = short(9)
auto nxarg  = dp::arg<"x">(nullptr);    // .x = nullptr
auto yarg   = dp::arg<"y">(99);         // .y = 99
auto zarg   = dp::arg<"z">("bye");      // .z = "bye"

using xarg_t  = decltype(xarg );
using nxarg_t = decltype(nxarg);
using yarg_t  = decltype(yarg );
using zarg_t  = decltype(zarg );

// White-box incremental testing
namespace test_details {

using namespace dp::details;

// test `can_return_without_dangling`
struct B { };
struct W : B { };
struct X { operator       W()   const; };
struct Y { operator const W&()  const; };
struct Z { operator       W&&() const; };

static_assert(  can_return_without_dangling_v<int, int>);
static_assert(! can_return_without_dangling_v<int, int&>);
static_assert(! can_return_without_dangling_v<int, const int&>);
static_assert(! can_return_without_dangling_v<int, int&&>);
static_assert(  can_return_without_dangling_v<W  , W  >);
static_assert(  can_return_without_dangling_v<W  , B  >);
static_assert(  std::is_convertible_v<W  , B  >);
static_assert(  can_return_without_dangling_v<W& , B  >);
static_assert(  can_return_without_dangling_v<W& , B& >);
static_assert(  can_return_without_dangling_v<W& , const B&>);
static_assert(  can_return_without_dangling_v<W&&, const B&>);
static_assert(  can_return_without_dangling_v<X  , W  >);
static_assert(! can_return_without_dangling_v<X  , const W&>);
static_assert(! can_return_without_dangling_v<X  , W&&>);
static_assert(  can_return_without_dangling_v<X& , W  >);
static_assert(! can_return_without_dangling_v<X& , const W&>);
static_assert(! can_return_without_dangling_v<X& , W&&>);
static_assert(  can_return_without_dangling_v<X&&, W  >);
static_assert(! can_return_without_dangling_v<X&&, const W&>);
static_assert(! can_return_without_dangling_v<X&&, W&&>);
static_assert(  can_return_without_dangling_v<Y  , W  >);
static_assert(  can_return_without_dangling_v<Y  , const W&>);
static_assert(! can_return_without_dangling_v<Y  , W&&>);
static_assert(  can_return_without_dangling_v<Y& , W  >);
static_assert(  can_return_without_dangling_v<Y& , const W&>);
static_assert(! can_return_without_dangling_v<Y& , W&&>);
static_assert(  can_return_without_dangling_v<Y&&, W  >);
static_assert(  can_return_without_dangling_v<Y&&, const W&>);
static_assert(! can_return_without_dangling_v<Y&&, W&&>);
static_assert(  can_return_without_dangling_v<Z  , W  >);
static_assert(! can_return_without_dangling_v<Z  , const W&>);
static_assert(  can_return_without_dangling_v<Z  , W&&>);
static_assert(  can_return_without_dangling_v<Z& , W  >);
static_assert(! can_return_without_dangling_v<Z& , const W&>);
static_assert(  can_return_without_dangling_v<Z& , W&&>);
static_assert(  can_return_without_dangling_v<Z&&, W  >);
static_assert(! can_return_without_dangling_v<Z&&, const W&>);
static_assert(  can_return_without_dangling_v<Z&&, W&&>);

// test `has_no_default`
static_assert(  has_no_default<xparam_t>::value);
static_assert(! has_no_default<yparam_t>::value);

// test `match_des`
static_assert(  match_des<x_des>::apply<xparam_t>::value);
static_assert(  match_des<y_des>::apply<yparam_t>::value);
static_assert(! match_des<x_des>::apply<yparam_t>::value);

// basic test for `match_da`
static_assert(  match_da<type_list<>, type_list<>>());
static_assert(! match_da<type_list<>, type_list<xarg_t>>());
static_assert(! match_da<type_list<xparam_t>, type_list<>>());
static_assert(  match_da<type_list<yparam_t>, type_list<>>());
static_assert(  match_da<type_list<xparam_t>, type_list<xarg_t>>());
static_assert(! match_da<type_list<xparam_t>, type_list<nxarg_t>>());
static_assert(! match_da<type_list<xparam_t>, type_list<yarg_t>>());
static_assert(! match_da<type_list<xparam_t, yparam_t>, type_list<yarg_t>>());
static_assert(  match_da<type_list<xparam_t, yparam_t>, type_list<xarg_t>>());
static_assert(  match_da<type_list<xparam_t, yparam_t>,
                type_list<xarg_t, yarg_t>>());
static_assert(! match_da<type_list<xparam_t, yparam_t>,
              type_list<nxarg_t, yarg_t>>());

// test parameter list
using params = type_list<pp1_t, pp2_t, zparam_t, xparam_t, yparam_t>;

// basic test for `match_pa`
static_assert(  match_pa<type_list<xparam_t>, type_list<xarg_t>>());
static_assert(! match_pa<type_list<xparam_t>, type_list<int>>());

static_assert(! match_pa<params, type_list<>>());
static_assert(! match_pa<params, type_list<int>>());
static_assert(  match_pa<params, type_list<int, const char(&)[4], const char*,
                                           xarg_t, yarg_t>>());
static_assert(  match_pa<params, type_list<int, const char(&)[4],
                                           xarg_t, yarg_t>>());
static_assert(  match_pa<params, type_list<float, xarg_t, yarg_t, zarg_t>>());
static_assert(  match_pa<params, type_list<float, yarg_t, xarg_t, zarg_t>>());
static_assert(! match_pa<params, type_list<yarg_t, xarg_t, zarg_t>>());
static_assert(! match_pa<params, type_list<float, yarg_t, zarg_t>>());
static_assert(  match_pa<params, type_list<float, xarg_t, zarg_t>>());
// The following should fail because argument `z` is specified both
// positionally and designated.
static_assert(! match_pa<params, type_list<int, const char(&)[4], const char*,
                                           xarg_t, yarg_t, zarg_t>>());

}

// test `is_designated_arg`
static_assert(  dp::is_designated_arg_v<xarg_t>);
static_assert(! dp::is_designated_arg_v<double>);

// A function signature for testing.  First argument is the same as pp1, but
// we need to test using an rvalue, which is more typical.
constexpr dp::func_signature sig1 = {
  dp::param<const double&>(), pp2, zparam, xparam, yparam
};

static_assert(! sig1.is_viable<>());
static_assert(! sig1.is_viable<int>());
static_assert(  sig1.is_viable<int, const char(&)[4], const char*,
                               xarg_t, yarg_t>());
static_assert(  sig1.is_viable<int, const char(&)[4], xarg_t, yarg_t>());
static_assert(  sig1.is_viable<float, xarg_t, yarg_t, zarg_t>());
static_assert(  sig1.is_viable<float, yarg_t, xarg_t, zarg_t>());
static_assert(! sig1.is_viable<yarg_t, xarg_t, zarg_t>());
static_assert(! sig1.is_viable<float, yarg_t, zarg_t>());
static_assert(  sig1.is_viable<float, xarg_t, zarg_t>());
static_assert(! sig1.is_viable<int, const char(&)[4], const char*,
                               xarg_t, yarg_t, zarg_t>()); // dup z

template <class... Args>
auto f1(Args&&... args) requires (sig1.is_viable<Args...>())
{
  auto [p1, p2, z, x, y] = sig1.get_param_values(std::forward<Args>(args)...);

  TEST_TYPE_IS(p1, const double&);
  TEST_TYPE_IS(p2, const char*);
  TEST_TYPE_IS(z , std::string);
  TEST_TYPE_IS(x , int);
  TEST_TYPE_IS(y , int&&);

#ifdef TRACE  // For debugging only
  std::cout << "\nf1 argument values:\n";
  PTV(p1);
  PTV(p2);
  PTV(z );
  PTV(x );
  PTV(y );
#endif

  return std::tuple{ p1, p2, z, x, y };
}

int main()
{
  usage1::run();
  usage2::run();
  usage3::run();
  usage4::run();
  usage5::run();
  usage6::run();
  usage7::run();
  usage8::run();
  usage9::run();
  usage10::run();

  {
    double d = 1.2;
    auto [p1, p2, z, x, y] = sig1.get_param_values(d, "a", "b",
                                                   dp::arg<"y">(99),
                                                   dp::arg<"x">(short(9)));

    TEST_TYPE_IS(p1, const double&);
    TEST_TYPE_IS(p2, const char*);
    TEST_TYPE_IS(z , std::string);
    TEST_TYPE_IS(x , int);
    TEST_TYPE_IS(y , int&&);

    d = 3.4;  // Ensure that the actual reference was captured
    TEST_EQ(std::tuple(3.4, "a", "b", 9, 99), std::tuple(p1, p2, z, x, y));
  }
  {
    constexpr double d = 1.2;
    auto [p1, p2, z, x, y] = sig1.get_param_values(d, "a",
                                                   dp::arg<"y">(99),
                                                   dp::arg<"x">(short(9)));

    TEST_TYPE_IS(p1, const double&);
    TEST_TYPE_IS(p2, const char*);
    TEST_TYPE_IS(z , std::string);
    TEST_TYPE_IS(x , int);
    TEST_TYPE_IS(y , int&&);

    TEST_EQ(std::tuple(1.2, "a", "hello", 9, 99), std::tuple(p1, p2, z, x, y));
  }

  TEST_EQ(std::tuple(1.2, "a", "b", 9, 99),
          f1(1.2,  "a", "b", dp::arg<"y">(99), dp::arg<"x">(short(9))));

  TEST_EQ(std::tuple(3.4, "c", "hello", 8, 88),
          f1(3.4,  "c", dp::arg<"x">(short(8)), dp::arg<"y">(88)));

  TEST_EQ(std::tuple(5.6, "d", "bye", 7, 5),
          f1(5.6,  "d", dp::arg<"x">(7), dp::arg<"z">("bye")));

  return error_count;
}


// Local Variables:
// c-basic-offset: 2
// End:
