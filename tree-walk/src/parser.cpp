#include <lox.hpp>
#include <parser.hpp>

std::shared_ptr<expr::ExprBase> Parser::expression() { return assignment(); }

std::shared_ptr<expr::ExprBase> Parser::equality()
{
  auto expr = comparison();
  while(match(Token::TokenType::BANG_EQUAL, Token::TokenType::EQUAL_EQUAL)) {
    auto op = previous();
    auto right = comparison();
    expr = std::make_shared<expr::Binary>(std::move(expr), op, std::move(right));
  }
  return expr;
}

std::shared_ptr<expr::ExprBase> Parser::comparison()
{
  auto expr = term();
  while(match(Token::TokenType::GREATER, Token::TokenType::GREATER_EQUAL, Token::TokenType::LESS,
              Token::TokenType::LESS_EQUAL)) {
    auto op = previous();
    auto right = term();
    expr = std::make_shared<expr::Binary>(std::move(expr), op, std::move(right));
  }
  return expr;
}

std::shared_ptr<expr::ExprBase> Parser::term()
{
  auto expr = factor();
  while(match(Token::TokenType::MINUS, Token::TokenType::PLUS)) {
    auto op = previous();
    auto right = factor();
    expr = std::make_shared<expr::Binary>(std::move(expr), op, std::move(right));
  }
  return expr;
}

std::shared_ptr<expr::ExprBase> Parser::factor()
{
  auto expr = unary();
  while(match(Token::TokenType::SLASH, Token::TokenType::STAR)) {
    auto op = previous();
    auto right = unary();
    expr = std::make_shared<expr::Binary>(std::move(expr), op, std::move(right));
  }
  return expr;
}

std::shared_ptr<expr::ExprBase> Parser::unary()
{
  if(match(Token::TokenType::BANG, Token::TokenType::MINUS)) {
    auto op = previous();
    auto right = unary();
    return std::make_shared<expr::Unary>(op, std::move(right));
  }
  return call();
}

std::shared_ptr<expr::ExprBase> Parser::primary()
{
  if(match(Token::TokenType::FALSE)) {
    return std::make_shared<expr::Literal>(Token::Literal{false});
  }
  else if(match(Token::TokenType::TRUE)) {
    return std::make_shared<expr::Literal>(Token::Literal{true});
  }
  else if(match(Token::TokenType::NIL)) {
    return std::make_shared<expr::Literal>(Token::Literal{});
  }
  else if(match(Token::TokenType::NUMBER, Token::TokenType::STRING)) {
    return std::make_shared<expr::Literal>(previous().get_literal());
  }
  else if(match(Token::TokenType::IDENTIFIER)) {
    return std::make_shared<expr::Variable>(previous());
  }
  else if(match(Token::TokenType::LEFT_PAREN)) {
    auto expr = expression();
    consume(Token::TokenType::RIGHT_PAREN, "Expect ')' after expression.");
    return std::make_shared<expr::Grouping>(std::move(expr));
  }
  else if(match(Token::TokenType::THIS)) {
    return std::make_shared<expr::This>(previous());
  }
  else if(match(Token::TokenType::SUPER)) {
    auto keyword = previous();
    consume(Token::TokenType::DOT, "Expect '.' after 'super'.");
    auto method = consume(Token::TokenType::IDENTIFIER, "Expect superclass method name.");
    return std::make_shared<expr::Super>(keyword, method);
  }
  throw error(peek(), "Expect expression.");
}

std::shared_ptr<expr::ExprBase> Parser::assignment()
{
  // trick: parse the left side as a single expression so that we automatically stop
  // at the first '='
  auto expr = logical_or();

  if(match(Token::TokenType::EQUAL)) {
    auto equals = previous();
    auto value = assignment(); // recursively parse the right side

    if(typeid(*expr) == typeid(expr::Variable)) {
      auto &name = dynamic_cast<expr::Variable &>(*expr);
      return std::make_shared<expr::Assignment>(name.token, std::move(value));
    }
    else if(typeid(*expr) == typeid(expr::Get)) {
      auto &get = dynamic_cast<expr::Get &>(*expr);
      // this is actually a set expression, so change the node type
      return std::make_shared<expr::Set>(std::move(get.object), get.name, std::move(value));
    }
    error(equals, "Invalid assignment target.");
  }
  return expr;
}

std::shared_ptr<expr::ExprBase> Parser::logical_or()
{
  auto expr = logical_and();
  while(match(Token::TokenType::OR)) {
    auto op = previous();
    auto right = logical_and();
    expr = std::make_shared<expr::Logical>(std::move(expr), op, std::move(right));
  }
  return expr;
}
std::shared_ptr<expr::ExprBase> Parser::logical_and()
{
  auto expr = equality();
  while(match(Token::TokenType::AND)) {
    auto op = previous();
    auto right = equality();
    expr = std::make_shared<expr::Logical>(std::move(expr), op, std::move(right));
  }
  return expr;
}

