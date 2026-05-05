#include <vector>
#include <string>
#include <variant>
#include <unordered_map>
#include <memory>
#include <fstream>
#include <iostream>
#include <cmath>

namespace json {

// Print the given error message and abort.
void die(const std::string& message) {
  std::cout << message << std::endl;
  exit(1);
}
  
struct JsonValue;

struct JsonNull {};
using JsonObject = std::unordered_map<std::string, std::unique_ptr<JsonValue>>;
using JsonArray = std::vector<std::unique_ptr<JsonValue>>;

struct JsonValue :
  std::variant<
    std::string,
    int,
    double,
    JsonObject,
    JsonArray,
    bool,
    JsonNull> {
  using variant::variant;

  const JsonValue& Get(const std::string& key) const {
    if (const auto* obj = std::get_if<JsonObject>(this)) {
      return *(obj->at(key));
    } else {
      die("JsonValue is not object, cannot do Get");
    }
  }

  const std::string& AsString() const {
    if (const auto* str = std::get_if<std::string>(this)) {
      return *str;
    } else {
      die("JsonValue is not string, cannot do AsString");
    }
  }

  const int& AsInt() const {
    if (const auto* number = std::get_if<int>(this)) {
      return *number;
    } else {
      die("JsonValue is not int, cannot do AsInt");
    }
  }

  const JsonValue& Index(int index) const {
    if (const auto* obj = std::get_if<JsonArray>(this)) {
      return *(obj->at(index));
    } else {
      die("JsonValue is not object, cannot do Index");
    }
  }

  struct Iterator {
    using iterator_category = std::forward_iterator_tag;
    using value_type        = JsonValue;
    using difference_type   = std::ptrdiff_t;
    using pointer           = const JsonValue*;
    using reference         = const JsonValue&;
    
    explicit Iterator(const JsonArray& arr, int index)
      : arr_(arr), index_(index) {}
    
    // Dereference
    reference operator*() const { return *arr_[index_]; }
    pointer operator->() { return arr_[index_].get(); }
    
    // Prefix increment
    Iterator& operator++() { index_++; return *this; }  
    
    // Postfix increment
    Iterator operator++(int) { Iterator tmp = *this; ++(*this); return tmp; }
    
    // Comparison
    friend bool operator== (const Iterator& a, const Iterator& b) {
      return (&a.arr_ == &b.arr_) && a.index_ == b.index_;
    }
    friend bool operator!= (const Iterator& a, const Iterator& b) {
      return !(a == b);
    }
    
  private:
    const JsonArray& arr_;
    int index_ = 0;
  };
  
  // Return iterator to the first element
  Iterator begin() const {
    if (const auto* arr = std::get_if<JsonArray>(this)) {
      return Iterator(*arr, 0);
    } else {
      die("JsonValue is not an array, cannot do begin");
    }
  }
  
  // Return iterator to one-past-the-last element
  Iterator end() const {
    if (const auto* arr = std::get_if<JsonArray>(this)) {
      return Iterator(*arr, arr->size());
    } else {
      die("JsonValue is not an array, cannot do begin");
    }
  }
};

namespace internal {

struct OpenCurlyBrace {};
struct CloseCurlyBrace {};
struct OpenSquareBrace {};
struct CloseSquareBrace {};
struct Comma {};
struct Colon {};
struct Null {};
using Token = std::variant<
  OpenCurlyBrace,
  CloseCurlyBrace,
  OpenSquareBrace,
  CloseSquareBrace,
  Comma,
  Colon,
  Null,
  std::string,
  int,
  double,
  bool>;

void DebugPrint(const Token& token) {
  if (std::get_if<OpenCurlyBrace>(&token)) {
    std::cout << "{";
  } else if (std::get_if<CloseCurlyBrace>(&token)) {
    std::cout << "}";
  } else if (std::get_if<OpenSquareBrace>(&token)) {
    std::cout << "[";
  } else if (std::get_if<CloseSquareBrace>(&token)) {
    std::cout << "]";
  } else if (std::get_if<CloseSquareBrace>(&token)) {
    std::cout << "]";
  } else if (std::get_if<Comma>(&token)) {
    std::cout << ",";
  } else if (std::get_if<Colon>(&token)) {
    std::cout << ":";
  } else if (std::get_if<Null>(&token)) {
    std::cout << "null";
  } else if (const auto* x = std::get_if<std::string>(&token)) {
    std::cout << "String(" << *x << ")";
  } else if (const auto* x = std::get_if<int>(&token)) {
    std::cout << "Int(" << *x << ")";
  } else if (const auto* x = std::get_if<double>(&token)) {
    std::cout << "Double(" << *x << ")";
  } else if (const auto* x = std::get_if<bool>(&token)) {
    std::cout << "Boolean(" << *x << ")";
  }
}

void DebugPrintKV(const std::string& key, const JsonValue& value, int indent = 0) {
}
  
void DebugPrint(const JsonValue& json, int indent = 0, bool skip_first_indent = false) {
  if (!skip_first_indent) {
    for (int i = 0; i < indent; ++i) {
      std::cout << "  ";
    }
  }
  if (const auto* x = std::get_if<std::string>(&json)) {
    std::cout << "String(" << *x << ")";
  } else if (const auto* x = std::get_if<int>(&json)) {
    std::cout << "Int(" << *x << ")";
  } else if (const auto* x = std::get_if<double>(&json)) {
    std::cout << "Double(" << *x << ")";
  } else if (const auto* x = std::get_if<bool>(&json)) {
    std::cout << "Boolean(" << *x << ")";
  } else if (std::get_if<JsonNull>(&json)) {
    std::cout << "null";
  } else if (const auto* x = std::get_if<JsonObject>(&json)) {
    std::cout << "Object{" << std::endl;
    for (const auto& [k, v] : *x) {
      for (int i = 0; i < indent + 1; ++i) {
	std::cout << "  ";
      }
      std::cout << "String(" << k << ") : ";
      DebugPrint(*v, indent + 1, /*skip_first_indent=*/true);
      std::cout << ", " << std::endl;
    }
    for (int i = 0; i < indent; ++i) {
      std::cout << "  ";
    }
    std::cout << "}";
  } else if (const auto* x = std::get_if<JsonArray>(&json)) {
    std::cout << "Array[" << std::endl;
    for (const auto& v : *x) {
      DebugPrint(*v, indent + 1);
      std::cout << ", " << std::endl;
    }
    for (int i = 0; i < indent; ++i) {
      std::cout << "  ";
    }
    std::cout << "]";
  }
}
  
bool IsWhitespace(char c) {
  return c == ' ' || c == '\n' || c == '\r' || '\t';
}

bool IsDigit(char c) {
  return c >= '0' && c <= '9';
}

class CharacterStream {
public:
  CharacterStream(const std::string& str)
    : str_(str), current_(0) {}

