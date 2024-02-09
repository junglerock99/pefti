#include <exception>
#include <iostream>

#include "application.h"

// Creates and runs the application.
// All exceptions are handled here.
// Returns 0 on normal exit, or 1 on abnormal exit.
int main() {
  try {
    pefti::Application app;
    app.run();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }
  return 0;
}
