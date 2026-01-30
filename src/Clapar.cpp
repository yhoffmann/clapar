#include "../include/Clapar.hpp"

const std::type_index Clapar::Type::BOOL = typeid(bool);
const std::type_index Clapar::Type::STRING = typeid(std::string);
const std::type_index Clapar::Type::FLOAT = typeid(float);
const std::type_index Clapar::Type::DOUBLE = typeid(double);

Clapar::OptionNamePair::Validity Clapar::OptionNamePair::getValidity(
    const std::unordered_map<std::string, std::shared_ptr<Option>> options)
    const {
  for (const auto& [argName, option] : options) {
    const bool doesNameExist = option->namePair.nameShort == nameShort ||
                               option->namePair.nameShort == nameLong ||
                               option->namePair.nameLong == nameShort ||
                               option->namePair.nameLong == nameLong ||
                               nameShort == nameLong;
    if (doesNameExist) {
      return CollisionWithOther;
    }
  }

  if ("" == nameShort && "" == nameLong) {
    return NoneGiven;
  } else if ("" == nameShort) {
    return LGiven;
  } else if ("" == nameLong) {
    return SGiven;
  } else {
    return BothGiven;
  }
}

void Clapar::addFlag(const Clapar::OptionNamePair& namePair,
                     const std::vector<std::string>& beforeThese,
                     const std::vector<std::string>& afterThese) {
  Clapar::Option option = {
      .namePair = namePair,
      .config = {.isFlag = true,
                 .beforeThese = beforeThese,
                 .afterThese = afterThese,
                 .paramOptions = {.type = Clapar::Type::BOOL,
                                  .vectorOptions = {}}},
      .val = std::any()};

  checkAndAdd(option);
}

void Clapar::addFlag(const char* nameShort,
                     const std::vector<std::string>& beforeThese,
                     const std::vector<std::string>& afterThese) {
  addFlag({nameShort, ""}, beforeThese, afterThese);
}

void Clapar::addOption(const Clapar::OptionNamePair& namePair,
                       const std::type_index type, const std::size_t numArgs,
                       const char vectorArgsDelimiter,
                       const std::vector<std::string>& beforeThese,
                       const std::vector<std::string>& afterThese) {
  Clapar::Option option = {
      .namePair = namePair,
      .config = {.isFlag = false,
                 .beforeThese = beforeThese,
                 .afterThese = afterThese,
                 .paramOptions = {.type = type,
                                  .vectorOptions = {.numArgs = numArgs,
                                                    .delimiter =
                                                        vectorArgsDelimiter}}},
      .val = std::any()};

  checkAndAdd(option);
}

void Clapar::addOption(const char* nameShort, const std::type_index type,
                       const std::size_t numArgs,
                       const char vectorArgsDelimiter,
                       const std::vector<std::string>& beforeThese,
                       const std::vector<std::string>& afterThese) {
  addOption({nameShort, ""}, type, numArgs, vectorArgsDelimiter, beforeThese,
            afterThese);
}

void Clapar::addFunction(const std::string& argName,
                         const std::function<void(void)> func) {
  if (m_options.contains(argName)) {
    Option& option = *m_options[argName];
    option.func = func;
  } else {
    std::cerr << "Clapar error: Tried assigning function to argument that does "
                 "not exist. Exiting!"
              << std::endl;
    exit(CLAPAR_ERR_NO_SUCH_ARGUMENT);
  }
}

void Clapar::parse(int argc, char* argv[]) {
  m_argv0 = argv[0];
  const std::vector<std::string> args = [argc, argv]() {
    std::vector<std::string> temp;
    for (int argIdx = 1; argIdx < argc; argIdx++)
      temp.emplace_back(argv[argIdx]);
    return temp;
  }();

  processArgs(args);
}

void Clapar::checkAndAdd(Option& option) {
  Clapar::OptionNamePair::Validity validity =
      option.namePair.getValidity(m_options);

  std::shared_ptr<Option> optionPtr =
      std::make_shared<Option>(std::move(option));

  std::string& nameShort = optionPtr->namePair.nameShort;
  std::string& nameLong = optionPtr->namePair.nameLong;
  switch (validity) {
    case OptionNamePair::SGiven:
      nameLong = optionPtr->namePair.nameShort + "Clapar";
      break;

    case OptionNamePair::BothGiven:
      break;

    case OptionNamePair::LGiven:
      nameShort = optionPtr->namePair.nameLong + "Clapar";
      break;

    case OptionNamePair::NoneGiven:
      (*userStream) << ERRMSG_NO_NAME_GIVEN << "\n";
      exit(CLAPAR_ERR_NO_NAME_GIVEN);
      break;

    case OptionNamePair::CollisionWithOther:
      (*userStream) << ERRMSG_NAME_COLLISION_1 << optionPtr->namePair.nameShort
                    << "\" and \"" << optionPtr->namePair.nameLong
                    << ERRMSG_NAME_COLLISION_2 << "\n";
      exit(CLAPAR_ERR_NAME_COLLISION);
      break;
  }
  m_options[nameShort] = optionPtr;
  m_options[nameLong] = optionPtr;
}

void Clapar::processArgs(const std::vector<std::string>& args) {
  for (std::size_t argIdx = 0, numArgs = args.size(); argIdx < numArgs;
       argIdx++) {
    const std::string& arg = args[argIdx];
    for (auto& [key, option] : m_options) {
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

void Clapar::processOption(std::size_t& argIdx,
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

  if (m_options.contains(passedValue)) {
    std::cerr << "Clapar error: passed the name of an option (" << passedValue
              << ") as a parameter to another option. This is probably not "
                 "what you want. Exiting!"
              << std::endl;
    exit(CLAPAR_ERR_WRONG_USAGE);
  }

  const std::type_index type = option.config.paramOptions.type;
  if (type == Type::STRING) {
    option.val = convertToAny<std::string>(passedValue);
  } else if (type == Type::BOOL) {
    option.val = convertToAny<bool>(passedValue);
  } else if (type == Type::FLOAT) {
    option.val = convertToAny<float>(passedValue);
  } else if (type == Type::DOUBLE) {
    option.val = convertToAny<double>(passedValue);
  } else {
    std::cerr << "Could not convert to std::any" << std::endl;
  }
  option.func();
}
