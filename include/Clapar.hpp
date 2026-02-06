#ifndef CLAPAR_HPP_
#define CLAPAR_HPP_

#define CLAPAR_ERR_NO_NAME_GIVEN 70
#define CLAPAR_ERR_NAME_COLLISION 71
#define CLAPAR_ERR_PARSE_FAILED 72
#define CLAPAR_ERR_WRONG_USAGE 73
#define CLAPAR_ERR_NO_SUCH_ARGUMENT 74

#include <any>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

#ifdef CLAPAR_TEST_COMPILE
#define TEST_LOG(x) std::cerr << x << std::endl;
#else
#define TEST_LOG(x)
#endif

class Clapar {
 public:
  struct OptionNamePair;  // forward definition

 public:
  void addFlag(const OptionNamePair& namePair,
               const std::string& helpText = "");
  void addFlag(const char* nameShort, const std::string& helpText = "");
  void addOption(const OptionNamePair& namePair, const std::type_index type,
                 const std::string& helpText = "",
                 const std::size_t numArgs = 1,
                 const char vectorArgsDelimiter = ',');
  void addOption(const char* nameShort, const std::type_index type,
                 const std::string& helpText = "",
                 const std::size_t numArgs = 1,
                 const char vectorArgsDelimiter = ',');
  void addFunction(const std::string& argName,
                   const std::function<void(void)> func);

  /* Add `helpText` to be printed in the help text for an argument */
  void addHelpText(const std::string& argName, const std::string& helpText);

  /* Add text to be printed in the help screen, for example a simple program
   * description like "Clapar: a command-line argument parser" */
  void setProgramDescription(const std::string& description) {
    m_description = description;
  }

  void parse(int argc, char* argv[]);

  template <typename T>
  std::optional<T> get(const std::string& argName) {
    if (m_optionsMap.contains(argName)) {
      const Option& option = *m_optionsMap[argName];

      const std::any& val = option.val;
      if (!val.has_value()) {
        return std::nullopt;
      } else
        return std::any_cast<T>(val);
    } else
      return std::nullopt;
  }
  std::string argv0() const { return m_argv0; }
  void printHelp() const;
  std::string getHelp() const;

  Clapar() = default;
  Clapar(int argc, char* argv[]) { parse(argc, argv); }
  Clapar(const std::string& description) : m_description(description) {}
  Clapar(const Clapar&) = delete;
  Clapar(Clapar&&) = delete;
  Clapar& operator=(const Clapar&) = delete;
  Clapar& operator=(Clapar&&) = delete;
  ~Clapar() = default;

 protected:
  struct Option;  // forward definition

 public:
  // struct that contains the name pair (long & short) for the option
  struct OptionNamePair {
    std::string nameShort;
    std::string nameLong;

   private:
    friend class Clapar;

    enum Validity {
      CollisionWithOther = -1,
      NoneGiven = 0,
      SGiven,
      LGiven,
      BothGiven
    };
    /* -does a bunch of checks on the provided name(s)
     * -among other checks, this will make sure that any name at all has been
     * provided. */
    Validity getValidity(
        const std::vector<std::shared_ptr<Option>>& options) const;
    bool operator==(const OptionNamePair& other) const {
      return (nameShort == other.nameShort && nameLong == other.nameLong);
    }
    bool contains(const std::string& name) const {
      return name == nameShort || name == nameLong;
    }
    bool containsIgnoreEmpty(const std::string& name) const {
      return (!nameShort.empty() && name == nameShort) ||
             (!nameLong.empty() && name == nameLong);
    }
  };

  // struct containing the possible types to convert the arguments to options
  // into
  struct Type {
    static const std::type_index BOOL;
    static const std::type_index STRING;
    static const std::type_index FLOAT;
    static const std::type_index DOUBLE;
  };

 protected:
  /* struct containing the config for the command-line option */
  struct OptionConfig {
    bool isFlag = false;

    struct {
      std::type_index type = typeid(std::string);

      struct {
        std::size_t numArgs = 1;
        char delimiter = ',';
      } vectorOptions;
    } paramOptions;
  };

  struct Option {
    OptionNamePair namePair;
    OptionConfig config;
    std::string helpText = "";
    /* the variable holding the value that might be passed by command line */
    std::any val;
    /* a function to run when a certain argument is read in;
     * can be lambda because it is a std::function */
    std::function<void(void)> func = []() {};
    /* if both a long and a short option name are supplied, we would iterate
     * over the same option 2 times, meaning that the long option could
     * overwrite the short one, or vice versa; this prevents that */
    bool wasProcessed = false;
  };

  std::vector<std::shared_ptr<Option>> m_options;
  std::unordered_map<std::string, std::shared_ptr<Option>> m_optionsMap;
  std::string m_argv0;
  std::string m_description =
      "A program using Clapar for the parsing of command-line arguments";

  std::ostream* userStream = &std::cerr;
  std::string userExitMsg =
      "There was a problem converting the following option to its specified "
      "type (string in parentheses is the input provided): ";

  static constexpr const char* const ERRMSG_NO_NAME_GIVEN =
      "Clapar error: Only empty flag/option names were provided. Exiting!";
  static constexpr const char* const ERRMSG_NAME_COLLISION_1 =
      "Clapar error: One of these names (\"";
  static constexpr const char* const ERRMSG_NAME_COLLISION_2 =
      "\") was assigned to multiple options. Exiting!";

  void checkAndAdd(Option& option);
  void processArgs(const std::vector<std::string>& args);
  void processOption(const std::size_t argIdx, const std::vector<std::string>& args,
                     Option& option);
  template <typename T>
  std::any convertToAny(const std::string& passedValue) {
    if constexpr (std::is_same_v<T, std::string>) {
      return std::any(passedValue);
    }
    T val;
    std::istringstream iss(passedValue);
    if (!(iss >> val) || !iss.eof() || iss.fail()) {
      std::cerr
          << "Clapar error: Conversion failed. Wrong type was supplied for the "
             "parsing (for example trying to parse string->float). Exiting!"
          << std::endl;
      exit(CLAPAR_ERR_PARSE_FAILED);
    }
    return std::any(val);
  }
};
#endif  // CLAPAR_HPP_
