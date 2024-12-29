#pragma once
#include <ast/expr.hpp>
#include <ast/stmt.hpp>
#include <iostream>
#include <vector>
#include <environment.hpp>

class Interpreter : public expr::Visitor<expr::Value>,
                    public stmt::Visitor<void>
{
public:
    Interpreter()
    {
        globals = std::make_shared<Environment>();
        environ = globals;
    }

    struct RuntimeError : public std::runtime_error
    {
        RuntimeError(const Token &token, const std::string &message) : std::runtime_error(message), token(token) {}
        Token token;
    };

    expr::Value visit_binary_expr(const expr::Binary &expr) override;
    expr::Value visit_grouping_expr(const expr::Grouping &expr) override;
    expr::Value visit_literal_expr(const expr::Literal &expr) override;
    expr::Value visit_unary_expr(const expr::Unary &expr) override;
    expr::Value visit_variable_expr(const std::shared_ptr<const expr::Variable> & expr) override;
    expr::Value visit_assignment_expr(const std::shared_ptr<const expr::Assignment> & expr) override;
    expr::Value visit_logical_expr(const expr::Logical &expr) override;
    expr::Value visit_call_expr(const expr::Call &expr) override;

    void visit_print_stmt(const stmt::Print &stmt) override;
    void visit_expr_stmt(const stmt::Expression &stmt) override;
    void visit_vardecl_stmt(const stmt::VariableDecl &stmt) override;
    void visit_block_stmt(const stmt::Block &stmt) override;
    void visit_if_stmt(const stmt::If &stmt) override;
    void visit_while_stmt(const std::shared_ptr<const stmt::While> & stmt) override;
    void visit_fun_stmt(const std::shared_ptr<const stmt::Function> & stmt) override;
    void visit_return_stmt(const stmt::Return &stmt) override;

    void interpret(const std::vector<std::shared_ptr<stmt::StmtBase>> &stms, bool repl = false);
    static std::string stringify(const expr::Value &value);
    void execute_block(const std::vector<std::shared_ptr<stmt::StmtBase>> &stmts, std::unique_ptr<Environment> environ);

    void resolve(std::shared_ptr<const expr::ExprBase> expr, int depth)
    {
        locals.emplace(std::move(expr), depth);
    };

private:
    void check_number_operand(const Token &op, const expr::Value &operand);
    void check_number_operands(const Token &op, const expr::Value &left, const expr::Value &right);
    bool is_truthy(const expr::Value &value);
    bool is_equal(const expr::Value &left, const expr::Value &right);
    expr::Value evaluate(expr::ExprBase &expr) { return expr.accept(*this); }
    void execute(const std::shared_ptr<const stmt::StmtBase> & stmt) { return stmt->accept(*this); }
    expr::Value lookup_variable(const Token &name, const std::shared_ptr<const expr::ExprBase> &expr);

    // distance map updated by resolver
    std::unordered_map<std::shared_ptr<const expr::ExprBase>, int> locals;
    std::shared_ptr<Environment> globals;
    std::shared_ptr<Environment> environ;
    bool repl{false};
    bool show_exp{false};
};