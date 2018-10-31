#include <stdio.h>

#include "absl/types/optional.h"

int main(int argc, char** argv) {
  absl::optional<int> x;
  x = argc;
  return *x;
}
