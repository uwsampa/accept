#include "accept.h"

using namespace llvm;

void Description::operator=(const Description &d) {
  text = d.text;
  blockers = d.blockers;
  stream = NULL;
}

Description::~Description() {
  if (stream)
    delete stream;
}

bool Description::operator==(const Description &rhs) {
  return (this->text == rhs.text) &&
      (this->blockers == rhs.blockers);
}

Description::operator llvm::raw_ostream &() {
  if (!stream) {
    stream = new llvm::raw_string_ostream(text);
  }
  return *stream;
}

std::string &Description::getText() {
  if (stream)
    stream->flush();
  return text;
}

void Description::blocker(int lineno, llvm::StringRef s) {
  blockers[lineno].push_back(s);
}

void Description::operator<<(const llvm::Instruction *inst) {
  const llvm::Module *mod = inst->getParent()->getParent()->getParent();
  blocker(
    inst->getDebugLoc().getLine(),
    instDesc(*mod, inst)
  );
}

Description *ApproxInfo::logAdd(llvm::StringRef kind,
      StringRef filename, const int lineno) {
  // TODO return NULL if the log is disabled
  Description::Location loc(kind, filename, lineno);
  std::vector<Description*> &descs = logDescs[loc];
  Description *desc = new Description();
  descs.push_back(desc);
  return desc;
}

Description *ApproxInfo::logAdd(llvm::StringRef kind,
    llvm::Instruction *where) {
  DebugLoc dl = where->getDebugLoc();
  return logAdd(
    kind,
    getFilename(*where->getParent()->getParent()->getParent(), dl),
    dl.getLine()
  );
}

void ApproxInfo::dumpLog() {
  int numKind = 0;
  std::string prevKind;

  // For each location, print all the descriptions to the log.
  for (std::map<Description::Location, std::vector<Description*>, Description::cmpLocation>::iterator
      i = logDescs.begin(); i != logDescs.end(); i++) {
    std::string newKind = i->first.kind;
    if (newKind != prevKind) {
      if (numKind != 0) {
        *logFile << "\n\n";
      }
      numKind++;

      // Descriptions are organized into sections according to their kinds.
      // Include a header if the description is of a different kind than the
      // previous description.
      if (newKind == "Function") {
        *logFile << "ANALYZING FUNCTIONS FOR PRECISE-PURITY:\n";
      } else if (newKind == "Loop") {
        *logFile << "ANALYZING LOOP BODIES FOR PERFORABILITY:\n";
      } else if (newKind == "NPU Region") {
        *logFile << "ANALYZING NPU REGIONS FOR PRECISE-PURITY:\n";
      } else if (newKind == "Synchronization") {
        *logFile << "ANALYZING CRITICAL SECTIONS FOR PRECISE-PURITY:\n";
      } else {
        for (size_t ch = 0; ch < newKind.length(); ch++) {
          newKind[ch] = toupper(newKind[ch]);
        }
        *logFile << "ANALYZING " << newKind << "S FOR PRECISE-PURITY:\n";
      }
    }

    prevKind = newKind;

    std::vector<Description*> descVector = i->second;
    for (std::vector<Description*>::iterator j = descVector.begin();
        j != descVector.end(); j++) {
      // Within the section for a kind, descriptions with blockers are
      // printed before descriptions with no blockers, in order to help
      // the programmer identify any possible annotations that remain.
      // Five additional dashes are included for the demarcation between
      // the last description with blockers and the first description
      // without blockers within a section.
      Description *desc = *j;
      *logFile << "-----\n" << desc->getText();

      std::map< int, std::vector<std::string> > blockers = desc->blockers;
      for (std::map< int, std::vector<std::string> >::iterator
          k = blockers.begin(); k != blockers.end(); k++) {
        std::vector<std::string> entryVector = k->second;
        for (std::vector<std::string>::iterator l = entryVector.begin();
            l != entryVector.end(); l++) {
          *logFile << "  * " << *l << "\n";
        }
      }

      // Free the description. We're done.
      delete desc;
    }
  }
}

