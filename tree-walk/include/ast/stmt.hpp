#pragma once

#include <ast/expr.hpp>
#include <memory>
#include <vector>
#include <visitor.hpp>

namespace stmt
{

  class StmtBase
  {
  public:
    virtual ~StmtBase() = default;
    virtual void accept(Visitor<void> &visitor) const = 0;
  };

  // This is an expression statement,
  // i.e. an expression followed by a semicolon
  struct Expression : public StmtBase {
    Expression(std::shared_ptr<expr::ExprBase> &&ex) : ex(std::move(ex)) {}
    void accept(Visitor<void> &visitor) const override { visitor.visit_expr_stmt(*this); }
    std::shared_ptr<expr::ExprBase> ex;
  };

  struct VariableDecl : public StmtBase {
    VariableDecl(const Token &token, std::shared_ptr<expr::ExprBase> &&initializer)
        : token(token), initializer(std::move(initializer))
    {}
    void accept(Visitor<void> &visitor) const override { visitor.visit_vardecl_stmt(*this); }
    Token token;
    std::shared_ptr<expr::ExprBase> initializer;
  };

  struct Print : public StmtBase {
    Print(std::shared_ptr<expr::ExprBase> &&ex) : ex(std::move(ex)) {}
    void accept(Visitor<void> &visitor) const override { visitor.visit_print_stmt(*this); }
    std::shared_ptr<expr::ExprBase> ex;
  };

  struct Block : public StmtBase {
    Block(std::vector<std::shared_ptr<StmtBase>> &&statements) : statements(std::move(statements))
    {}
    void accept(Visitor<void> &visitor) const override { visitor.visit_block_stmt(*this); }
    std::vector<std::shared_ptr<stmt::StmtBase>> statements;
  };

  struct If : public StmtBase {
    If(std::shared_ptr<expr::ExprBase> &&condition, std::shared_ptr<stmt::StmtBase> &&then_stm,
       std::shared_ptr<stmt::StmtBase> &&else_stm)
        : condition(std::move(condition)), then_stm(std::move(then_stm)), else_stm(std::move(else_stm))
    {}
    void accept(Visitor<void> &visitor) const override { visitor.visit_if_stmt(*this); }
    std::shared_ptr<expr::ExprBase> condition;
    std::shared_ptr<stmt::StmtBase> then_stm;
    std::shared_ptr<stmt::StmtBase> else_stm;
  };

  struct While : public StmtBase, public std::enable_shared_from_this<const While> {
    While(std::shared_ptr<expr::ExprBase> &&condition, std::shared_ptr<stmt::StmtBase> &&body)
        : condition(std::move(condition)), body(std::move(body))
    {}
    void accept(Visitor<void> &visitor) const override
    {
      visitor.visit_while_stmt(shared_from_this());
    }
    std::shared_ptr<expr::ExprBase> condition;
    std::shared_ptr<stmt::StmtBase> body;
  };

  struct Function : public StmtBase, public std::enable_shared_from_this<const Function> {
    Function(const Token &name, const std::vector<Token> &params,
             std::vector<std::shared_ptr<stmt::StmtBase>> &&body)
        : name(name), params(params), body(std::move(body))
    {}
    void accept(Visitor<void> &visitor) const override
    {
      visitor.visit_fun_stmt(shared_from_this());
    }
    Token name;
    std::vector<Token> params;
    std::vector<std::shared_ptr<stmt::StmtBase>> body;
  };

  struct Return : public StmtBase {
    Return(const Token &keyword, std::shared_ptr<expr::ExprBase> &&value)
        : keyword(keyword), value(std::move(value))
    {}
    void accept(Visitor<void> &visitor) const override { visitor.visit_return_stmt(*this); }
    Token keyword;
    std::shared_ptr<expr::ExprBase> value;
  };

} // namespace stmt
