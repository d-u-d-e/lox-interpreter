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
        // global env
        env = std::make_shared<Environment>();
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
    expr::Value visit_variable_expr(const expr::Variable &expr) override;
    expr::Value visit_assignment_expr(const expr::Assignment &expr) override;
    expr::Value visit_logical_expr(const expr::Logical &expr) override;
    expr::Value visit_call_expr(const expr::Call &expr) override;

    void visit_print_stmt(stmt::Print &stmt) override;
    void visit_expr_stmt(stmt::Expression &stmt) override;
    void visit_vardecl_stmt(stmt::VariableDecl &stmt) override;
    void visit_block_stmt(stmt::Block &stmt) override;
    void visit_if_stmt(stmt::If &stmt) override;
    void visit_while_stmt(std::shared_ptr<stmt::While> stmt) override;
    void visit_fun_stmt(std::shared_ptr<stmt::Function> stmt) override;
    void visit_return_stmt(stmt::Return &stmt) override;

    void interpret(const std::vector<std::shared_ptr<stmt::StmtBase>> &stms, bool repl = false);
    static std::string stringify(const expr::Value &value);
    std::shared_ptr<Environment> get_env() { return env; }
    void execute_block(const std::vector<std::shared_ptr<stmt::StmtBase>> &stmts, std::unique_ptr<Environment> environ);

private:
    void check_number_operand(const Token &op, const expr::Value &operand);
    void check_number_operands(const Token &op, const expr::Value &left, const expr::Value &right);

    bool is_truthy(const expr::Value &value);
    bool is_equal(const expr::Value &left, expr::Value &right);
    expr::Value evaluate(const expr::ExprBase &expr) { return expr.accept(*this); }
    void execute(std::shared_ptr<stmt::StmtBase> stmt) { return stmt->accept(*this); }
    std::shared_ptr<Environment> env;
    bool repl{false};
    bool show_exp{false};
};