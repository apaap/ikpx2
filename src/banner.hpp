#pragma once

#include <iostream>

void print_banner() {

    std::cerr << "\033[31;1m\n           :'',\n          ,: '' '' ,,\n         :     : ,, ' ,\n        :    ,:   : : :\n"
                "         : , : , ', ,\n              ,,' ' ':  ,,\n                  ,  :'''',\n                 ,: :,'' ,,\n"
                "                    :, :',\n                  ,, :  '' :\n                   '''','': ,\n                          ',:\n"
                "                   '''     '  :\n                   ,    :'  , '\n                   ''', ',:,\n"
                "          \x1b[0m\x1b[34;1m*---*   *---*  \x1b[0m\x1b[31;1m'\x1b[0m\x1b[34;1m*---------*   *---*   *---*\n"
                "         /   /|  /   /\x1b[0m\x1b[31;1m'   \x1b[0m\x1b[34;1m|\\         \\   \\   \\ /   /\n"
                "        /   / * /   *---*\x1b[0m\x1b[31;1m:\x1b[0m\x1b[34;1m* \\   v2.0  \\   \\   v   /\n"
                "       /   / / /         \\ \\ \\         *   *     *\n"
                "      /   / / /       *---* \\ \\   *---*   /       \\\n"
                "     /   / / /   / \\   \\  \x1b[0m\x1b[31;1m,,,\x1b[0m\x1b[34;1m\\ \\   \\\x1b[0m\x1b[31;1m,    \x1b[0m\x1b[34;1m/   / \\   \\\n"
                "    *---* / *---* | *---* \x1b[0m\x1b[31;1m: ,:\x1b[0m\x1b[34;1m\\ *---*   *---* | *---*\n"
                "    |   |/  |   |/ \\|   | \x1b[0m\x1b[31;1m'   :\x1b[0m\x1b[34;1m\\|   |\x1b[0m\x1b[31;1m'' \x1b[0m\x1b[34;1m|   |/ \\|   |\n"
                "    *---*   *---*   *---*  \x1b[0m\x1b[31;1m'''  \x1b[0m\x1b[34;1m*---*   *---*   *---*\n"
                "                           \x1b[0m\x1b[31;1m,''',\n"
                "                            ' ::,\n                         ::' , ,:'\n                           '',  '\n"
                "                         ,''''':'',\n                          , ,:  :,:\n                                ''' ,,\n"
                "                              ':  , :\n                            ':,  ': :'\n                             ':',,' :\n"
                "                              ' ' :   ,\n                               ::,, '\n                                  :::\n"
                "                                ,,,''\n                                ',,\n\033[0m" << std::endl;

}


void print_help() {

    std::cerr << "Incremental Knightship Partial Extender, version 2.0\n"
                "====================================================\n\n"

                "Usage: ./ikpx2 -v VELOCITY [OTHER_OPTIONS...] [INPUT_FILES...]\n\n"

                "Minimal example: ./ikpx2 -v '(2,1)c/6'\n\n"

                "Options:\n"

                "   -v, --velocity          specify the velocity, e.g. '(2,1)c/6' or 'c/5d'\n"
                "   -k, --lookahead         number of logical rows by which to extend partials\n"
                "   -p, --threads           number of CPU threads to use (default 8)\n"
                "   -d, --directory         existing directory in which to save backup files\n"
                "   -b, --backup            number of seconds between adjacent backups\n"
                "   -m, --minimum-depth     only search from this position onwards\n"
                "   -w, --width             initial search width in logical columns\n"
                "   -x, --maximum-width     terminate the program when this width is exhausted\n"
                "   -h, --help              show this message and exit\n\n"

                "The input files can be either RLEs or ikpx2 backups (of the same velocity!).\n\n"

                "To change the underlying rule, use:\n\n"

                "    ./recompile.sh --rule RULE\n\n"

                "where RULE can be any lifelike or isotropic rule in Hensel notation.\n" << std::endl;

}
