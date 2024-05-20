#ifndef CPPCMD_TYPEDEFS_H
#define CPPCMD_TYPEDEFS_H

#include "arguments.h"
#include "options.h"
#include "parser/option/gnu_style_parser.h"

namespace cppcmd {

    using options = basic_options<parser::gnu_style_parser>;
    using arguments = basic_arguments<parser::gnu_style_parser>;

}

#endif //CPPCMD_TYPEDEFS_H
