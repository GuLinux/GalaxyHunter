#ifndef UTILS_SEQUENCE_H
#define UTILS_SEQUENCE_H
#include <functional>

template<typename T, T defaultValue, typename check_operator = std::equal_to<T>>
class sequence {
public:
  typedef std::function<T()> run_function;
  typedef std::function<void(const T &, const std::string &)> on_error_f;
  struct run {
    run_function f;
    std::string label;
    T check;
    run(run_function f, const std::string &label = {}, T check = defaultValue) : f(f), label(label), check(check) {}
  };
  template <typename ...Args>
  sequence(const std::list<run> &runs) : runs(runs), _check_operator(check_operator{}) {}
  ~sequence() {
    for(auto r: runs) {
      T result = r.f();
      if(! _check_operator(result, r.check)) {
	_run_on_error(result, r.label);
	return;
      }
    };
    _run_last();
  }
  sequence &on_error(on_error_f run_on_error) { _run_on_error = run_on_error; return *this; }
  sequence &run_last(std::function<void()> run_last) { _run_last = run_last; return *this; }
private:
  std::list<run> runs;
  on_error_f _run_on_error = [](const T&, const std::string&) {};
  check_operator _check_operator;
  std::function<void()> _run_last = []{};
};

#ifdef IN_IDE_PARSER
#define sequence_run(...) {}
#else
#define sequence_run(...) { __VA_ARGS__ , #__VA_ARGS__}
#endif

#endif