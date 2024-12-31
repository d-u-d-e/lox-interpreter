#pragma once
#include <class.hpp>

class LoxInstance
{
public:
  LoxInstance(std::shared_ptr<LoxClass> klass) : klass(std::move(klass)) {}
  std::string to_string() const { return klass->to_string() + " instance"; }

private:
  std::shared_ptr<LoxClass> klass;
};