#pragma once
#include <string>
#include <vector>
#include <interpreter.hpp>

class LoxCallable
{
public:
  virtual expr::Value call(Interpreter &interpreter, const std::vector<expr::Value> &args) = 0;
  virtual int arity() const = 0;
  virtual ~LoxCallable() = default;
  virtual std::string to_string() const = 0;
};