#include "../include/Clapar.hpp"

#include <algorithm>

const std::type_index Clapar::Type::BOOL = typeid(bool);
const std::type_index Clapar::Type::STRING = typeid(std::string);
const std::type_index Clapar::Type::FLOAT = typeid(float);
const std::type_index Clapar::Type::DOUBLE = typeid(double);
const std::type_index Clapar::Type::INT = typeid(int);
const std::type_index Clapar::Type::UNSIGNED = typeid(std::size_t);
const std::type_index Clapar::Type::CHAR = typeid(char);

Clapar::OptionNamePair::Validity Clapar::OptionNamePair::getValidity(
    const std::vector<std::shared_ptr<Option>>& options) const {
  const bool isNameHelp = nameShort == "--help" || nameLong == "--help";
  for (const auto& option : options) {
    const bool doesNameExist = [&option, this]() -> bool {
      const Clapar::OptionNamePair& otherNamePair = option->namePair;
      return (otherNamePair.containsIgnoreEmpty(nameShort) ||
              otherNamePair.containsIgnoreEmpty(nameLong));
    }();
    if (doesNameExist || isNameHelp) {
      return CollisionWithOther;
    }
  }

  if (nameShort.empty() && nameLong.empty()) {
    return NoneGiven;
  } else if (nameShort.empty()) {
    return LGiven;
  } else if (nameLong.empty()) {
    return SGiven;
  } else {
    return BothGiven;
  }
}

void Clapar::addFlag(const Clapar::OptionNamePair& namePair,
                     const std::string& helpText) {
  Clapar::Option option = {
      namePair,
      {
          true,
          {Clapar::Type::BOOL},
      },
      helpText,
      std::any(),
  };

  checkAndAdd(option);
}

void Clapar::addFlag(const char* nameShort, const std::string& helpText) {
  addFlag({nameShort, ""}, helpText);
}

void Clapar::addOption(const Clapar::OptionNamePair& namePair,
                       const std::type_index type,
                       const std::string& helpText) {
  Clapar::Option option = {
      namePair,
      {
          false,
          {type},
      },
      helpText,
      std::any(),
  };

  checkAndAdd(option);
}

void Clapar::addOption(const char* nameShort, const std::type_index type,
                       const std::string& helpText) {
  addOption({nameShort, ""}, type, helpText);
}

void Clapar::addVectorOption(const OptionNamePair& namePair,
                             const std::type_index type, const char delimiter,
                             const std::string& helpText) {
  Clapar::Option option = {
      namePair,
      {
          false,
          {type, delimiter},
      },
      helpText,
      std::any(),
  };

  checkAndAdd(option);
}

void Clapar::addVectorOption(const char* nameShort, const std::type_index type,
                             const char delimiter,
                             const std::string& helpText) {
  addVectorOption({nameShort, ""}, type, delimiter, helpText);
}

void Clapar::setDefault(const std::string& argName, const std::any defaultVal) {
  m_optionsMap[argName]->val = defaultVal;
}

void Clapar::addFunction(const std::string& argName,
                         const std::function<void(void)> func) {
  if (m_optionsMap.find(argName) != m_optionsMap.end()) {
    Option& option = *m_optionsMap[argName];
    option.func = func;
  } else {
    error(ErrNoSuchArgument,
          "Tried assigning function to option \"" + argName +
              "\", but no option with the name exists. Exiting!");
  }
}

void Clapar::addHelpText(const std::string& argName,
                         const std::string& helpText) {
  if (m_optionsMap.find(argName) != m_optionsMap.end()) {
    m_optionsMap[argName]->helpText = helpText;
  } else {
    error(ErrNoSuchArgument,
          "Tried assigning help text to option \"" + argName +
              "\", but no option with that names exists. Exiting!");
  }
}

void Clapar::parse(int argc, char* argv[]) {
  m_argv0 = argv[0];
  const std::vector<std::string> args = [argc, argv, this]() {
    std::vector<std::string> temp;
    for (int argIdx = 1; argIdx < argc; argIdx++) {
      temp.emplace_back(argv[argIdx]);
      if (temp.back() == "--help") {
        printHelp();
        exit(0);
      }
    }

    return temp;
  }();

  processArgs(args);
}

void Clapar::printHelp() const { std::cout << getHelp(); }

std::string Clapar::getHelp() const {
  std::vector<std::shared_ptr<Option>> optionsTemp(m_options);
  std::sort(optionsTemp.begin(), optionsTemp.end(),
            [](const std::shared_ptr<Option> left,
               const std::shared_ptr<Option> right) -> bool {
              const std::string& leftComparator =
                  ("" != left->namePair.nameLong) ? left->namePair.nameLong
                                                  : left->namePair.nameShort;
              const std::string& rightComparator =
                  ("" != right->namePair.nameLong) ? right->namePair.nameLong
                                                   : right->namePair.nameShort;

              return leftComparator <= rightComparator;
            });

  std::ostringstream oss;

  oss << m_argv0 << ": " << m_description << "\n";
  for (const auto& optionPtr : optionsTemp) {
    const bool isShortEmpty = optionPtr->namePair.nameShort.empty();
    const bool isLongEmpty = optionPtr->namePair.nameLong.empty();
    const bool isOptionNameTextEmpty = optionPtr->helpText.empty();
    const bool isLastEntry = &optionPtr == &optionsTemp.back() && optionPtr;
    const std::string type = getTypeName(optionPtr->config.paramOptions.type);

    oss << setBold << "  " << (!isLongEmpty ? optionPtr->namePair.nameLong : "")
        << unsetBold << (isShortEmpty || isLongEmpty ? "" : ", ") << setBold
        << (!isShortEmpty ? optionPtr->namePair.nameShort : "") << unsetBold
        << (!optionPtr->config.isFlag
                ? " <" + type +
                      (optionPtr->config.paramOptions.delimiter.has_value()
                           ? ",...>"
                           : ">")
                : "")
        << (isOptionNameTextEmpty ? "" : ":\n\t" + optionPtr->helpText)
        << (!isLastEntry ? "\n" : "");
  }

  return oss.str();
}

