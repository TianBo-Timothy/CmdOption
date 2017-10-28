#include <getopt.h>
#include <sstream>
#include <iostream>
#include <vector>
#include <map>

#pragma once

/**
 * This classes store a value in its string form, it can be convert to desired
 * types when needed. This is used as return type of CmdOption's [] operator.
 *
 * As per performance, the class is a light-weight wrapper of original string,
 * the overhead is minimal.
 */

class StringValue
{
private:
    // store the string value(s), if multiple strings are added ( see add() ),
    // those strings are separated by "\n".
    std::string m_text;

    // number of strings that have been stored
    int m_count = 0;

public:
    /**
     * default constructor
     */
    StringValue()
    {
        // do nothing
    }

    /**
     * Constructor
     *
     * In most cases, this constructor is used to construct the object with a
     * string.
     *
     * @param str
     * a string
     */
    StringValue(const std::string & str)
    {
        m_text = str;
        m_count = 1;
    }

    /**
     * Add a new string to the string value
     *
     * @param str
     * a new string
     */
    void add(const std::string & str)
    {
        if (m_count == 0) {
            m_text = str;
        }
        else {
            m_text += "\n" + str;
        }
        ++m_count;
    }

    /**
     * Check if the object has been initialized
     *
     * @return
     * true in case the object is initialized.
     */
    explicit operator bool() const
    {
        return (m_count > 0);
    }

    /**
     * Get number of values the object stores
     *
     * @return
     * number of values the object stores
     */
    int count() const
    {
        return m_count;
    }

    /**
     * Implicit conversion operator
     *
     * It is essentially a short hand to as<>()
     */
    template<typename T>
    operator T() const
    {
        return as<T>();
    }

    /**
     * A short hand of as<std::string>().
     *
     * Example:
     * StringValue sv("abc");
     * std::string s;
     * s = sv.str();
     *
     * Note that the above can be done implicitly:
     *
     * StringValue sv("foo");
     * std::string s = sv;
     *
     * The problem of implicit conversion for std::string is that it works only
     * for copy constructor. The following case does not work as it is
     * ambiguous (note StringValue -> int -> char -> std::string):
     *
     * std::string s;
     * s = sv;
     *
     * The workaround is to use the as<> function:
     *
     * s = sv.as<std::string>();
     *
     * As string is quite common type, we provide the function str() as a
     * shorthand replacement.
     */
    std::string str() const
    {
        return as<std::string>();
    }

    /**
     * Get the value of type T from the object.
     *
     * If the object was not initialized, use the default value @c t instead
     *
     * @param t
     * Default value
     *
     * @return
     * the value from the object if possible or t if otherwise
     */
    template<typename T>
    T valueOr(T t) const
    {
        T ret;
        if (m_count == 0) {
            ret = t;
        }
        else {
            try {
                ret = as<T>();
            }
            catch (...) {
                ret = t;
            }
        }
        return ret;
    }

    /**
     *  Overload of valueOr for C string
     */
    std::string valueOr(const char * pstr) const
    {
        std::string ret;
        if (m_count == 0) {
            if (pstr != nullptr) {
                ret = pstr;
            }
        }
        else {
            ret = m_text;
        }
        return ret;
    }

    /**
     * Interpret the string as value in given type T
     *
     * @tparam T
     * Template parameter T can be int, long, float, double or std::string
     *
     * @return
     * Value in type T
     *
     * @throw
     * std::invalid_argument if the conversion cannot be done.
     */
    template<typename T>
    T as() const
    {
        validate();
        T v;
        getValue(m_text, v);
        return v;
    }

private:

    // check if the object has been initialized
    void validate() const
    {
        if (m_count == 0) {
            throw std::invalid_argument("null value");
        }
    }

    // the implementation of as() function, it assumes the parameter is valid
    template<typename T>
    void getValue(const std::string & str, T& v) const
    {
        std::size_t pos;
        stox(str, &pos, v);
        if (pos != str.length()) {
            throw std::invalid_argument("stox");
        }
    }

    // overload version of getValue() for std::string
    void getValue(const std::string & str, std::string & v) const
    {
        v = str;
    }

    /*
     * Interpret the string as a vector
     *
     * The strings added by add() function are stored internaly as "\n"
     * separated string. The string is parsed again to get the returned vector.
     *
     * In case there is only one string added, the return vector size will be 1.
     *
     * @tparam T
     * Template parameter T can be int, long, float, double or std::string
     *
     * @throw
     * std::invalid_argument if the conversion cannot be done.
     */
    template<typename T>
    void getValue(const std::string & str, std::vector<T> & vec) const
    {
        std::stringstream s(m_text);
        std::string line;

        while (std::getline(s, line)) {
            T v;
            getValue(line, v);
            vec.push_back(v);
        }
    }

    // overload versions of stoi, stol, stof, stod

    void stox(const std::string & str, std::size_t * pos, int & v) const
    {
        v = std::stoi(str, pos);
    }

    void stox(const std::string & str, std::size_t * pos, long & v) const
    {
        v = std::stol(str, pos);
    }

