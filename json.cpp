#include "json.h"
#include <iostream>

namespace json::internal {

void RunJsonParser() {
  std::cout << "Enter JSON. Double empty line to end" << std::endl;

  const std::string str = ReadMultiLineString();
  
  // CharacterStream char_stream(str);
  // const std::vector<Token> tokens = Tokenize(char_stream);

  // for (const auto& token : tokens) {
  //   std::cout << "Token: ";
  //   DebugPrint(token);
  //   std::cout << std::endl;
  // }

  std::unique_ptr<JsonValue> json = ParseJsonTopLevel(str);

  DebugPrint(*json);
  std::cout << std::endl << std::endl;
}
  
};

int main(int argc, char* argv[]) {
  json::internal::RunJsonParser();
  return 0;
}

