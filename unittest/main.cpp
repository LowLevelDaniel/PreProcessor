#include <PreY>

int main(int argc, char **argv) {
  for (int i = 1; i < argc; ++i) {
    std::string file_name = argv[i];
    FILE *file = fopen(argv[i],"r");
    if (file == NULL) {
      PRINT_ERROR("Error Opening File %s In Read Mode", argv[i]);
      return 1;
    }
    proccess_file(file,(file_name + ".ii").c_str(),{});
  }
}