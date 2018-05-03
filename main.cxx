#include <iostream>
#include <utility>
#include <stack>
#include <queue>
#include <any>
#include <functional>
#include <string>
#include <type_traits>

using namespace std::string_literals;

struct Channel {
  std::queue<std::tuple<std::any, std::function<std::ostream & (std::ostream &, void *)>>> q;
};

struct Secret {
  int value;
};
std::ostream & operator << (std::ostream &os, Secret const &s) {
  return os << s.value;
}

template <typename T>
struct UnaryWrapper;

template <typename T>
struct UnaryWrapper<T &> {
  T &value;
};

template <typename T>
struct UnaryWrapper<T const &> {
  T const &value;
};

template <typename T>
struct UnaryWrapper<T &&> {
  T &&value;
  UnaryWrapper(T &&value_): value{std::forward<T>(value_)} {}
};

template <typename T>
struct UnaryWrapper<T const &&> {
  T const &&value;
  UnaryWrapper(T const &&value_): value{std::forward<T>(value_)} {}
};

template <std::size_t N>
struct UnaryWrapper<char const [N]>;

// template <std::size_t N>
// auto operator - (UnaryWrapper<char const [N]> rhs) {
//   return rhs;
// }

template <typename Value>
UnaryWrapper<Value &&> box(Value &&v) {
  return UnaryWrapper<Value &&>{std::forward<Value>(v)};
}

template <typename T>
auto operator - (T &&rhs) {
  return UnaryWrapper<T &&>{std::forward<T>(rhs)};
}

template <typename T>
auto operator - (T const &&rhs) {
  return UnaryWrapper<T const &&>{std::forward<T const>(rhs)};
}

template <typename T>
auto operator - (UnaryWrapper<T &&> &&rhs) {
  return std::forward<UnaryWrapper<T &&>>(rhs);
}

// template <std::size_t N>
// auto operator - (Literal<char const [N]> rhs) {
//   return UnaryWrapper<char const [N]>{rhs.value};
// }

auto operator - (Channel &rhs) {
  return UnaryWrapper<Channel &>{rhs};
}

struct AnyProxy {
  std::any el;

  template <typename T>
  operator T () {
    return std::any_cast<T>(el);
  }

  // friend std::ostream & operator << (std::ostream &os, AnyProxy const &rhs) {
  //   // os << rhs.el;
  // }
};

struct StubAssign {  
  AnyProxy operator < (UnaryWrapper<Channel &> rhs) {
    // std::any val = rhs.value.q.front();
    auto [val, print] = rhs.value.q.front();
    rhs.value.q.pop();

    std::cout << "Print: ";
    print(std::cout, &val);
    std::cout << '\n';

    return {val};
  }
};

namespace {
StubAssign _;
}

template <typename T>
constexpr auto is_printable(T const *t) ->
    decltype(std::declval<std::ostream>() << *t, std::true_type{}) { return {}; }
constexpr auto is_printable(...) ->
    std::false_type { return {}; }
          
template <typename T>
Channel & operator < (Channel &ch, UnaryWrapper<T &&> rhs) {
  // ch.q.emplace(std::forward<T>(rhs.value));
  ch.q.emplace(std::forward<T>(rhs.value),
               [](std::ostream &os, void *t) -> std::ostream & {
                 if constexpr (is_printable(static_cast<T const *>(nullptr))) {
                   os << std::any_cast<T const>(*static_cast<std::any const *>(t));
                 } else {
                   os << "<<non-printable>>";
                 }
                 return os;
               });
  return ch;
}

template <typename T>
requires std::is_arithmetic_v<T>
Channel & operator < (Channel &ch, T &&rhs) {
  return ch <- box(std::forward<T>(-rhs));
}

// template <std::size_t N>
// Channel & operator < (Channel &ch, UnaryWrapper<char const (&)[N]> rhs) {
//   ch.q.emplace(rhs.value);
//   std::cout << "emplaced " << rhs.value << '\n';
//   std::cout << "queue size is " << ch.q.size() << '\n';
//   return ch;
// }

template <typename T>
T & operator < (T &lhs, UnaryWrapper<Channel &> rhs) {
  // lhs = std::any_cast<T>(rhs.value.q.front());
  auto [val, print] = rhs.value.q.front();
  lhs = val;
  rhs.value.q.pop();

  std::cout << "Print: ";
  print(std::cout, &lhs);
  std::cout << '\n';

  return lhs;
}

std::any & operator < (std::any &lhs, UnaryWrapper<Channel &> rhs) {
  // lhs = rhs.value.q.front();
  auto [val, print] = rhs.value.q.front();
  lhs = val;
  rhs.value.q.pop();
  return lhs;
}

int main(int argc, char **argv) {
  Channel ch;

  ch <- "notasecret"s;
  ch <- Secret{42};
  ch <- 42;
  ch <- box((int*)(1));
  ch <- box((int*)(2));
  ch <- box(nullptr);
  ch <- box(nullptr);
  ch <- box((int*)(1));

  std::string password = _ <- ch;
  Secret      secret   = _ <- ch;
  int         num      = _ <- ch;
  auto        x        = _ <- ch;
  x        = _ <- ch;
  x        = _ <- ch;

  std::cout << "password : " << password << '\n';
  std::cout << "secret   : " << secret << '\n';
  std::cout << "num      : " << num << '\n';
  // std::cout << "x        : " << x << '\n';

  return 0;
}
