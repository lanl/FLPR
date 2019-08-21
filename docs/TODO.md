# The FLPR To-Do List

## File_Line processing

Items related CPP (C-Preprocessor) and raw line formatting
- Match comment lines to *preceding* Fortran lines if the comment is
  approximately aligned with a right-comment on the Fortran stmt.
- Only convert from fixed-format to free-format on request, and make
  it distinct from parsing.
- In fixed -> free format conversion, handle comments a bit better:
  + If the comment text begins in C2 (column 2), put the comment in
    `left_text`.
  + If the comment text begins in C7, but there is no fortran
    statement in C7 below (e.g. indented), put the comment in
    `left_text`.
  + If the comment text is aligned with the statement `main_txt`, put
    the comment in `main_txt`.
  + If the comment text is in `right_text`, leave it there.
- Check that statement continuations are being handled correctly.
  + In free format, a leading ampersand on a continuation line implies
    that the text of that line gets directly appended to the text of
    the previous line before the trailing ampersand.  In particular,
    *any* lexical token can be split this way.
  + In fixed format, characters 7-72 are directly appended to
    character 72 of the previous line.  This rule is particularly
    important in a continued character context. 
  
## Parsing

Implement the missing parsers:
* *block-data*
* *change-team-construct*
* *critical-construct*
* *submodule*

## Efficiency

Items related to performance and memory use.
- Speed up the parser combinator approach.
- Look at clearing `main_txt` from `File_Line` once it has been put
  under the control of a `Logical_Line` (text is already found in the
  `Token_Text`)
- Could "collapse" linear subtrees in the `Stmt_Tree`.
- Consider reading/processing/writing one program-unit at a time,
  rather than reading in the entire file.
- Add in switch statements for things like action-stmt to jump to appropriate
  parsers, rather than sequentially moving through them.


## Build System, Directory Structure, Testing 

Items related to CMake configuration, the layout of directories and
files, and the test system
(preprocessor and library).
- Integrate the documentation systems
  + Add breathe and exhale support to Sphinx
  + Update doxygen output to work with above
  + Make it all work with CMake
- Simplify the FLPRApp install.
- Add more tests.  Always add more tests.

