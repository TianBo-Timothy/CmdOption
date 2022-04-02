# CmdOption
A simplified command line parser which intialize itself by parsing the usage text.

Unlike other comand line parsers, where the command options have to be added to the parser before parsing, the construction of this command line parser is done by parsing the usage text, which is supposed to be provided anyway.

Using the usage text as the input of the parser help the author to keep the usage text and the actual usage consistent as they are from the same source.

The following shows an example to demonstrate how simple it is to use the CmdOption to construct the parser and pasre the command line.

## A simple example

This example is a simple divider program. The task of the program is simply divide two integers. For demostration purpose, a few options are added.

```c++
// FILE: divider.cpp

#include "CmdOption.h"
int main(int argc, char **argv) {
  tianbo::CmdOption command_opt;
  // the command line parser is initialized with the usage text
  command_opt << R"(
This example divider program demonstrates how to use the CmdOption class to parse the command
line. This usage message is not only used as a help message, but also as the input for the
CmdOption object to initialize itself.

Usage: divide [options] a b

Options:
-w --warning
    Warn if the divisor is zero
    This option has no parameter, i.e. it is a switch

-p --precision=NUM
    Controls the precision of the output
    This option does not have a short option for it and an argument could be assigned to it.
    Note that "NUM" is informative only to the reader of the message, not to the parser.

-f FILE
    Save the result into a file.
    There is no long option for the option. This option also needs an argument. In this form,
    explanation must be in a separate line

-h --help  Show this help message.
    Description text can be at the same line of the options

Examples:

The following command divide 5 by 3
divide 5 3

The command will print 1.66666666666667. To get a short answer, a precision option can be added.
For example:
divide --precision=3 5 3

Get a warning if the divisor is zero
divide -w 5 0

... Add more description as needed.
)";
```

  // The following is only for debugging. In case the CmdOption class cannot
  // parse the usage, use this function to print the error message to see which
  // line went wrong. This line should be removed once debugging is done.
  command_opt.reportError();
  command_opt.debugReport();

  // parse the options and arguments
  command_opt.parse(argc, argv);

  // The [] operator is used to access the switch. The following shows how to
  // check if -h option is given. Alternatively, this switch can be checked with
  // command_opt["help"]
  if (command_opt["h"]) {
    command_opt.usage();
    return 0;
  }

  // Internally, the [] operator returns a StringValue type, which can be
  // interpreted as a boolean to tell if it has been set.
  bool show_warning = (bool)command_opt["w"];

  // "precision" option has no short form. It has to be accessed with the long
  // option. The StringValue returned by [] operator can be converted to an
  // integer. It is equivalent to int precision = 15; if
  // (command_opt["precision"])
  //   precision = command_opt["precision"];
  int precision = command_opt["precision"].valueOr(15);

  // The command line arguments are accessible through arguments()
  if (command_opt.arguments().count() != 2) {
    std::cerr << "No operands provided" << std::endl;
    return 1;
  }

  // The arguments can be converted into a vector of integers or other data
  // types
  std::vector<int> operands = command_opt.arguments();
  if (show_warning && operands.back() == 0) {
    std::cerr << "The divisor is zero!" << std::endl;
    return 2;
  }

  double result = static_cast<double>(operands.front()) /
                  static_cast<double>(operands.back());

  std::cout << "the result: " << std::setprecision(precision) << result
            << std::endl;

  if (command_opt["f"]) {
    std::ofstream(command_opt["f"].str())
        << "the result: " << std::setprecision(precision) << result
        << std::endl;
  }
  return 0;
}