  bool IsEmpty() const { return current_ >= str_.size(); }

  char Peek() const {
    if (IsEmpty()) {
      die("Tried to peek at an empty stream.");
    }
    return str_[current_];
  }

  char Next() {
    if (IsEmpty()) {
      die("Tried to advance an empty stream.");
    }
    current_++;
    return str_[current_ - 1];
  }

  void Expect(char expected) {
    char c = Next();
    if (c != expected) {
      die("Got unexpected character");
    }
  }

  void ExpectString(const std::string& str) {
    for (char c : str) {
      Expect(c);
    }
  }

  int ExpectHexDigit() {
    char c = Next();
    if (c >= '0' && c <= '9') {
      return c - '0';
    }
    if (c >= 'A' && c <= 'F') {
      return c - 'A' + 10;
    }
    if (c >= 'a' && c <= 'f') {
      return c - 'a' + 10;
    }
    die("Invalid hex digit");
  }

  int ExpectDigit() {
    char c = Next();
    if (IsDigit(c)) {
      return c - '0';
    }
    die("Invalid digit");
  }
  
  bool TryConsumeWhitespace() {
    char c = Peek();
    if (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
      Next();
      return true;
    }
    return false;
  }

  bool TryConsume(char c) {
    if (c == Peek()) {
      Next();
      return true;
    }
    return false;
  }

private:
  std::string str_;
  int current_ = 0;
};

Token ReadTokenNull(CharacterStream& stream) {
  stream.ExpectString("null");
  return Null();
}

char ReadEscapeCharacter(CharacterStream& stream) {
  stream.Expect('\\');
  if (stream.TryConsume('"')) {
    return '"';
  } else if (stream.TryConsume('\\')) {
    return '\\';
  } else if (stream.TryConsume('/')) {
    return '/';
  } else if (stream.TryConsume('b')) {
    return '\b';
  } else if (stream.TryConsume('f')) {
    return '\f';
  } else if (stream.TryConsume('n')) {
    return '\n';
  } else if (stream.TryConsume('r')) {
    return '\r';
  } else if (stream.TryConsume('t')) {
    return '\t';
  } else if (stream.TryConsume('u')) {
    stream.ExpectHexDigit();
    stream.ExpectHexDigit();
    stream.ExpectHexDigit();
    stream.ExpectHexDigit();
    // No real unicode support lol.
    return '?';
  } else {
    die("Invalid escape sequence");
  }
}    
  
Token ReadTokenString(CharacterStream& stream) {
  std::string str;
  stream.Expect('"');
  while (stream.Peek() != '"') {
    if (stream.Peek() == '\\') {
      str.push_back(ReadEscapeCharacter(stream));
    } else {
      str.push_back(stream.Next());
    }
  }
  stream.Expect('"');
  return str;
}

Token ReadTokenTrue(CharacterStream& stream) {
  stream.ExpectString("true");
  return true;
}

Token ReadTokenFalse(CharacterStream& stream) {
  stream.ExpectString("false");
  return false;
}

int ReadInteger(CharacterStream& stream) {
  int value = stream.ExpectDigit();
  while (IsDigit(stream.Peek())) {
    value *= 10;
    value += stream.ExpectDigit();
  }
  return value;
}

double ReadFractionalPart(CharacterStream& stream) {
  int fractional_part = 0;
  int num_fractional_digits = 0;
  while (IsDigit(stream.Peek())) {
    fractional_part *= 10;
    fractional_part += stream.ExpectDigit();
    ++num_fractional_digits;
  }
  return (fractional_part * std::pow(10, -num_fractional_digits));
}
  
// A bit more permissive than actual JSON, since we allow leading zeros,
// decimals that don't start with a zero, etc.
Token ReadTokenNumber(CharacterStream& stream) {
  const bool negative = stream.TryConsume('-');
  
  const int whole_part = ReadInteger(stream);

  const bool has_fraction = stream.TryConsume('.');
  const double fractional_part =
    has_fraction ? ReadFractionalPart(stream) : 0.0;

  const bool has_exponent = stream.TryConsume('e') || stream.TryConsume('E');
  const bool exponent_is_negative = has_exponent && stream.TryConsume('-');
  if (has_exponent && !exponent_is_negative) {
    stream.TryConsume('+');
  }
  const int exponent_part =
    has_exponent ? ReadInteger(stream) : 0;

  if (has_fraction || has_exponent) {
    // Need to return a double.
    double sign = negative ? -1.0 : 1.0;
    double base_number = (1.0 * whole_part) + fractional_part;
    double exponent = std::pow(10, (exponent_is_negative ? -exponent_part : exponent_part));
    return sign * base_number * exponent;
  } else {
    // Need to return an int.
    return negative ? -whole_part : whole_part;
  }
}

std::vector<Token> Tokenize(CharacterStream& stream) {
  std::vector<Token> tokens;
  while (!stream.IsEmpty()) {
    if (stream.TryConsumeWhitespace()) {
      continue;
    } else if (stream.TryConsume('{')) {
      tokens.push_back(OpenCurlyBrace());
    } else if (stream.TryConsume('}')) {
      tokens.push_back(CloseCurlyBrace());
    } else if (stream.TryConsume('[')) {
      tokens.push_back(OpenSquareBrace());
    } else if (stream.TryConsume(']')) {
      tokens.push_back(CloseSquareBrace());
    } else if (stream.TryConsume(']')) {
      tokens.push_back(CloseSquareBrace());
    } else if (stream.TryConsume(',')) {
      tokens.push_back(Comma());
    } else if (stream.TryConsume(':')) {
      tokens.push_back(Colon());
    } else if (stream.Peek() == 'n') {
      tokens.push_back(ReadTokenNull(stream));
    } else if (stream.Peek() == '"') {
      tokens.push_back(ReadTokenString(stream));
    } else if (stream.Peek() == 't') {
      tokens.push_back(ReadTokenTrue(stream));
    } else if (stream.Peek() == 'f') {
      tokens.push_back(ReadTokenFalse(stream));
    } else {
      tokens.push_back(ReadTokenNumber(stream));
    }
  }
  return tokens;
}

class TokenStream {
public:
  TokenStream(const std::vector<Token>& tokens)
    : tokens_(tokens), current_(0) {}