    void stox(const std::string & str, std::size_t * pos, float & v) const
    {
        v = std::stof(str, pos);
    }

    void stox(const std::string & str, std::size_t * pos, double & v) const
    {
        v = std::stod(str, pos);
    }
};

/**
 * This class represents command line options
 *
 * The major difference from other existing solutions is that this class do not
 * require manually adding option definitions. Instead, it adds those definition
 * by parsing usage text.
 *
 * Another minor difference is that accessing parsed options can be done with
 * implicit conversion along with explicit means. Those conversions are done
 * with the class StringValue. See above for more information.
 */
class CmdOption
{

public:
    /**
     * Initialize the options with usage text
     *
     * @param usage
     * The usage text following the linux system manual page style. For example:
     *
     *
     * -a, --all show all elements, no arguments required
     * -b, --batch  this option is separated by more than one space
     * -c  no long option and no argument required
     * -d --delta=NUM set delta number, need argument
     * -e --epsilon[=NUM] requires optional argument
     *     with or without comma ',' after short option are OK
     *     the lines not started with '-' will be ignored
     *
     * -f FILE
     *     delete a file, no long option, need argument; in this case,
     *     explanation must be in separate line
     */
    void operator<<(const std::string & usage)
    {
        m_usage = usage;
        init();
    }

    /**
     * Show usage
     *
     * @param os
     * output stream, default is std::cout
     */
    void usage(std::ostream& os = std::cout)
    {
        os << m_usage << std::endl;
    }

    /**
     * Check the status of the object
     *
     * @return
     * true if no error encountered
     */
    bool good()
    {
        return m_errorStr.empty();
    }

    /**
     * Parse the command line
     *
     * @param argc
     * @param argv
     * The parameters passed to main()
     */
    void parse(int argc, char** argv)
    {
        opterr = 0; // tell getopt_long not to print invalid option on screen

        while (true) {
            int this_option_optind = optind ? optind : 1;
            int option_index = 0;

            int c = getopt_long(argc, argv, m_shortOptStr.c_str(),
                    &m_longOptions[0], &option_index);

            if (c < 0) {
                // no more options
                break;
            }

            int index;
            if (c == 0) {
                // get a long option

                index = m_indexMap[m_longOptNames[option_index]];
            }
            else if (c == '?') {
                // unknown option
                addErrorStr(std::string("Unknown option: ") + char(optopt));
                continue;
            }
            else if (c == ':') {
                // missing option argument
                addErrorStr(std::string("Missing argument for: ") + char(optopt));
                continue;
            }
            else {
                // a short option
                std::string str;
                str = (char)c;
                auto it = m_indexMap.find(str);
                if (it == m_indexMap.end()) {
                    addErrorStr(std::string("unknown short option: ") + str);
                    break;
                }

                index = it->second;
            }

            if (optarg) {
                m_options[index].add(optarg);
            }
            else {
                m_options[index].add("");
            }
        }

        if (optind < argc) {
            // the rest are arguments
            while (optind < argc) {
                m_arguments.add(argv[optind++]);
            }
        }

        // reset the global variable in case multiple parsings are required
        optind = 0;
    }

    /**
     * Access an option
     *
     * @param opt
     * short or long option name
     *
     * @return
     * A StringValue object that can be converted to various types
     */
    StringValue& operator[](const std::string & opt)
    {
        auto it = m_indexMap.find(opt);
        if (it == m_indexMap.end()) {
            throw std::invalid_argument("unknown option: " + opt);
        }

        int index = it->second;

        auto it2 = m_options.find(index);
        if (it2 == m_options.end()) {
            return m_nullStrValue;
        }

        return it2->second;
    }

    /**
     * Access arguments
     *
     * @return
     * A reference to the arguments
     */
    const StringValue & arguments()
    {
        return m_arguments;
    }

    /**
     *
     */
    void reportError(std::ostream & os = std::cerr)
    {
        if (!good()) {
            os << m_errorStr << std::endl;
        }
    }

    /**
     * Debug output
     *
     * It prints how the object understand the option settings.
     */
    void debugReport()
    {
        std::cout << "\n";
        std::cout << "short option string: " << m_shortOptStr << "\n" << std::endl;

        std::cout << "long options\n";
        for (auto opt : m_longOptions) {
            if (opt.name) {
                std::cout << opt.name << "\t" << opt.has_arg << "\t" << (char) opt.val << std::endl;
            }
        }
        std::cout << std::endl;

        if (!m_options.empty()) {
            std::cout << "options"  << std::endl;
            for (auto & item : m_options) {
                for (auto & optItem : m_indexMap) {
                    if (optItem.second == item.first) {
                        std::cout << optItem.first << " ";
                    }
                }
                std::cout << item.second.str() << std::endl;
            }
            std::cout << std::endl;
        }

        if (m_arguments) {
            std::cout << "arguments"  << std::endl;
            std::cout << m_arguments.str() << "\n" << std::endl;;
        }

        if (!m_errorStr.empty()) {
            std::cout << "error: " << m_errorStr << std::endl;
        }
    }

private:

