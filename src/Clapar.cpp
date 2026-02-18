#include "../include/Clapar.hpp"

#include <algorithm>

const std::type_index Clapar::Type::BOOL = typeid(bool);
const std::type_index Clapar::Type::STRING = typeid(std::string);
const std::type_index Clapar::Type::FLOAT = typeid(float);
const std::type_index Clapar::Type::DOUBLE = typeid(double);

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
  Clapar::Option option = {.namePair = namePair,
                           .config =
                               {
                                   .isFlag = true,
                                   .paramOptions = {.type = Clapar::Type::BOOL},
                               },
                           .helpText = helpText,
                           .val = std::any()};

  checkAndAdd(option);
}

void Clapar::addFlag(const char* nameShort, const std::string& helpText) {
  addFlag({nameShort, ""}, helpText);
}

void Clapar::addOption(const Clapar::OptionNamePair& namePair,
                       const std::type_index type,
                       const std::string& helpText) {
  Clapar::Option option = {
      .namePair = namePair,
      .config = {.isFlag = false, .paramOptions = {.type = type}},
      .helpText = helpText,
      .val = std::any()};

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
      .namePair = namePair,
      .config = {.isFlag = false,
                 .paramOptions = {.type = type, .delimiter = delimiter}},
      .helpText = helpText,
      .val = std::any()};

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
  if (m_optionsMap.contains(argName)) {
    Option& option = *m_optionsMap[argName];
    option.func = func;
  } else {
    std::cerr << "Clapar error: Tried assigning function to option " << argName
              << ", but no option with that name exists. Exiting!" << std::endl;
    exit(CLAPAR_ERR_NO_SUCH_ARGUMENT);
  }
}

void Clapar::addHelpText(const std::string& argName,
                         const std::string& helpText) {
  if (m_optionsMap.contains(argName)) {
    m_optionsMap[argName]->helpText = helpText;
  } else {
    std::cerr << "Clapar error: Tried adding help text to option " << argName
              << ", but no option with that name exists. Exiting!" << std::endl;
    exit(CLAPAR_ERR_NO_SUCH_ARGUMENT);
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
    oss << "\t" << (!isLongEmpty ? optionPtr->namePair.nameLong : "")
        << (isShortEmpty || isLongEmpty ? "" : ", ")
        << (!isShortEmpty ? optionPtr->namePair.nameShort : "")
        << (isOptionNameTextEmpty ? "" : ":\t" + optionPtr->helpText)
        << std::endl;
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
      (*userStream) << ERRMSG_NO_NAME_GIVEN << "\n";
      exit(CLAPAR_ERR_NO_NAME_GIVEN);
      break;

    case OptionNamePair::CollisionWithOther:
      (*userStream) << ERRMSG_NAME_COLLISION_1 << optionPtr->namePair.nameShort
                    << "\" or \"" << optionPtr->namePair.nameLong
                    << ERRMSG_NAME_COLLISION_2 << "\n";
      exit(CLAPAR_ERR_NAME_COLLISION);
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
      return;
    }
  }

  {
    const bool isLastArg = argIdx + 1 >= args.size();
    if (isLastArg) {
      std::cerr << "Clapar error: Wrong usage: option "
                << (option.namePair.nameLong != "" ? option.namePair.nameShort
                                                   : option.namePair.nameLong)
                << " needs a passed value but has none. Exiting!" << std::endl;
      exit(CLAPAR_ERR_WRONG_USAGE);
    }
  }

  const std::string& passedValue = args[argIdx + 1];

  if (m_optionsMap.contains(passedValue)) {
    std::cerr << "Clapar error: Passed the name of an option (" << passedValue
              << ") as a parameter to another option. This is probably not "
                 "what you want. Exiting!"
              << std::endl;
    exit(CLAPAR_ERR_WRONG_USAGE);
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
  } else {
    std::cerr << "Clapar warning: Could not convert passed value ("
              << passedValue << ") to std::any." << std::endl;
  }

  option.func();
}