  bool IsEmpty() const { return current_ >= tokens_.size(); }

  const Token& Peek() const {
    if (IsEmpty()) {
      die("Tried to peek at an empty stream.");
    }
    return tokens_[current_];
  }

  template <typename T>
  const T* PeekAsType() const {
    const Token& token = Peek();
    return std::get_if<T>(&token);
  }

  const Token& Next() {
    if (IsEmpty()) {
      die("Tried to advance an empty stream.");
    }
    current_++;
    return tokens_[current_ - 1];
  }

  template <typename T>
  const T& Expect() {
    const Token& token = Next();
    const T* value = std::get_if<T>(&token);
    if (value == nullptr) {
      die("Got unexpected token");
    }
    return *value;
  }

  template <typename T>
  bool TryConsume() {
    const Token& token = Peek();
    if (const T* value = std::get_if<T>(&token)) {
      Next();
      return true;
    } else {
      return false;
    }
  }

private:
  std::vector<Token> tokens_;
  int current_ = 0;
};

// Forward declaration since functions are mutually recursive.
std::unique_ptr<JsonValue> ParseJson(TokenStream& stream);
  
std::unique_ptr<JsonValue> ParseJsonObject(TokenStream& stream) {
  JsonObject object;
  
  stream.Expect<OpenCurlyBrace>();
  while (stream.PeekAsType<CloseCurlyBrace>() == nullptr) {
    if (!object.empty()) {
      stream.Expect<Comma>();
    }
    std::string key = stream.Expect<std::string>();
    stream.Expect<Colon>();
    std::unique_ptr<JsonValue> value = ParseJson(stream);
    object[std::move(key)] = std::move(value);
  }
  stream.Expect<CloseCurlyBrace>();

  return std::make_unique<JsonValue>(std::move(object));
}

std::unique_ptr<JsonValue> ParseJsonArray(TokenStream& stream) {
  JsonArray array;
  
  stream.Expect<OpenSquareBrace>();
  while (stream.PeekAsType<CloseSquareBrace>() == nullptr) {
    if (!array.empty()) {
      stream.Expect<Comma>();
    }
    std::unique_ptr<JsonValue> value = ParseJson(stream);
    array.push_back(std::move(value));
  }
  stream.Expect<CloseSquareBrace>();

  return std::make_unique<JsonValue>(std::move(array));
}
  
std::unique_ptr<JsonValue> ParseJson(TokenStream& stream) {
  if (stream.PeekAsType<OpenCurlyBrace>() != nullptr) {
    return ParseJsonObject(stream);
  } else if (stream.PeekAsType<OpenSquareBrace>() != nullptr) {
    return ParseJsonArray(stream);
  } else if (stream.TryConsume<Null>()) {
    return std::make_unique<JsonValue>(JsonNull());
  } else if (stream.PeekAsType<std::string>() != nullptr) {
    return std::make_unique<JsonValue>(stream.Expect<std::string>());
  } else if (stream.PeekAsType<int>() != nullptr) {
    return std::make_unique<JsonValue>(stream.Expect<int>());
  } else if (stream.PeekAsType<double>() != nullptr) {
    return std::make_unique<JsonValue>(stream.Expect<double>());
  } else if (stream.PeekAsType<bool>() != nullptr) {
    return std::make_unique<JsonValue>(stream.Expect<bool>());
  }
  return nullptr;
}

}; // namespace internal
  
std::unique_ptr<JsonValue> ParseJsonTopLevel(const std::string& str) {
  internal::CharacterStream char_stream(str);
  const std::vector<internal::Token> tokens = internal::Tokenize(char_stream);
  internal::TokenStream tok_stream(tokens);
  std::unique_ptr<JsonValue> value = internal::ParseJson(tok_stream);
  if (!tok_stream.IsEmpty()) {
    die("Extraneous token at end of json string");
  }
  return value;
}

std::string ReadMultiLineString() {
  int consecutive_empty_count = 0;
  std::string str;

  for (;;) {
    std::string line;
    std::getline(std::cin, line);
    str += line;
    str += "\n";

    if (line.empty()) {
      ++consecutive_empty_count;
    } else {
      consecutive_empty_count = 0;
    }

    if (consecutive_empty_count >= 2) {
      break;
    }
  }

  return str;
}
  
}; // namespace json
