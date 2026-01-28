#ifndef CLAPAR_HPP_
#define CLAPAR_HPP_

#define CLAPAR_ERROR_NO_NAME_GIVEN 70
#define CLAPAR_ERROR_NAME_COLLISION 71
#define CLAPAR_ERROR_PARSE_FAILED 72
#define CLAPAR_ERROR_WRONG_USAGE 73

#include <any>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

class Clapar {
 public:
  struct OptionNamePair;  // forward definition

 public:
  void addFlag(const OptionNamePair& namePair,
               const std::vector<std::string>& beforeThese = {},
               const std::vector<std::string>& afterThese = {});
  void addOption(const OptionNamePair& namePair, const std::type_index type,
                 const std::size_t numArgs = 1,
                 const char vectorArgsDelimiter = ',',
                 const std::vector<std::string>& beforeThese = {},
                 const std::vector<std::string>& afterThese = {});
  template <typename T>
  std::optional<T> get(const std::string& argName) {
    if (m_options.contains(argName)) {
      const Option& option = *m_options[argName];

      const std::any& val = option.val;
      if (!val.has_value()) {
        return std::nullopt;
      } else
        return std::any_cast<T>(val);
    } else
      return std::nullopt;
  }
  void parse(int argc, char* argv[]);
  std::string argv0() { return m_argv0; }

  Clapar() = default;
  Clapar(int argc, char* argv[]) { parse(argc, argv); }
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
        const std::unordered_map<std::string, std::shared_ptr<Option>> options)
        const;
    bool operator==(const OptionNamePair& other) const {
      return (nameShort == other.nameShort && nameLong == other.nameLong);
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

    /* vectors  */
    std::vector<std::string> beforeThese;
    std::vector<std::string> afterThese;

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
    /* the variable holding the value that might be passed by command line */
    std::any val;
    /* this is not strictly necessary for the program to function correctly, but
     * does improve performance because we would be iterating over the same
     * options up to 2 times because `m_options` contains up to 2 keys the refer
     * to the same deref'd share_ptr 
     * NOTE: this does however force the only */
    bool wasProcessed = false;
  };

  std::unordered_map<std::string, std::shared_ptr<Option>> m_options;
  std::string m_argv0;

  std::ostream* userStream = &std::cerr;
  std::string userExitMsg =
      "There was a problem converting the following option to its specified "
      "type (string in parentheses is the input provided): ";

  static constexpr const char* const ERRMSG_NO_NAME_GIVEN =
      "Clapar ERR: Only empty flag/option names were provided. Exiting!";
  static constexpr const char* const ERRMSG_NAME_COLLISION_1 =
      "Clapar ERR: One of these names (\"";
  static constexpr const char* const ERRMSG_NAME_COLLISION_2 =
      "\") was assigned to two options. Exiting!";

  void checkAndAdd(const Option& option);
  void processArgs(const std::vector<std::string>& args);
  void processOption(const std::size_t argIdx,
                     const std::vector<std::string>& args, Option& option);
  template <typename T>
  std::any convertToAny(std::istringstream& ss) {
    T val;
    if (!(ss >> val) || !ss.eof() || ss.fail()) {
      std::cerr
          << "Clapar error: Conversion failed. Wrong type was supplied for the "
             "parsing (for example trying to parse string->float). Exiting!"
          << std::endl;
      exit(CLAPAR_ERROR_PARSE_FAILED);
    }
    return std::any(val);
  }
};
#endif  // CLAPAR_HPP_