void Clapar::checkAndAdd(Option& option) {
  const Clapar::OptionNamePair::Validity validity =
      option.namePair.getValidity(m_options);

  m_options.push_back(std::make_shared<Option>(option));
  std::shared_ptr<Option> optionPtr = m_options.back();

  std::string& nameShort = optionPtr->namePair.nameShort;
  std::string& nameLong = optionPtr->namePair.nameLong;
  switch (validity) {
    case OptionNamePair::SGiven:
      m_optionsMap[nameShort] = optionPtr;
      break;

    case OptionNamePair::BothGiven:
      m_optionsMap[nameShort] = optionPtr;
      m_optionsMap[nameLong] = optionPtr;
      break;

    case OptionNamePair::LGiven:
      m_optionsMap[nameLong] = optionPtr;
      break;

    case OptionNamePair::NoneGiven:
      error(ErrNoNameGiven, ERRMSG_NO_NAME_GIVEN);
      break;

    case OptionNamePair::CollisionWithOther:
      error(ErrNameCollision, ERRMSG_NAME_COLLISION_1 +
                                  optionPtr->namePair.nameShort + "\" or \"" +
                                  optionPtr->namePair.nameLong +
                                  ERRMSG_NAME_COLLISION_2);
      break;
  }
}

void Clapar::processArgs(const std::vector<std::string>& args) {
  for (std::size_t argIdx = 0, numArgs = args.size(); argIdx < numArgs;
       argIdx++) {
    const std::string& arg = args[argIdx];
    for (auto& [key, option] : m_optionsMap) {
      if (option->wasProcessed) continue;

      const bool isArg = (arg == option->namePair.nameShort ||
                          arg == option->namePair.nameLong);
      if (isArg) {
        processOption(argIdx, args, *option);
        option->wasProcessed = true;
        break; /* matching arg was found: break means that we do not look for
                * the same arg again, meaninig that only the first time an arg
                * is provided on the command-line, will it be read; all mentions
                * beyond the first are discarded and the associated function is
                * called only when the first arg is assigned */
      }
    }
  }
}

void Clapar::processOption(const std::size_t argIdx,
                           const std::vector<std::string>& args,
                           Option& option) {
  {
    const bool needsNoPassedValue = option.config.isFlag;
    if (needsNoPassedValue) {
      option.val = std::any(true);
      option.func();
      return;
    }
  }

  {
    const bool isLastArg = argIdx + 1 >= args.size();
    if (isLastArg) {
      error(ErrWrongUsage, "Wrong usage: option " + option.namePair.string() +
                               " needs a passed value but has none. Exiting!");
    }
  }

  const std::string& passedValue = args[argIdx + 1];

  if (m_optionsMap.find(passedValue) != m_optionsMap.end()) {
    error(ErrWrongUsage, "Passed the name of option \"" + passedValue +
                             "\" as a parameter to another option. THis is "
                             "probably not what you want. Exiting!");
  }

  std::optional<const char> delimiter = option.config.paramOptions.delimiter;
  const std::type_index type = option.config.paramOptions.type;
  if (type == Type::STRING) {
    option.val = convertToAny<std::string>(passedValue, delimiter);
  } else if (type == Type::BOOL) {
    option.val = convertToAny<bool>(passedValue, delimiter);
  } else if (type == Type::FLOAT) {
    option.val = convertToAny<float>(passedValue, delimiter);
  } else if (type == Type::DOUBLE) {
    option.val = convertToAny<double>(passedValue, delimiter);
  } else if (type == Type::INT) {
    option.val = convertToAny<int>(passedValue, delimiter);
  } else if (type == Type::UNSIGNED) {
    option.val = convertToAny<std::size_t>(passedValue, delimiter);
  } else if (type == Type::CHAR) {
    option.val = convertToAny<char>(passedValue, delimiter);
  } else {
    std::cerr << "Clapar warning: Could not convert passed value ("
              << passedValue << ") to std::any." << std::endl;
  }

  option.func();
}

const char* Clapar::getTypeName(const std::type_index type) {
  if (type == Type::STRING) {
    return "string";
  } else if (type == Type::BOOL) {
    return "bool";
  } else if (type == Type::FLOAT) {
    return "float";
  } else if (type == Type::DOUBLE) {
    return "double";
  } else if (type == Type::INT) {
    return "int";
  } else if (type == Type::UNSIGNED) {
    return "unsigned";
  } else if (type == Type::CHAR) {
    return "char";
  } else {
    return "";
  }
}

void Clapar::error(Clapar::StatusCode exitCode, const std::string& msg) {
  *(userStream) << m_argv0 << " Clapar error: " << msg << std::endl;
  exit(static_cast<int>(exitCode));
}