    /**
     * Initialization
     *
     * In this function, based on usage information, we construct short option
     * string and long option struct as required by getopt_long() function.
     * Furthermore, as some options have both short and long option and user
     * will provide one of them, we also construct an internal index maps to
     * map short options and long options to an index set so that one option
     * will map to a unique index.
     *
     * Note: in theory, short option and long option may have the same name,
     * for example:
     * -a, --all
     * -b, --a
     *
     * The second long option collides with the first short option. However, this
     * case is rare and not meaningful in practice and we just issue an error
     * as duplicate option in this case.
     */
    void init()
    {
        std::stringstream s(m_usage);
        std::string line;
        int i = 0;
        while (good() && std::getline(s, line)) {
            parseLine(i++, line);
        }

        m_longOptions.push_back({0, 0, 0, 0});
        for (size_t i = 0; i < m_longOptNames.size(); ++i) {
            m_longOptions[i].name = m_longOptNames[i].c_str();
        }
    }

    /**
     * Parse one usage line
     *
     * @param i
     * The line number of usage, used when reporting error
     *
     * @param line
     * The content of the line
     */
    void parseLine(int i, const std::string & line)
    {
        if (!parseOptLine(i, line)) {
            addErrorStr("invalid option at line: " + std::to_string(i) + "\n" + line);
        }
    }

    /**
     * Add error string
     */
    void addErrorStr(const std::string & str)
    {
        if (!m_errorStr.empty()) {
            m_errorStr += "\n";
        }
        m_errorStr += str;
    }

    /**
     * The actual implementation of parseLine
     */
    bool parseOptLine(int i, const std::string & line)
    {
        std::stringstream ss(line);

        std::string word;
        char shortOpt = 0;
        std::string longOpt;
        int argReqmt = 0;

        int n = 0;  // number of words encountered
        while (ss >> word) {
            ++n;

            if (n > 2) {
                // we need to count the number of words for disambiguation, e.g.

                // -f this is explanation
                // -f FILE
                //    This is explanation
                //
                // we consider FILE as argument becuase there is no explanation
                // text after it.
                break;
            }

            if (word.length() <= 1) {
                return false;
            }

            if (word[0] != '-') {
                if (n == 1) {
                    // the first word does not start with '-', this is not an
                    // option line, ignore the entire line
                    return true;
                }

                continue; // ignore the word
            }

            if (word[1] == '-') { // long option

                auto pos = word.find_first_of("=");
                if (pos == std::string::npos) {
                    longOpt = word.substr(2);
                    argReqmt = no_argument;
                }
                else {
                    if (word[pos - 1] == '[') {
                        if (word.back() != ']') {
                            return false;
                        }
                        longOpt = word.substr(2, pos - 1 - 2);
                        argReqmt = optional_argument;
                    }
                    else {
                        longOpt = word.substr(2, pos - 2);
                        argReqmt = required_argument;
                    }
                }
            }
            else { // short option
                if ( (shortOpt != 0) ||     // set before
                    ((word.length() == 3) && (word[2] != ',')) || // not end with comma
                    (word.length() > 3) ) { // extra characters

                    return false;
                }

                shortOpt = word[1];
            }
        }

        if (n == 0) {
            return true;    // ignore empty lines
        }

        if ( (shortOpt == 0) && longOpt.empty()) {
            // this should not happen
            return false;
        }

        if (longOpt.empty()) {
            // No long option, so we decide the argument requirement by short
            // option. If only one word followed after short option, then
            // argument is required
            argReqmt = (n == 2)? required_argument: no_argument;
        }


        bool indexUsed = false;
        if (shortOpt != 0) {
            m_shortOptStr += shortOpt;
            if (argReqmt == required_argument || argReqmt == optional_argument) {
                m_shortOptStr += ":";
            }

            // add to the map
            std::string str;
            str = shortOpt;
            if (m_indexMap.find(str) != m_indexMap.end()) {
                addErrorStr("duplicate short option: " + str);
            }
            else {
                m_indexMap[str] = m_maxIndex;
                indexUsed = true;
            }
        }

        if (!longOpt.empty()) {
            m_longOptNames.push_back(longOpt);
            m_longOptions.push_back({0, argReqmt, 0, shortOpt});    // do not set the pointer at the moment

            if (m_indexMap.find(longOpt) != m_indexMap.end()) {
                addErrorStr("duplicate long option: " + longOpt);
            }
            else {
                m_indexMap[longOpt] = m_maxIndex;
                indexUsed = true;
            }
        }

        if (indexUsed) {
            ++m_maxIndex;
        }

        return true;
    }

private:
    std::string m_usage;
    std::string m_errorStr;

    // starting with colon will make getopt() to report colon on missing
    // argument
    std::string m_shortOptStr = ":";

    std::vector<struct option> m_longOptions;
    std::vector<std::string> m_longOptNames;

    int m_maxIndex = 0;    // used only during building up the maps
    std::map<std::string, int> m_indexMap;
    std::map<int, StringValue> m_options;
    StringValue m_arguments;
    StringValue m_nullStrValue;
};

