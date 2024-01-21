#include <iostream>

#include "application.h"
#include "config.h"

int main() {
  try {
    pefti::Application app;
    app.run();
  } catch (const std::string& e) {
    std::cerr << e << '\n';
  } catch (const char* e) {
    std::cerr << e << '\n';
  }
  return 0;
}
