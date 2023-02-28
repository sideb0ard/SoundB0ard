#include <utils.h>

#include <interpreter/lexer.hpp>
#include <iostream>
#include <string>

namespace lexer {

Lexer::Lexer(std::string input) : input_{input} { ReadChar(); }

bool Lexer::ReadInput(std::string input) {
  input_ += input;

  if (IsBalanced(input_)) {
    ReadChar();
    return true;
  }
  return false;
}

void Lexer::Reset() {
  input_.clear();
  current_char_ = 0;
  current_position_ = 0;
  next_position_ = 0;
}

std::string Lexer::ReadString() {
  auto pos = current_position_ + 1;
  while (true) {
    ReadChar();
    if (current_char_ == '"' || current_char_ == 0) break;
  }
  return input_.substr(pos, current_position_ - pos);
}

void Lexer::ReadChar() {
  const int len = input_.length();
  if (next_position_ >= len)
    current_char_ = 0;
  else
    current_char_ = input_[next_position_];

  current_position_ = next_position_;
  next_position_ += 1;
}

char Lexer::PeekChar() {
  if (next_position_ >= static_cast<int>(input_.length()))
    return 0;
  else
    return input_[next_position_];
}

token::Token Lexer::NextToken() {
  token::Token tok;

  SkipWhiteSpace();

  switch (current_char_) {
    case ('='):
      if (PeekChar() == '=') {
        ReadChar();
        tok.type_ = token::SLANG_EQ;
        tok.literal_ = "==";
      } else {
        tok.type_ = token::SLANG_ASSIGN;
        tok.literal_ = current_char_;
      }
      break;
    case ('+'):
      if (PeekChar() == '+') {
        ReadChar();
        tok.type_ = token::SLANG_INCREMENT;
        tok.literal_ = "++";
      } else {
        tok.type_ = token::SLANG_PLUS;
        tok.literal_ = current_char_;
      }
      break;
    case ('-'):
      if (PeekChar() == '-') {
        ReadChar();
        tok.type_ = token::SLANG_DECREMENT;
        tok.literal_ = "--";
      } else {
        tok.type_ = token::SLANG_MINUS;
        tok.literal_ = current_char_;
      }
      break;
    case ('!'):
      if (PeekChar() == '=') {
        ReadChar();
        tok.type_ = token::SLANG_NOT_EQ;
        tok.literal_ = "!=";
      } else {
        tok.type_ = token::SLANG_BANG;
        tok.literal_ = current_char_;
      }
      break;
    case ('$'):
      tok.type_ = token::SLANG_DOLLAR;
      tok.literal_ = current_char_;
      break;
    case ('~'):
      tok.type_ = token::SLANG_NOT;
      tok.literal_ = current_char_;
      break;
    case ('#'):
      tok.type_ = token::SLANG_HASH;
      tok.literal_ = current_char_;
      break;
    case ('*'):
      tok.type_ = token::SLANG_ASTERISK;
      tok.literal_ = current_char_;
      break;
    case ('/'):
      tok.type_ = token::SLANG_SLASH;
      tok.literal_ = current_char_;
      break;
    case ('%'):
      tok.type_ = token::SLANG_MODULO;
      tok.literal_ = current_char_;
      break;
    case ('<'):
      tok.type_ = token::SLANG_LT;
      tok.literal_ = current_char_;
      break;
    case ('>'):
      tok.type_ = token::SLANG_GT;
      tok.literal_ = current_char_;
      break;
    case (','):
      tok.type_ = token::SLANG_COMMA;
      tok.literal_ = current_char_;
      break;
    case (';'):
      tok.type_ = token::SLANG_SEMICOLON;
      tok.literal_ = current_char_;
      break;
    case ('('):
      tok.type_ = token::SLANG_LPAREN;
      tok.literal_ = current_char_;
      break;
    case (')'):
      tok.type_ = token::SLANG_RPAREN;
      tok.literal_ = current_char_;
      break;
    case ('{'):
      tok.type_ = token::SLANG_LBRACE;
      tok.literal_ = current_char_;
      break;
    case ('}'):
      tok.type_ = token::SLANG_RBRACE;
      tok.literal_ = current_char_;
      break;
    case ('['):
      tok.type_ = token::SLANG_LBRACKET;
      tok.literal_ = current_char_;
      break;
    case (']'):
      tok.type_ = token::SLANG_RBRACKET;
      tok.literal_ = current_char_;
      break;
    case (':'):
      tok.type_ = token::SLANG_COLON;
      tok.literal_ = current_char_;
      break;
    case ('|'):
      if (PeekChar() == '|') {
        ReadChar();
        tok.type_ = token::SLANG_OR;
        tok.literal_ = "||";
      } else {
        tok.type_ = token::SLANG_PIPE;
        tok.literal_ = current_char_;
      }
      break;
    case ('&'):
      if (PeekChar() == '&') {
        ReadChar();
        tok.type_ = token::SLANG_AND;
        tok.literal_ = "&&";
      }
      break;
    case ('"'):
      tok.type_ = token::SLANG_STRING;
      tok.literal_ = ReadString();
      break;
    case (0):
      tok.type_ = token::SLANG_EOFF;
      break;
    default:
      if (current_char_ == 'p' && IsDigit(PeekChar())) {
        tok.literal_ = ReadProcId();
        tok.type_ = token::SLANG_PROC_ID;
        return tok;
      }
      if (IsValidIdentifier(current_char_) && !IsDigit(current_char_)) {
        tok.literal_ = ReadIdentifier();
        tok.type_ = token::LookupIdent(tok.literal_);
        return tok;
      } else if (IsDigit(current_char_)) {
        tok.type_ = token::SLANG_NUMBER;
        tok.literal_ = ReadNumber();
        return tok;
      } else {
        tok.type_ = token::SLANG_ILLEGAL;
        tok.literal_ = current_char_;
      }
  }

  ReadChar();
  return tok;
}

std::string Lexer::ReadIdentifier() {
  int position = current_position_;
  while (IsValidIdentifier(current_char_)) ReadChar();
  return input_.substr(position, current_position_ - position);
}

std::string Lexer::ReadNumber() {
  int position = current_position_;
  // current_char_ == '-')
  while (IsDigit(current_char_) || current_char_ == '.') ReadChar();
  return input_.substr(position, current_position_ - position);
}

std::string Lexer::ReadProcId() {
  // discard current 'p'
  ReadChar();

  int position = current_position_;
  while (IsDigit(current_char_)) ReadChar();
  return input_.substr(position, current_position_ - position);
}

void Lexer::SkipWhiteSpace() {
  while (current_char_ == ' ' || current_char_ == '\t' ||
         current_char_ == '\n' || current_char_ == '\r')
    ReadChar();
}

std::string Lexer::GetInput() { return input_; }

}  // namespace lexer
