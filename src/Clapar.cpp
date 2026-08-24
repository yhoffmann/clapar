#include "../include/Clapar.hpp"

#include <algorithm>
#include <set>

Clapar::EntryNamePair::Validity Clapar::EntryNamePair::getValidity(
    const std::vector<std::shared_ptr<EntryInternal>>& entries) const {
  const bool isNameHelp = nameShort == "--help" || nameLong == "--help";
  for (const auto& entry : entries) {
    const bool doesNameExist = [&entry, this]() -> bool {
      const Clapar::EntryNamePair& otherNamePair = entry->namePair;
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

void Clapar::addFlag(const Clapar::EntryNamePair& namePair,
                     const std::string& helpText) {
  Clapar::EntryInternal entry = {
      namePair,
      {
          true,
          {Clapar::Type::BOOL},
      },
      helpText,
      std::any(),
  };

  checkAndAdd(entry);
}

void Clapar::addFlag(const char* nameShort, const std::string& helpText) {
  addFlag({nameShort, ""}, helpText);
}

void Clapar::addOption(const Clapar::EntryNamePair& namePair,
                       const std::type_index type,
                       const std::string& helpText) {
  Clapar::EntryInternal entry = {
      namePair,
      {
          false,
          {type},
      },
      helpText,
      std::any(),
  };

  checkAndAdd(entry);
}

void Clapar::addOption(const char* nameShort, const std::type_index type,
                       const std::string& helpText) {
  addOption({nameShort, ""}, type, helpText);
}

void Clapar::addVectorOption(const EntryNamePair& namePair,
                             const std::type_index type, const char delimiter,
                             const std::string& helpText) {
  Clapar::EntryInternal entry = {
      namePair,
      {
          false,
          {type, delimiter},
      },
      helpText,
      std::any(),
  };

  checkAndAdd(entry);
}

void Clapar::addVectorOption(const char* nameShort, const std::type_index type,
                             const char delimiter,
                             const std::string& helpText) {
  addVectorOption({nameShort, ""}, type, delimiter, helpText);
}

void Clapar::addEntry(const Entry& entry) {
  Clapar::EntryInternal entryInternal = {
      entry.namePair,
      {
          entry.isFlag,
          {entry.type, entry.delimiter},

      },
      entry.helpText,
      entry.isFlag ? std::any() : std::any{entry.defaultVal},
  };
  checkAndAdd(entryInternal);

  addFunction(entry.namePair.string(), entry.function);
}

void Clapar::setDefault(const std::string& argName, const std::any defaultVal) {
  auto entry = m_entriesMap[argName];
  entry->val = defaultVal;
  entry->isDefault = true;
}

void Clapar::addFunction(const std::string& argName,
                         const std::function<void(void)> func) {
  if (m_entriesMap.find(argName) != m_entriesMap.end()) {
    EntryInternal& entry = *m_entriesMap[argName];
    entry.func = func;
  } else {
    error(ErrNoSuchArgument,
          "Tried assigning function to entry \"" + argName +
              "\", but no entry with the name exists. Exiting!");
  }
}

void Clapar::addHelpText(const std::string& argName,
                         const std::string& helpText) {
  if (m_entriesMap.find(argName) != m_entriesMap.end()) {
    m_entriesMap[argName]->helpText = helpText;
  } else {
    error(ErrNoSuchArgument,
          "Tried assigning help text to entry \"" + argName +
              "\", but no entry with that names exists. Exiting!");
  }
}

void Clapar::setProgramDescription(const std::string& description) {
  m_description = description;
}

void Clapar::shouldExitAtUnknownArg(const bool b) {
  m_shouldExitAtUnknownArg = b;
}

void Clapar::shouldExitIfParamIsSameAsAnyArg(const bool b) {
  m_shouldExitIfParamIsSameAsAnyArg = b;
}

void Clapar::parse(const int argc, const char* const argv[]) {
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
  std::vector<std::shared_ptr<EntryInternal>> entriesTemp(m_entries);
  std::sort(entriesTemp.begin(), entriesTemp.end(),
            [](const std::shared_ptr<EntryInternal> left,
               const std::shared_ptr<EntryInternal> right) -> bool {
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
  for (const auto& entryPtr : entriesTemp) {
    const bool isShortEmpty = entryPtr->namePair.nameShort.empty();
    const bool isLongEmpty = entryPtr->namePair.nameLong.empty();
    const bool isentryNameTextEmpty = entryPtr->helpText.empty();
    const bool isLastEntry = entryPtr.get() == entriesTemp.back().get();
    const std::string type = getTypeName(entryPtr->config.paramConfig.type);

    oss << setBold << "  " << (!isLongEmpty ? entryPtr->namePair.nameLong : "")
        << unsetBold << (isShortEmpty || isLongEmpty ? "" : ", ") << setBold
        << (!isShortEmpty ? entryPtr->namePair.nameShort : "") << unsetBold
        << (!entryPtr->config.isFlag
                ? " <" + type +
                      (entryPtr->config.paramConfig.delimiter.has_value()
                           ? ",...>"
                           : ">")
                : "")
        << (isentryNameTextEmpty ? "" : ":\n\t" + entryPtr->helpText)
        << (!isLastEntry ? "\n" : "");
  }

  return oss.str();
}

Clapar::Clapar(const std::string& description) : m_description(description) {}

bool Clapar::getFlag(const std::string& flagName) const {
  return get<bool>(flagName).value_or(false);
}

std::string Clapar::argv0() const { return m_argv0; }

void Clapar::checkAndAdd(EntryInternal& entry) {
  const Clapar::EntryNamePair::Validity validity =
      entry.namePair.getValidity(m_entries);

  m_entries.push_back(std::make_shared<EntryInternal>(entry));
  std::shared_ptr<EntryInternal> entryPtr = m_entries.back();

  std::string& nameShort = entryPtr->namePair.nameShort;
  std::string& nameLong = entryPtr->namePair.nameLong;
  switch (validity) {
    case EntryNamePair::SGiven:
      m_entriesMap[nameShort] = entryPtr;
      break;

    case EntryNamePair::BothGiven:
      m_entriesMap[nameShort] = entryPtr;
      m_entriesMap[nameLong] = entryPtr;
      break;

    case EntryNamePair::LGiven:
      m_entriesMap[nameLong] = entryPtr;
      break;

    case EntryNamePair::NoneGiven:
      error(ErrNoNameGiven, ERRMSG_NO_NAME_GIVEN);
      break;

    case EntryNamePair::CollisionWithOther:
      error(ErrNameCollision, ERRMSG_NAME_COLLISION_1 +
                                  entryPtr->namePair.nameShort + "\" or \"" +
                                  entryPtr->namePair.nameLong +
                                  ERRMSG_NAME_COLLISION_2);
      break;
  }
}

void Clapar::processArgs(const std::vector<std::string>& args) {
  bool isParam = false;
  std::set<std::string> processedArgs;

  for (std::size_t argIdx = 0, numArgs = args.size(); argIdx < numArgs;
       argIdx++) {
    if (isParam) {
      isParam = false;
      continue;
    }

    const std::string& arg = args[argIdx];

    if (m_shouldExitAtUnknownArg &&
        processedArgs.find(arg) != processedArgs.end())
      error(ErrWrongUsage,
            "Found entry \"" + arg +
                "\" twice and program is setup to exit if args show up "
                "twice. Exiting!");

    bool isArgKnown = false;
    for (auto& [argNameToCheck, entry] : m_entriesMap) {
      if (m_shouldExitAtUnknownArg && entry->wasProcessed) continue;

      if (arg == argNameToCheck) {
        processEntry(argIdx, args, *entry, isParam);
        entry->wasProcessed = true;
        processedArgs.insert(entry->namePair.nameShort);
        processedArgs.insert(entry->namePair.nameLong);
        isArgKnown = true;
      }
    }

    if (m_shouldExitAtUnknownArg && !isArgKnown)
      error(ErrNoSuchArgument,
            "The provided arg \"" + arg +
                "\" is not known and the program is set up to exit if an "
                "unknown arg is found. Exiting!");
  }
}

void Clapar::processFlag(EntryInternal& entry) {
  entry.val = std::any(true);
  entry.isDefault = false;
  entry.func();
}

void Clapar::processEntry(const std::size_t argIdx,
                          const std::vector<std::string>& args,
                          EntryInternal& entry, bool& isParam) {
  if (entry.config.isFlag)
    return processFlag(entry);
  else {
    const bool hasNoValue = argIdx + 1 >= args.size();
    if (hasNoValue)
      error(ErrWrongUsage, "Wrong usage: entry " + entry.namePair.string() +
                               " needs a passed value but has none. Exiting!");
  }
  isParam = true;

  const std::string& passedValue = args[argIdx + 1];

  if (m_shouldExitIfParamIsSameAsAnyArg &&
      m_entriesMap.find(passedValue) != m_entriesMap.end()) {
    error(ErrWrongUsage, "Passed the name of entry \"" + passedValue +
                             "\" as a parameter to another entry. This is "
                             "probably not what you want. Exiting!");
  }

  std::optional<const char> delimiter = entry.config.paramConfig.delimiter;
  const std::type_index type = entry.config.paramConfig.type;
  if (type == Type::STRING) {
    entry.val = convertToAny<std::string>(passedValue, delimiter);
  } else if (type == Type::BOOL) {
    entry.val = convertToAny<bool>(passedValue, delimiter);
  } else if (type == Type::FLOAT) {
    entry.val = convertToAny<float>(passedValue, delimiter);
  } else if (type == Type::DOUBLE) {
    entry.val = convertToAny<double>(passedValue, delimiter);
  } else if (type == Type::INT) {
    entry.val = convertToAny<int>(passedValue, delimiter);
  } else if (type == Type::UNSIGNED) {
    entry.val = convertToAny<std::size_t>(passedValue, delimiter);
  } else if (type == Type::CHAR) {
    entry.val = convertToAny<char>(passedValue, delimiter);
  } else {
    std::cerr << "Clapar warning: Could not convert passed value ("
              << passedValue << ") to std::any." << std::endl;
  }

  entry.isDefault = false;
  entry.func();
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
