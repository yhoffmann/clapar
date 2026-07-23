#ifndef CLAPAR_HPP_
#define CLAPAR_HPP_

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
  struct EntryNamePair;  // forward definition
  struct Entry;          // forward definition

 public:
  /* Add a flag. This will make `::get<bool>("--name-of-flag")` return true if
   * the flag is found in the command-line arguments, and false otherwise. */
  void addFlag(const EntryNamePair& namePair, const std::string& helpText = "");
  /* Single-name overload of `addFlag`. */
  void addFlag(const char* nameShort, const std::string& helpText = "");

  /* Add an entry to read in data from.
   * This will look for the names specified in `namePair` (or just the single,
   * short name for the overloaded function) and read in the parameter specified
   * after that argument. `helpText` is an optional variable to pass a text that
   * will be displayed when an argument is `--help`. */
  void addOption(const EntryNamePair& namePair, const std::type_index type,
                 const std::string& helpText = "");
  /* Overloaded function to allow passing a single name instead of the struct
   * holding two. */
  void addOption(const char* nameShort, const std::type_index type,
                 const std::string& helpText = "");

  /* The vector version of `addEntry`, which will do the same thing but look
   * for a vector of arguments like this (with hypothetical entry name `--vec`,
   * delimiter `,`, and type float): `--vec 1,2,3`. The delimiter can also be a
   * space, but then the vector arguments need to be passed with quotes around
   * them like this: `--vec '1 2 3'`. Make sure to NOT have 2 delimiters after
   * one another, as this will make the passing fail because it expects a value
   * between delimiters. You can however end with a delimiter (like this
   * `--vec 1,2,` <- notice trailing comma). */
  void addVectorOption(const EntryNamePair& namePair,
                       const std::type_index type, const char delimiter,
                       const std::string& helpText = "");
  /* Overloaded function to allow passing a single name instead of the struct
   * holding two. */
  void addVectorOption(const char* nameShort, const std::type_index type,
                       const char delimiter, const std::string& helpText = "");

  /* @brief Add arg to look for from struct `Entry`
   * @param entry The `Entry` struct containing the settings; both @see `Option`
   * and @see `Flag` inherit from this, so we can pass them both */
  void addEntry(const Entry& entry);

  /* Set a default value for an argument so that it always has a value set if
   * accessed. Side note: Similar behaviour can also be achieved by just writing
   * `.get<type>(argName).value_or(defaultVal)`. Important note: make sure
   * that the supplied type is actually what you want. For example, you need to
   * pass `std::string("some string")` instead of just `"some string"`, because
   * we cannot implicitly convert types here since this constructs a `std::any`
   * from `defaultVal`. */
  void setDefault(const std::string& argName, const std::any defaultVal);

  /* Function to add a function (`std::function<void(void)>` or lambda) to be
   * executed when the corresponding argument is found in the command-line
   * arguments. Does not call or error out if the argument is not found in the
   * command-line arguments. Does error if the `argName`, that is passed to the
   * function, is not found in the configured arguments. */
  void addFunction(const std::string& argName,
                   const std::function<void(void)> func);

  /* Add `helpText` to be printed in the help text for an argument */
  void addHelpText(const std::string& argName, const std::string& helpText);

  /* Add text to be printed in the help screen, for example a simple program
   * description like "Clapar: a command-line argument parser" */
  void setProgramDescription(const std::string& description);

  void shouldExitAtUnknownArg(const bool b = true);
  void shouldExitIfParamIsSameAsAnyArg(const bool b = true);

  void parse(const int argc, const char* const argv[]);

  /* Get a `std::optional` that may contain the value passed with the
   * corresponding entry/flag.
   * Make sure to get the type correct as this will error out if not. */
  template <typename T>
  std::optional<T> get(const std::string& argName) const {
    if (m_entriesMap.find(argName) != m_entriesMap.end()) {
      const EntryInternal& entry = *m_entriesMap.at(argName);

      if (!entry.val.has_value()) {
        return std::nullopt;
      } else
        return std::any_cast<T>(entry.val);
    } else
      return std::nullopt;
  }

  /* Function to get a `std::optional` with the vector containing vector
   * arguments. Will obviously error out if called on non-vector arguments. */
  template <typename T>
  std::optional<std::vector<T>> getVector(const std::string& argName) const {
    return get<std::vector<T>>(argName);
  }

  /* Less verbose function to quickly access a flag's state. We do not have to
   * specify the type to parse into as flags are always bool. */
  bool getFlag(const std::string& flagName) const;

  /* Get `argv[0]`, the path and name that the binary was called
   * with. */
  std::string argv0() const;
  /* Print the help text, which automatically includes any help texts that were
   * passed any of the functions to add an entry/flag. */
  void printHelp() const;
  /* Get the help text that would normally be printed by `printHelp` */
  std::string getHelp() const;

  Clapar() = default;
  Clapar(const std::string& description);
  Clapar(const Clapar&) = delete;
  Clapar(Clapar&&) = delete;
  Clapar& operator=(const Clapar&) = delete;
  Clapar& operator=(Clapar&&) = delete;
  ~Clapar() = default;

 protected:
  struct EntryInternal;  // forward definition
  enum StatusCode : int {
    ErrNoNameGiven = 70,
    ErrNameCollision,
    ErrParseFailed,
    ErrWrongUsage,
    ErrNoSuchArgument
  };
  void error(StatusCode exitCode, const std::string& msg);

 public:
  // struct that contains the name pair (long & short) for the entry
  struct EntryNamePair {
    std::string nameShort;
    std::string nameLong;

    operator std::string() {
      return (!nameShort.empty() ? nameShort : nameLong);
    }
    operator const std::string() const {
      return (!nameShort.empty() ? nameShort : nameLong);
    }
    operator std::string&() {
      return (!nameShort.empty() ? nameShort : nameLong);
    }
    operator const std::string&() const {
      return (!nameShort.empty() ? nameShort : nameLong);
    }
    operator std::string&&() {
      return std::move(!nameShort.empty() ? nameShort : nameLong);
    }
    std::string string() const {
      return (!nameShort.empty() ? nameShort : nameLong);
    }

    EntryNamePair() {};
    EntryNamePair(const char* const eitherName) : nameLong(eitherName) {}
    EntryNamePair(const std::string& eitherName) : nameLong(eitherName) {}
    EntryNamePair(const std::string& nameShort, const std::string& nameLong)
        : nameShort(nameShort), nameLong(nameLong) {}

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
        const std::vector<std::shared_ptr<EntryInternal>>& entries) const;
    bool operator==(const EntryNamePair& other) const {
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

  /* struct containing the possible types to convert the arguments to options
   * into */
  struct Type {
    static const std::type_index BOOL;
    static const std::type_index STRING;
    static const std::type_index FLOAT;
    static const std::type_index DOUBLE;
    static const std::type_index INT;
    static const std::type_index UNSIGNED;
    static const std::type_index CHAR;
  };

  /* @brief Struct containing config for an argument */
  struct Entry {
    /* @brief Whether the entry is a flag or not
     * @note Setting this to `true` (NOT default) will result in
     * these other members beging ignored: @see type @see defaultVal @see
     * delimiter @see addFlag (is `true`) @see addEntry (if `false`) */
    bool isFlag = false;
    /* @brief The name pair
     * @note @see addFlag @see addEntry */
    EntryNamePair namePair;
    /* @brief The type that is supplied
     * @note @see addEntry*/
    std::type_index type;
    /* @brief Text to be displayed when `--help` is found
     * @note @see addFlag */
    std::string helpText;
    /* @brief Default val that is stored
     * @see addFlag */
    std::any defaultVal{};
    /* @brief Delimiter for vector entry
     * @note If `isFlag` is `false`, the status of this will determine
     * whether the program will look for vector options or not: vector options
     * if set to anything; single entry if `std::nullopt` (default) */
    std::optional<char> delimiter{};
    /* @brief Function to call when found
     * @note @see addFunction */
    std::function<void(void)> function = []() {};

    Entry(
        const bool isFlag, const EntryNamePair& namePair,
        const std::type_index type, const std::string& helpText,
        const std::any defaultVal = {},
        const std::optional<char> delimiter = std::nullopt,
        const std::function<void(void)> function = []() {})
        : isFlag(isFlag)
        , namePair(namePair)
        , type(type)
        , helpText(helpText)
        , defaultVal(defaultVal)
        , delimiter(delimiter)
        , function(function) {};

    static Entry makeFlag(
        const EntryNamePair& namePair, const std::string& helpText,
        const std::function<void(void)> function = []() {}) {
      return Entry(true, namePair, Clapar::Type::BOOL, helpText, bool(false),
                   std::nullopt, function);
    }
    static Entry makeOption(
        const EntryNamePair& namePair, const std::type_index type,
        const std::string& helpText = "", const std::any defaultVal = {},
        const std::optional<char> delimiter = std::nullopt,
        const std::function<void(void)> function = []() {}) {
      return Entry(false, namePair, type, helpText, defaultVal, delimiter,
                   function);
    }
  };

 protected:
  /* struct containing the config for the command-line entry */
  struct EntryConfig {
    bool isFlag = false;

    struct {
      std::type_index type = typeid(std::string);
      std::optional<const char> delimiter = std::nullopt;
    } paramConfig;
  };

  struct EntryInternal {
    EntryNamePair namePair;
    EntryConfig config;
    std::string helpText = "";
    /* the variable holding the value that might be passed by command line */
    std::any val;
    /* a function to run when a certain argument is read in;
     * can be lambda because it is a std::function */
    std::function<void(void)> func = []() {};
    /* if both a long and a short entry name are supplied, we would iterate
     * over the same entry 2 times, meaning that the long entry could
     * overwrite the short one, or vice versa; this prevents that */
    bool wasProcessed = false;
  };

  std::vector<std::shared_ptr<EntryInternal>> m_entries;
  std::unordered_map<std::string, std::shared_ptr<EntryInternal>> m_entriesMap;
  bool m_shouldExitAtUnknownArg = false;
  bool m_shouldExitIfParamIsSameAsAnyArg = false;
  std::string m_argv0;
  std::string m_description =
      "A program using Clapar for the parsing of command-line arguments";

  std::ostream* userStream = &std::cerr;
  std::string userExitMsg =
      "There was a problem converting the following entry to its specified "
      "type (string in parentheses is the input provided): ";

  static constexpr const char* const ERRMSG_NO_NAME_GIVEN =
      "Clapar error: Only empty flag/entry names were provided. Exiting!";
  static constexpr const char* const ERRMSG_NAME_COLLISION_1 =
      "Clapar error: One of these names (\"";
  static constexpr const char* const ERRMSG_NAME_COLLISION_2 =
      "\") was assigned to multiple entries. Exiting!";

  void checkAndAdd(EntryInternal& entry);
  void processArgs(const std::vector<std::string>& args);
  void processEntry(const std::size_t argIdx,
                    const std::vector<std::string>& args, EntryInternal& entry,
                    bool& isParam);
  void processFlag(EntryInternal& entry);

  static const char* getTypeName(const std::type_index type);

  template <typename T>
  std::any convertToAny(const std::string& passedValue,
                        const std::optional<const char> delimiter) {
    if (!delimiter.has_value()) {
      if constexpr (std::is_same_v<T, std::string>) {
        return std::any(passedValue);
      } else if constexpr (std::is_same_v<T, char>) {
        return std::any(passedValue[0]);
      }
      T val;
      std::istringstream iss(passedValue);
      if (!(iss >> val) || !iss.eof() || iss.fail()) {
        error(
            StatusCode::ErrParseFailed,
            "Conversion failed. Wrong type was supplied for the parsing of \"" +
                passedValue + "\". Exiting!");
      }
      return std::any(val);
    } else {  // delimiter.has_value() == true
      TEST_LOG(__FUNCTION__ << passedValue << " " << delimiter.value())
      std::vector<T> vals;
      std::string buf;
      std::istringstream iss(passedValue);
      while (std::getline(iss, buf, delimiter.value())) {
        if constexpr (std::is_same_v<T, std::string>) {
          vals.push_back(buf);
        } else {
          T val;
          std::istringstream issSingle(buf);
          if (!(issSingle >> val) || !issSingle.eof() || issSingle.fail()) {
            error(StatusCode::ErrParseFailed,
                  "When trying to prase vector parameters \"" + passedValue +
                      "\", could not parse one of the values. This can also be "
                      "caused by having two or more delimiters after one "
                      "another without a parameter inbetween. Exiting!");
          }
          vals.push_back(val);
        }
      }
      return std::any(vals);
    }
  }

 protected:
  static std::ostream& setBold(std::ostream& os) {
    return os
#ifdef __linux__
           << "\033[1m"
#endif
        ;
  }
  static std::ostream& unsetBold(std::ostream& os) {
    return os
#ifdef __linux__
           << "\033[0m"
#endif
        ;
  }
};
#endif  // CLAPAR_HPP_
