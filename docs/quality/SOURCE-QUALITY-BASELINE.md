# Source Quality Baseline

## Current rule

The production executable uses an explicit source list in `CMakeLists.txt`.
A new production `.c` file must be added to `FC_SOURCES` in the same change
that introduces its implementation and tests.

Do not use recursive source globbing for the production target. Explicit
ownership makes build inputs reviewable and prevents empty or experimental
translation units from silently entering the binary.

## Placeholder modules

The following source files currently contain no implementation and are not
part of the production target:

- `src/core/event.c`
- `src/core/pipeline.c`
- `src/logging/logger.c`
- `src/resolver/resolver.c`
- `src/utils/string.c`

Their presence is not evidence that the corresponding features are implemented.
Keep them out of the executable until a real module contract, implementation,
and tests are delivered together. Existing headers that are also placeholders
must not be treated as supported public APIs.

## Warning policy

The project enables `-Wall -Wextra -Wpedantic -Wformat=2 -Wshadow` and
`-Wstrict-prototypes` for the production executable. Do not suppress a warning
globally just to obtain a clean log. Fix the underlying issue or document a
specific compiler/platform exception.

This change removes the five known empty-translation-unit warnings by excluding
those files from the build; it does not claim that the entire repository is
warning-free under every compiler.

## Validation

For every supported C language standard and build type, CI must configure,
compile, and run the complete CTest suite:

- C11 Debug and Release
- C17 Debug and Release
- C23 Debug and Release

The next quality pass should add compiler diversity and static analysis
(Clang diagnostics, clang-tidy, and cppcheck) after tool availability and
existing findings are assessed. Static-analysis findings must be triaged;
do not bulk-apply automated fixes to security-sensitive filesystem code.
