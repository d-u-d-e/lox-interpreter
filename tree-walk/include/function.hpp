#pragma once
#include <callable.hpp>
#include <memory>

namespace stmt
{
    class Function;
};

class LoxFunction : public LoxCallable
{
public:
    LoxFunction(std::shared_ptr<const stmt::Function> decl) : declaration(decl) {}
    expr::Value call(Interpreter &interpreter, const std::vector<expr::Value> &args) override;
    int arity() const override;
    std::string to_string() const override;

private:
    std::shared_ptr<const stmt::Function> declaration;
};