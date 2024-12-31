#pragma once
#include <ast/stmt.hpp>
#include <unordered_map>
#include <vector>
#include <visitor.hpp>

namespace stmt
{
  class StmtBase;
};

class Resolver : public expr::Visitor<void>, public stmt::Visitor<void>
{
public:
  Resolver(Interpreter &interpreter) : interpreter(interpreter) {}
  void visit_binary_expr(const expr::Binary &expr) override;
  void visit_grouping_expr(const expr::Grouping &expr) override;
  void visit_literal_expr(const expr::Literal &expr) override;
  void visit_unary_expr(const expr::Unary &expr) override;
  void visit_variable_expr(const std::shared_ptr<const expr::Variable> &expr) override;
  void visit_assignment_expr(const std::shared_ptr<const expr::Assignment> &expr) override;
  void visit_logical_expr(const expr::Logical &expr) override;
  void visit_call_expr(const expr::Call &expr) override;
  void visit_get_expr(const expr::Get &expr) override;
  void visit_set_expr(const expr::Set &expr) override;

  void visit_print_stmt(const stmt::Print &stmt) override;
  void visit_expr_stmt(const stmt::Expression &stmt) override;
  void visit_vardecl_stmt(const stmt::VariableDecl &stmt) override;
  void visit_block_stmt(const stmt::Block &stmt) override;
  void visit_if_stmt(const stmt::If &stmt) override;
  void visit_while_stmt(const stmt::While &stmt) override;
  void visit_fun_stmt(const std::shared_ptr<const stmt::Function> &stmt) override;
  void visit_return_stmt(const stmt::Return &stmt) override;
  void visit_class_stmt(const std::shared_ptr<const stmt::Class> &stmt) override;

  void resolve(const std::vector<std::shared_ptr<stmt::StmtBase>> &statements);

private:
  enum class FunctionType {
    NONE,
    FUNCTION,
    METHOD
  };

  std::vector<std::unordered_map<std::string, bool>> scopes;
  Interpreter &interpreter;
  FunctionType current_func{FunctionType::NONE};

  void begin_scope() { scopes.push_back(std::unordered_map<std::string, bool>()); }
  void end_scope() { scopes.pop_back(); }
  void resolve(const std::shared_ptr<stmt::StmtBase> &stm) { stm->accept(*this); }
  void resolve(const std::shared_ptr<expr::ExprBase> &expr) { expr->accept(*this); }

  void declare(const Token &token);
  void define(const Token &token);
  void resolve_local(const std::shared_ptr<const expr::ExprBase> &expr, const Token &token);
  void resolve_function(const std::shared_ptr<const stmt::Function> &function, FunctionType type);
};