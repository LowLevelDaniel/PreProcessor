#define MACRO_FUNCTION(a,b,c) dosomething(abc)
#define ML_MACRO_FUNCTION(a,b,c) \
  do\
something(a\
bc)

int main() {
  MACRO_FUNCTION(10,1,4);

  foo(x);

  ML_MACRO_FUNCTION(9,3,6);
  return 0;
}