std::shared_ptr<expr::ExprBase> Parser::call()
{
  auto expr = primary();

  while(true) {
    if(match(Token::TokenType::LEFT_PAREN)) {
      expr = finish_call(std::move(expr));
    }
    else if(match(Token::TokenType::DOT)) {
      auto name = consume(Token::TokenType::IDENTIFIER, "Expect property name after '.'.");
      expr = std::make_shared<expr::Get>(std::move(expr), name);
    }
    else {
      break;
    }
  }
  return expr;
}

std::shared_ptr<expr::ExprBase> Parser::finish_call(std::shared_ptr<expr::ExprBase> callee)
{
  std::vector<std::shared_ptr<expr::ExprBase>> arguments;
  if(!check(Token::TokenType::RIGHT_PAREN)) {
    do {
      if(arguments.size() >= 255) {
        // don't throw here because the parser is not confused
        // just report the error
        error(peek(), "Can't have more than 255 arguments.");
      }
      arguments.push_back(expression());
    } while(match(Token::TokenType::COMMA));
  }
  auto paren = consume(Token::TokenType::RIGHT_PAREN, "Expect ')' after arguments.");
  return std::make_shared<expr::Call>(std::move(callee), paren, std::move(arguments));
}

Parser::ParseError Parser::error(Token token, const std::string &message)
{
  Lox::error(token, message);
  return ParseError();
}

Token Parser::consume(Token::TokenType type, const std::string &message)
{
  if(check(type)) {
    return advance();
  }
  else {
    throw error(peek(), message);
  }
}

std::shared_ptr<stmt::StmtBase> Parser::statement()
{
  if(match(Token::TokenType::PRINT)) {
    return print_statement();
  }
  else if(match(Token::TokenType::WHILE)) {
    return while_statement();
  }
  else if(match(Token::TokenType::FOR)) {
    return for_statement();
  }
  else if(match(Token::TokenType::RETURN)) {
    return return_statement();
  }
  else if(match(Token::TokenType::LEFT_BRACE)) {
    // why not just block()?
    // because block() is used to parse function blocks and we don't want the result to
    // be embedded in a stmt::Block
    return std::make_shared<stmt::Block>(block());
  }
  else if(match(Token::TokenType::IF)) {
    return if_statement();
  }
  return expr_statement(false);
}

std::shared_ptr<stmt::Print> Parser::print_statement()
{
  auto expr = expression();
  consume(Token::TokenType::SEMICOLON, "Expect ';' after value.");
  return std::make_shared<stmt::Print>(std::move(expr));
}

std::shared_ptr<stmt::Expression> Parser::expr_statement(bool parse_semicolon_in_repl)
{
  auto expr = expression();
  if(!repl || parse_semicolon_in_repl || peek().get_type() == Token::TokenType::SEMICOLON) {
    consume(Token::TokenType::SEMICOLON, "Expect ';' after expression.");
  }
  return std::make_shared<stmt::Expression>(std::move(expr));
}

std::shared_ptr<stmt::StmtBase> Parser::declaration()
{
  try {
    if(match(Token::TokenType::VAR)) {
      return var_declaration();
    }
    else if(match(Token::TokenType::FUN)) {
      return function("function");
    }
    else if(match(Token::TokenType::CLASS)) {
      return class_declaration();
    }
    return statement();
  }
  catch(const ParseError &) {
    synchronize();
    return nullptr;
  }
}

std::shared_ptr<stmt::VariableDecl> Parser::var_declaration()
{
  Token name = consume(Token::TokenType::IDENTIFIER, "Expect variable name.");
  std::shared_ptr<expr::ExprBase> initializer{};

  if(match(Token::TokenType::EQUAL)) {
    initializer = expression();
  }

  consume(Token::TokenType::SEMICOLON, "Expect ';' after variable declaration.");
  return std::make_shared<stmt::VariableDecl>(name, std::move(initializer));
}

std::vector<std::shared_ptr<stmt::StmtBase>> Parser::block()
{
  std::vector<std::shared_ptr<stmt::StmtBase>> statements;
  while(!check(Token::TokenType::RIGHT_BRACE) && !is_at_end()) {
    statements.push_back(declaration());
  }
  consume(Token::TokenType::RIGHT_BRACE, "Expect '}' after block.");
  return statements;
}

std::shared_ptr<stmt::If> Parser::if_statement()
{
  consume(Token::TokenType::LEFT_PAREN, "Expect '(' after 'if'.");
  auto condition = expression();
  consume(Token::TokenType::RIGHT_PAREN, "Expect ')' after condition.");

  auto then_stm = statement();
  std::shared_ptr<stmt::StmtBase> else_stm = nullptr;
  if(match(Token::TokenType::ELSE)) {
    else_stm = statement();
  }
  return std::make_shared<stmt::If>(std::move(condition), std::move(then_stm), std::move(else_stm));
}

