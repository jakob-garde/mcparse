# mcparse

Custom parser for the mcstas McStas DSL: A modern, low-dependency, instrument- and component 
parser and code generator.

This project is part of an exploratory re-imagining of the mcstas/mcxtrace simulation
core, and is a stand-alone demonstration of parsing and code-generation.

- Example bulk output: https://github.com/climbcat/mcparse/blob/main/example_output_0.1.0.md
- Example error output: https://github.com/climbcat/mcparse/blob/main/failtest_output_0.1.0.md
- Tool building example: https://github.com/climbcat/mctrace

For the official McCode simulation project for neutron- and x-ray scattering instrumentation,
see [mccode.org](mccode.org).

## Features

Improved McStas DSL parser:

- Prints precise error messages and pin-points the line error, allowing users to quickly grasp the nature of a mistake
- Much reduced library dependencies with no linking required; compiles in a second on older systems
- Eliminates any parser-generator build steps
- No abstract grammar-rule configuration (using e.g. lex/yacc), just the parser code
- Edit build and debug the light-weight parser in seconds
- Extensive test set and convenient cli parameters for dry-runs and code generation

This repository includes a set of ~ 350 components and ~ 150 instruments from McStas 3.
These files are parsed very quickly (see timing outputs at the end of the bulk example).

It parses all of the DSL except for a few, rarely-used legacy features and a few
unused grammars rules. Minor changes were applied to the otherwise standard test set
of component and instrument files, in order to slightly streamline the DSL definition.

Notable omisions are the JUMP keyword and unused grammar rules such as the DEFINITION PARAMETERS.
Function declarations and function pointers in DECLARE sections, are parsed,
but not syntax checked, nor captured for code generation, since these were not
required for the present purposes.

The parser strengthens parsing security in some respects, for example, declarations 
in DECLARE sections are treated as actual C struct members, or an error is produced.
It also checks argument default values for validity, a mistake that would otherwise
be caught only at runtime.

However, not all of the DSL features are implemented in the code generator.

## build & run

Download this repository, and execute the following commands: 

<pre>
./lib/getdeps.sh
./build.sh
./mcparse --help
</pre>

Run the tool by build doing, e.g.: 

<pre>
./mcparse mcstas-comps/
</pre>

Currently targetting Linux only.


## Code Generation

In addition to parsing, this project includes a code generator which outputs component
functions and type wrappers. The output is C++ compliant - assuming C++ compliant components
are used as input. A number of components were ported to C++ for this purpose, and the
porting process was straightforward, even for arguably complex components such as Monitor_nD.

The current generator outputs one .h file (with implementations included)  for each component,
plus one meta file for all included components in the parse set. It outputs an instrument
configuration .h file for each instrument included in the parse set.

The generated .h files depend only on the simulation core, and the baselayer library
mentioned above, and can thus be easilly ported to other projects, be that for optimization,
testing, porting or component- or tool development purposes.

In combination With the <code>simlib.h</code> and <code>simcore.h</code> source files
(found in the <code>mctrace</code> project linked above), which include the mcstas runtime
simulation code and helper libraries, the code-generated .h files enables an accessible
debugging- and development environment for McStas simualtions, which was previously unheard-of.

### Notes on the C++ port/compatibility

Fourteen (14) components were ported, initially: Those comprising our example
instrument, the infamouse PSI_DMC. This instrument was also code generated, and runs live
simulation, plot and display, in the mctrace project (see above).

The original C code turned out to be very compatible with a standard C++ compiler (g++).
Some explicit porting was necessary, but  straight-forward and not
difficult.

All support for OpenMPI and PGCC/OpenAcc were excluded from the port, since these features
are not essential for the purposes of this project.

First, they complicate the process, whoose first goal was a proof-of-concept. Second,
as a re-imagining, such tools would perhaps be re-implemented differently. For example, 
multi-threading would be implemented natively.

### Building high-level functionality

The generated components meta file provides access to all of the parsed components 
through a unified API: Thus it becomes possible to build high-level operations, 
e.g. simulation strategies or user interface, directly on top of the generated 
code.

For those who are familiar, this includes mcdisplay-like functionality and visual,
live simulation execution and monitoring.

Configuration code for instruments is also generated. This sets up the component geometry,
custom instrument initialization and EXTEND code, and initializes everything into a
simulation-ready state.

This is of course exactly what the mcstas-generated code does, but this format is more
accessible. As a comparison, the mcstas-generator easily outputs 10000
lines of hard-to-read code for the PSI_DMC. Much of this is either redundant library code
or define-switched versions targetting a variety of platforms. As such, mcstas output is
not intended for inclusion in other projects, while the mcparse output was designed for
exactly this purpose.

### What about McXrace?

Porting things to McXtrace is straight-forward, and follows exactly the steps that
were demonstrated for the McStas code.

These steps were not taken for the same reason that just one instrument was
ported; the project's purpose as a proof-of-concept exploration.
