#include "../include/Clapar.hpp"

const std::type_index Clapar::Type::BOOL = typeid(bool);
const std::type_index Clapar::Type::STRING = typeid(std::string);
const std::type_index Clapar::Type::FLOAT = typeid(float);
const std::type_index Clapar::Type::DOUBLE = typeid(double);

Clapar::OptionNamePair::Validity Clapar::OptionNamePair::getValidity(
    const std::unordered_map<std::string, std::shared_ptr<Option>> options)
    const {
  for (const auto& [argName, option] : options) {
    if (option->namePair.nameShort == nameShort ||
        option->namePair.nameShort == nameLong ||
        option->namePair.nameLong == nameShort ||
        option->namePair.nameLong == nameLong || nameShort == nameLong) {
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
  for (auto& [key, option] : m_options) {
    if (option->wasProcessed) continue;
    option->wasProcessed = true;

    for (std::size_t argIdx = 0, numArgs = args.size(); argIdx < numArgs;
         argIdx++) {
      const std::string& arg = args[argIdx];
      const bool isArg = (arg == option->namePair.nameShort ||
                          arg == option->namePair.nameLong);
      if (isArg) {
        processOption(argIdx, args, *option);
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
  if (option.config.isFlag) {
    option.val = std::any(true);
    return;
  }

  if (argIdx + 1 >= args.size()) {
    std::cerr << "Clapar error: Wrong usage: option "
              << (option.namePair.nameLong != "" ? option.namePair.nameShort
                                                 : option.namePair.nameLong)
              << " needs a passed value but has none. Exiting!" << std::endl;
    exit(CLAPAR_ERROR_WRONG_USAGE);
  }
  const std::string& passedValue = args[argIdx + 1];

  std::istringstream ss(passedValue);
  const std::type_index type = option.config.paramOptions.type;
  if (type == Type::STRING) {
    option.val = convertToAny<std::string>(ss);
  } else if (type == Type::BOOL) {
    option.val = convertToAny<bool>(ss);
  } else if (type == Type::FLOAT) {
    option.val = convertToAny<float>(ss);
  } else if (type == Type::DOUBLE) {
    option.val = convertToAny<double>(ss);
  } else {
    std::cerr << "Could not convert to std::any" << std::endl;
  }
}
