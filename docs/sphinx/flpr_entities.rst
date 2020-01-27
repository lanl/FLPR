.. _flpr_entities:

================================
Overview of FLPR Data Structures
================================

This attempts to give a high-level overview of the various FLPR data
structures and how they relate to one another.  This hierarchy of data
structures is built up from the low-level text representation up to
the parsed files.

One note on design philosophy.  FLPR is designed to manipulate source
code in "gentle" ways, allowing a program to make minor changes
without radically changing the appearance of the source code.  To do
this, it tracks a lot of physical layout data that is not present in
compilers.  However, it is also not intended to be a compiler
front-end, so it may not currently provide the analysis and inquiry
tools that you would expect in those.


-------------
Lexical Level
-------------

These are low-level entities related to the manipulation of Fortran
input text.  Considering that FLPR is designed to preserve or
manipulate source code, including formatting, these classes are more
complex than you may find in a normal compiler.  They must handle
things like lexemes being split across physical lines and multiple
statements compounded together on a line.

  Line_Buf (Logical_File::Line_Buf)
    A buffer of raw strings representing Fortran source code.

  File_Line
    A representation of the textual layout of a single raw source
    line.  This separates the line into "fields", which describe parts
    of the line that Fortran treats specially (e.g. labels,
    continuations, trailing comments, etc).  Line_Buf entries are
    converted to File_Line by ``File_Line::analyze_fixed`` or
    ``File_Line::analyze_free``, depending on the input style.  The
    File_Line is also used as a template for reformatting source
    lines.

    Examples of fields in File_Line include ``left_txt`` (control
    columns for fixed form), ``left_space`` (whitespace between
    ``left_txt`` and ``main_txt``) and ``main_txt`` (the body of the
    Fortran line).  File_Line also provides line-level categorization
    flags, including ``comment``, ``blank``, ``label``, etc.
    
  Token_Text
    The representation of a single "atom" of input.  Token_Text includes
    the lexeme (the actual text), its token value as determined by the
    lexer, and some textual layout information.

  Logical_Line
    A contiguous set of File_Lines that belong together, such as
    (continued) statement(s), or a comment block. This structure
    maintains the format of the original input, so that it can be
    re-emitted in a similar form.  NOTE: there may be more than one
    statement in a Logical_Line if they have been compounded with
    semicolons.

    The Logical_Line is a fundamental data structure that: groups
    File_Lines; presents a sequence of Token_Text representing the
    Fortran tokens; and organizes Token_Text into ranges for
    categorization.

  LL_TT_Range
    This is more of an intermediary than anything, but it is the base
    of other classes.  The "Logical Line Token Text Range" is a class
    that allows a range of Token_Text in a given Logical_Line to be
    handled as a stand-alone entity, storing references to both the
    TT_Range and to the source Logical_Line.

  LL_Stmt
    The "Logical Line Statement" is meant to represent one Fortran
    statement.  Derived from LL_TT_Range, it provides all the physical
    layout associated with a range of tokens in a Logical_Line, and
    acts as anchor for (optionally) holding a concrete syntax tree for
    the statement (see next section).  An LL_Stmt also holds any
    comment block found above the Fortran statement.  This is to allow
    a comment to stay anchored to a statement as text moves around.

  Logical_File
    A sequence of Logical_Lines and LL_Stmts that make up a file, plus
    other identifying information.  This is the level at which Logical_Lines
    are inserted, modified, deleted or split.

-------------
Grammar Level
-------------

The results of parsing sets of input lines are stored as trees,
generically defined in ``Tree.hh``.  Each node in the tree can store
data, and can have an arbitrary number of branches (children).

To make tree traversal a little more convenient, the idea of a
"cursor" is employed.  This is effectively an iterator that can move
in four directions: "up" (go up a level in the tree), "prev" (go to
previous node at this level sharing up), "next" (go to next node at
this level), and "down" (go down to the first branch off of this
node). Dereferencing a cursor with ``*`` or ``->`` returns a reference
to the user data stored at the current node.  
 
  Stmt::Stmt_Tree
    A Concrete Syntax Tree (CST) representing the structure of one
    complete Fortran statement.  These encode the productions of the
    statement grammar, and are produced by the rules in ``parse_stmt.cc``.

  Prgm::Prgm_Tree
    A tree of Stmt_Trees, organizing Fortran statements into larger
    constructs, parts, and procedures.  These are produced by rules
    in ``Prgm_Parsers_impl.hh``,

  Procedure
    This adapter class takes a Prgm_Tree cursor to a procedure
    (function-subprogram, subroutine-subprogram, main-program, or
    separate-module-subprogram) and creates (possibly-empty) LL_Stmt
    ranges across important regions of the procedure. This form is a
    lot easier to manipulate than the Prgm_Tree. For example, this can
    return an iterator to just the USE statements, avoiding a lot
    of condition testing in client code.  This is the easiest mechanism
    to use for changing statements in a particular section.

-----------------
Aggregate Objects
-----------------

These are the high-level entities that should be used by most client
applications to interrogate or manipulate Fortran source code.

  Parsed_File
    A lazy-evaluation container for a Logical_File and associated Prgm_Tree.
    This class also provides indentation functionality.
    
  Procedure_Visitor
    Given a Parsed_File, call a provided function on each procedure.


--------------
Missing Things
--------------

Unfortunately, concrete syntax trees (CSTs) are a real pain to work
with, as they represent the organization of tokens, rather than that of
language concepts.  This is why compilers generally skip this step and
go right to abstract syntax trees (ASTs).  However, for gently
manipulating the text in a line of source code, a CST is the way to
go.  FLPR needs to add AST support, or the functional equivalent
thereof, for easier interrogation (e.g. "what subroutine name is being
specified in this call-stmt?")

As of yet, there are also no symbol tables ("symtabs") in FLPR.  These are
being worked on, and also drive the need for AST functionality.

Finally (for now), it would also be useful to have some stmt-label
management, to easily identify unused labels or unreachable code.
This requires better parsing of things like write-stmt and
computed-goto-stmt.

    