std::shared_ptr<stmt::While> Parser::while_statement()
{
  consume(Token::TokenType::LEFT_PAREN, "Expect '(' after 'while'.");
  auto condition = expression();
  consume(Token::TokenType::RIGHT_PAREN, "Expect ')' after condition.");
  auto body = statement();
  return std::make_shared<stmt::While>(std::move(condition), std::move(body));
}

std::shared_ptr<stmt::StmtBase> Parser::for_statement()
{
  consume(Token::TokenType::LEFT_PAREN, "Expect '(' after 'for'.");
  std::shared_ptr<stmt::StmtBase> initializer;
  if(match(Token::TokenType::SEMICOLON)) {
    initializer = nullptr;
  }
  else if(match(Token::TokenType::VAR)) {
    initializer = var_declaration();
  }
  else {
    initializer = expr_statement();
  }
  // first ; parsed
  std::shared_ptr<expr::ExprBase> condition = nullptr;
  if(!check(Token::TokenType::SEMICOLON)) {
    condition = expression();
  }
  consume(Token::TokenType::SEMICOLON, "Expect ';' after loop condition.");

  std::shared_ptr<expr::ExprBase> increment = nullptr;
  if(!check(Token::TokenType::RIGHT_PAREN)) {
    increment = expression();
  }
  consume(Token::TokenType::RIGHT_PAREN, "Expect ')' after for clauses.");

  auto body = statement();

  // desugaring the for loop into a while loop

  if(increment) {
    auto statements = std::vector<std::shared_ptr<stmt::StmtBase>>();
    statements.push_back(std::move(body));
    statements.push_back(std::make_shared<stmt::Expression>(std::move(increment)));
    body = std::make_shared<stmt::Block>(std::move(statements));
  }
  if(!condition) {
    condition = std::make_shared<expr::Literal>(Token::Literal{true});
  }
  body = std::make_shared<stmt::While>(std::move(condition), std::move(body));

  if(initializer) {
    auto statements = std::vector<std::shared_ptr<stmt::StmtBase>>();
    statements.push_back(std::move(initializer));
    statements.push_back(std::move(body));
    body = std::make_shared<stmt::Block>(std::move(statements));
  }
  return body;
}

std::shared_ptr<stmt::Function> Parser::function(std::string kind)
{
  Token name = consume(Token::TokenType::IDENTIFIER, "Expect " + kind + " name.");
  consume(Token::TokenType::LEFT_PAREN, "Expect '(' after " + kind + " name.");
  std::vector<Token> params;
  if(!check(Token::TokenType::RIGHT_PAREN)) {
    do {
      if(params.size() >= 255) {
        error(peek(), "Can't have more than 255 parameters.");
      }
      params.push_back(consume(Token::TokenType::IDENTIFIER, "Expect parameter name."));
    } while(match(Token::TokenType::COMMA));
  }
  consume(Token::TokenType::RIGHT_PAREN, "Expect ')' after parameters.");
  consume(Token::TokenType::LEFT_BRACE, "Expect '{' before " + kind + " body.");
  auto body = block();
  return std::make_shared<stmt::Function>(name, params, std::move(body));
}

std::shared_ptr<stmt::Return> Parser::return_statement()
{
  auto keyword = previous();
  std::shared_ptr<expr::ExprBase> value = nullptr;
  if(!check(Token::TokenType::SEMICOLON)) {
    value = expression();
  }
  consume(Token::TokenType::SEMICOLON, "Expect ';' after return value.");
  return std::make_shared<stmt::Return>(keyword, std::move(value));
}

std::shared_ptr<stmt::Class> Parser::class_declaration()
{
  Token name = consume(Token::TokenType::IDENTIFIER, "Expect class name.");
  std::shared_ptr<expr::Variable> superclass{};

  if(match(Token::TokenType::LESS)) {
    consume(Token::TokenType::IDENTIFIER, "Expect superclass name.");
    superclass = std::make_shared<expr::Variable>(previous());
  }

  consume(Token::TokenType::LEFT_BRACE, "Expect '{' before class body.");

  std::vector<std::shared_ptr<stmt::Function>> methods;
  while(!check(Token::TokenType::RIGHT_BRACE) && !is_at_end()) {
    methods.push_back(function("method"));
  }

  consume(Token::TokenType::RIGHT_BRACE, "Expect '}' after class body.");
  return std::make_shared<stmt::Class>(name, std::move(superclass), std::move(methods));
}

void Parser::synchronize()
{
  while(!is_at_end()) {
    switch(peek().get_type()) {
    case Token::TokenType::CLASS:
    case Token::TokenType::FUN:
    case Token::TokenType::VAR:
    case Token::TokenType::FOR:
    case Token::TokenType::IF:
    case Token::TokenType::WHILE:
    case Token::TokenType::PRINT:
    case Token::TokenType::RETURN: return;
    }

    advance();
    if(previous().get_type() == Token::TokenType::SEMICOLON)
      return;
  }
}