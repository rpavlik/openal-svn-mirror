MACRO(TEST_COMPILER_ATTRIBUTE)
CHECK_C_SOURCE_COMPILES(
"void  foo (int bar __attribute__((unused)) ) { }
static void baz (void) __attribute__((unused));
static void baz (void) { }
int main(){}
" HAVE_ATTRIBUTE)
IF(HAVE_ATTRIBUTE)
  ADD_DEFINE(HAVE___ATTRIBUTE__)
ENDIF(HAVE_ATTRIBUTE)
ENDMACRO(TEST_COMPILER_ATTRIBUTE)
