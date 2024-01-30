#include <iostream>

#include "application.h"
#include "config.h"

int main() {
  try {
    pefti::Application app;
    app.run();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }
  return 0;
}
