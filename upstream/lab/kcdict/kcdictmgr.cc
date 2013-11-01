/*************************************************************************************************
 * The command line utility of the word dictionary
 *                                                               Copyright (C) 2009-2011 FAL Labs
 * This file is part of Kyoto Cabinet.
 * This program is free software: you can redistribute it and/or modify it under the terms of
 * the GNU General Public License as published by the Free Software Foundation, either version
 * 3 of the License, or any later version.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 *************************************************************************************************/


#include <kcutil.h>
#include <kcpolydb.h>
#include <kcdbext.h>

namespace kc = kyotocabinet;


// global variables
const char* g_progname;                  // program name


// function prototypes
int main(int argc, char** argv);
static void usage();
static void dberrprint(kc::BasicDB* db, const char* info);
static void normalizequery(const std::string& query, std::string* dest);
static int32_t runimport(int argc, char** argv);
static int32_t runsearch(int argc, char** argv);
static int32_t procimport(const char* path, const char* srcpath);
static int32_t procsearch(const char* path, const char* query, int64_t max, int32_t mode);


// structure for sorting indexed records
struct IndexedRecord {
  int64_t rank;
  std::string text;
  bool operator <(const IndexedRecord& right) const {
    if (rank != right.rank) return rank < right.rank;
    return text < right.text;
  }
};


// structure for sorting ambiguous records
struct AmbiguousRecord {
  uint32_t ed;
  std::string text;
  bool operator <(const AmbiguousRecord& right) const {
    if (ed != right.ed) return ed < right.ed;
    return text < right.text;
  }
};


// structure for sorting plain records
struct PlainRecord {
  std::string key;
  std::string text;
  bool operator <(const PlainRecord& right) const {
    if (key != right.key) return key < right.key;
    return text < right.text;
  }
};


// main routine
int main(int argc, char** argv) {
  g_progname = argv[0];
  kc::setstdiobin();
  if (argc < 2) usage();
  int32_t rv = 0;
  if (!std::strcmp(argv[1], "import")) {
    rv = runimport(argc, argv);
  } else if (!std::strcmp(argv[1], "search")) {
    rv = runsearch(argc, argv);
  } else {
    usage();
  }
  return rv;
}


// print the usage and exit
static void usage() {
  std::cerr << g_progname << ": the command line utility of the word dictionary" << std::endl;
  std::cerr << std::endl;
  std::cerr << "  " << g_progname << " import path src" << std::endl;
  std::cerr << "  " << g_progname << " search [-max num] [-f|-a|-m|-r|-tm|-tr] [-iu]"
      " path query" << std::endl;
  std::cerr << std::endl;
  std::exit(1);
}


// print error message of database
static void dberrprint(kc::BasicDB* db, const char* info) {
  const kc::BasicDB::Error& err = db->error();
  std::cerr << g_progname << ": " << info << ": " << db->path().c_str() << ": " << err.code() <<
      ": " << err.name() << ": " << err.message() << std::endl;
}


// normalize a query
static void normalizequery(const std::string& query, std::string* dest) {
  bool lowmode = true;
  bool nacmode = true;
  bool spcmode = true;
  std::vector<uint32_t> ucs;
  kc::strutftoucs(query, &ucs);
  size_t onum = ucs.size();
  size_t nnum = 0;
  for (size_t i = 0; i < onum; i++) {
    uint32_t c = ucs[i];
    if (c >= 0x10000) {
      ucs[nnum++] = c;
      continue;
    }
    uint32_t high = c >> 8;
    if (high == 0x00) {
      if (c < 0x0020 || c == 0x007f) {
        // control characters
        if (spcmode) {
          ucs[nnum++] = 0x0020;
        } else if (c == 0x0009 || c == 0x000a || c == 0x000d) {
          ucs[nnum++] = c;
        } else {
          ucs[nnum++] = 0x0020;
        }
      } else if (c == 0x00a0) {
        // no-break space
        ucs[nnum++] = 0x0020;
      } else {
        // otherwise
        if (lowmode) {
          if (c < 0x007f) {
            if (c >= 0x0041 && c <= 0x005a) c += 0x20;
          } else if (c >= 0x00c0 && c <= 0x00de && c != 0x00d7) {
            c += 0x20;
          }
        }
        if (nacmode) {
          if (c >= 0x00c0 && c <= 0x00c5) {
            c = 'A';
          } else if (c == 0x00c7) {
            c = 'C';
          } if (c >= 0x00c7 && c <= 0x00cb) {
            c = 'E';
          } if (c >= 0x00cc && c <= 0x00cf) {
            c = 'I';
          } else if (c == 0x00d0) {
            c = 'D';
          } else if (c == 0x00d1) {
            c = 'N';
          } if ((c >= 0x00d2 && c <= 0x00d6) || c == 0x00d8) {
            c = 'O';
          } if (c >= 0x00d9 && c <= 0x00dc) {
            c = 'U';
          } if (c == 0x00dd || c == 0x00de) {
            c = 'Y';
          } else if (c == 0x00df) {
            c = 's';
          } else if (c >= 0x00e0 && c <= 0x00e5) {
            c = 'a';
          } else if (c == 0x00e7) {
            c = 'c';
          } if (c >= 0x00e7 && c <= 0x00eb) {
            c = 'e';
          } if (c >= 0x00ec && c <= 0x00ef) {
            c = 'i';
          } else if (c == 0x00f0) {
            c = 'd';
          } else if (c == 0x00f1) {
            c = 'n';
          } if ((c >= 0x00f2 && c <= 0x00f6) || c == 0x00f8) {
            c = 'o';
          } if (c >= 0x00f9 && c <= 0x00fc) {
            c = 'u';
          } if (c >= 0x00fd && c <= 0x00ff) {
            c = 'y';
          }
        }
        ucs[nnum++] = c;
      }
    } else if (high == 0x01) {
      // latin-1 extended
      if (lowmode) {
        if (c <= 0x0137) {
          if ((c & 1) == 0) c++;
        } else if (c == 0x0138) {
          c += 0;
        } else if (c <= 0x0148) {
          if ((c & 1) == 1) c++;
        } else if (c == 0x0149) {
          c += 0;
        } else if (c <= 0x0177) {
          if ((c & 1) == 0) c++;
        } else if (c == 0x0178) {
          c = 0x00ff;
        } else if (c <= 0x017e) {
          if ((c & 1) == 1) c++;
        } else if (c == 0x017f) {
          c += 0;
        }
      }
      if (nacmode) {
        if (c == 0x00ff) {
          c = 'y';
        } else if (c <= 0x0105) {
          c = ((c & 1) == 0) ? 'A' : 'a';
        } else if (c <= 0x010d) {
          c = ((c & 1) == 0) ? 'C' : 'c';
        } else if (c <= 0x0111) {
          c = ((c & 1) == 0) ? 'D' : 'd';
        } else if (c <= 0x011b) {
          c = ((c & 1) == 0) ? 'E' : 'e';
        } else if (c <= 0x0123) {
          c = ((c & 1) == 0) ? 'G' : 'g';
        } else if (c <= 0x0127) {
          c = ((c & 1) == 0) ? 'H' : 'h';
        } else if (c <= 0x0131) {
          c = ((c & 1) == 0) ? 'I' : 'i';
        } else if (c == 0x0134) {
          c = 'J';
        } else if (c == 0x0135) {
          c = 'j';
        } else if (c == 0x0136) {
          c = 'K';
        } else if (c == 0x0137) {
          c = 'k';
        } else if (c == 0x0138) {
          c = 'k';
        } else if (c >= 0x0139 && c <= 0x0142) {
          c = ((c & 1) == 1) ? 'L' : 'l';
        } else if (c >= 0x0143 && c <= 0x0148) {
          c = ((c & 1) == 1) ? 'N' : 'n';
        } else if (c >= 0x0149 && c <= 0x014b) {
          c = ((c & 1) == 0) ? 'N' : 'n';
        } else if (c >= 0x014c && c <= 0x0151) {
          c = ((c & 1) == 0) ? 'O' : 'o';
        } else if (c >= 0x0154 && c <= 0x0159) {
          c = ((c & 1) == 0) ? 'R' : 'r';
        } else if (c >= 0x015a && c <= 0x0161) {
          c = ((c & 1) == 0) ? 'S' : 's';
        } else if (c >= 0x0162 && c <= 0x0167) {
          c = ((c & 1) == 0) ? 'T' : 't';
        } else if (c >= 0x0168 && c <= 0x0173) {
          c = ((c & 1) == 0) ? 'U' : 'u';
        } else if (c == 0x0174) {
          c = 'W';
        } else if (c == 0x0175) {
          c = 'w';
        } else if (c == 0x0176) {
          c = 'Y';
        } else if (c == 0x0177) {
          c = 'y';
        } else if (c == 0x0178) {
          c = 'Y';
        } else if (c >= 0x0179 && c <= 0x017e) {
          c = ((c & 1) == 1) ? 'Z' : 'z';
        } else if (c == 0x017f) {
          c = 's';
        }
      }
      ucs[nnum++] = c;
    } else if (high == 0x03) {
      // greek
      if (lowmode) {
        if (c >= 0x0391 && c <= 0x03a9) {
          c += 0x20;
        } else if (c >= 0x03d8 && c <= 0x03ef) {
          if ((c & 1) == 0) c++;
        } else if (c == 0x0374 || c == 0x03f7 || c == 0x03fa) {
          c++;
        }
      }
      ucs[nnum++] = c;
    } else if (high == 0x04) {
      // cyrillic
      if (lowmode) {
        if (c <= 0x040f) {
          c += 0x50;
        } else if (c <= 0x042f) {
          c += 0x20;
        } else if (c >= 0x0460 && c <= 0x0481) {
          if ((c & 1) == 0) c++;
        } else if (c >= 0x048a && c <= 0x04bf) {
          if ((c & 1) == 0) c++;
        } else if (c == 0x04c0) {
          c = 0x04cf;
        } else if (c >= 0x04c1 && c <= 0x04ce) {
          if ((c & 1) == 1) c++;
        } else if (c >= 0x04d0) {
          if ((c & 1) == 0) c++;
        }
      }
      ucs[nnum++] = c;
    } else if (high == 0x20) {
      if (c == 0x2002) {
        // en space
        ucs[nnum++] = 0x0020;
      } else if (c == 0x2003) {
        // em space
        ucs[nnum++] = 0x0020;
      } else if (c == 0x2009) {
        // thin space
        ucs[nnum++] = 0x0020;
      } else if (c == 0x2010) {
        // hyphen
        ucs[nnum++] = 0x002d;
      } else if (c == 0x2015) {
        // fullwidth horizontal line
        ucs[nnum++] = 0x002d;
      } else if (c == 0x2019) {
        // apostrophe
        ucs[nnum++] = 0x0027;
      } else if (c == 0x2033) {
        // double quotes
        ucs[nnum++] = 0x0022;
      } else {
        // (otherwise)
        ucs[nnum++] = c;
      }
    } else if (high == 0x22) {
      if (c == 0x2212) {
        // minus sign
        ucs[nnum++] = 0x002d;
      } else {
        // (otherwise)
        ucs[nnum++] = c;
      }
    } else if (high == 0x30) {
      if (c == 0x3000) {
        // fullwidth space
        if (spcmode) {
          ucs[nnum++] = 0x0020;
        } else {
          ucs[nnum++] = c;
        }
      } else {
        // (otherwise)
        ucs[nnum++] = c;
      }
    } else if (high == 0xff) {
      if (c == 0xff01) {
        // fullwidth exclamation
        ucs[nnum++] = 0x0021;
      } else if (c == 0xff03) {
        // fullwidth igeta
        ucs[nnum++] = 0x0023;
      } else if (c == 0xff04) {
        // fullwidth dollar
        ucs[nnum++] = 0x0024;
      } else if (c == 0xff05) {
        // fullwidth parcent
        ucs[nnum++] = 0x0025;
      } else if (c == 0xff06) {
        // fullwidth ampersand
        ucs[nnum++] = 0x0026;
      } else if (c == 0xff0a) {
        // fullwidth asterisk
        ucs[nnum++] = 0x002a;
      } else if (c == 0xff0b) {
        // fullwidth plus
        ucs[nnum++] = 0x002b;
      } else if (c == 0xff0c) {
        // fullwidth comma
        ucs[nnum++] = 0x002c;
      } else if (c == 0xff0e) {
        // fullwidth period
        ucs[nnum++] = 0x002e;
      } else if (c == 0xff0f) {
        // fullwidth slash
        ucs[nnum++] = 0x002f;
      } else if (c == 0xff1a) {
        // fullwidth colon
        ucs[nnum++] = 0x003a;
      } else if (c == 0xff1b) {
        // fullwidth semicolon
        ucs[nnum++] = 0x003b;
      } else if (c == 0xff1d) {
        // fullwidth equal
        ucs[nnum++] = 0x003d;
      } else if (c == 0xff1f) {
        // fullwidth question
        ucs[nnum++] = 0x003f;
      } else if (c == 0xff20) {
        // fullwidth atmark
        ucs[nnum++] = 0x0040;
      } else if (c == 0xff3c) {
        // fullwidth backslash
        ucs[nnum++] = 0x005c;
      } else if (c == 0xff3e) {
        // fullwidth circumflex
        ucs[nnum++] = 0x005e;
      } else if (c == 0xff3f) {
        // fullwidth underscore
        ucs[nnum++] = 0x005f;
      } else if (c == 0xff5c) {
        // fullwidth vertical line
        ucs[nnum++] = 0x007c;
      } else if (c >= 0xff21 && c <= 0xff3a) {
        // fullwidth alphabets
        if (lowmode) {
          c -= 0xfee0;
          if (c >= 0x0041 && c <= 0x005a) c += 0x20;
          ucs[nnum++] = c;
        } else {
          ucs[nnum++] = c - 0xfee0;
        }
      } else if (c >= 0xff41 && c <= 0xff5a) {
        // fullwidth small alphabets
        ucs[nnum++] = c - 0xfee0;
      } else if (c >= 0xff10 && c <= 0xff19) {
        // fullwidth numbers
        ucs[nnum++] = c - 0xfee0;
      } else if (c == 0xff61) {
        // halfwidth full stop
        ucs[nnum++] = 0x3002;
      } else if (c == 0xff62) {
        // halfwidth left corner
        ucs[nnum++] = 0x300c;
      } else if (c == 0xff63) {
        // halfwidth right corner
        ucs[nnum++] = 0x300d;
      } else if (c == 0xff64) {
        // halfwidth comma
        ucs[nnum++] = 0x3001;
      } else if (c == 0xff65) {
        // halfwidth middle dot
        ucs[nnum++] = 0x30fb;
      } else if (c == 0xff66) {
        // halfwidth wo
        ucs[nnum++] = 0x30f2;
      } else if (c >= 0xff67 && c <= 0xff6b) {
        // halfwidth small a-o
        ucs[nnum++] = (c - 0xff67) * 2 + 0x30a1;
      } else if (c >= 0xff6c && c <= 0xff6e) {
        // halfwidth small ya-yo
        ucs[nnum++] = (c - 0xff6c) * 2 + 0x30e3;
      } else if (c == 0xff6f) {
        // halfwidth small tu
        ucs[nnum++] = 0x30c3;
      } else if (c == 0xff70) {
        // halfwidth prolonged mark
        ucs[nnum++] = 0x30fc;
      } else if (c >= 0xff71 && c <= 0xff75) {
        // halfwidth a-o
        uint32_t tc = (c - 0xff71) * 2 + 0x30a2;
        if (c == 0xff73 && i < onum - 1 && ucs[i+1] == 0xff9e) {
          tc = 0x30f4;
          i++;
        }
        ucs[nnum++] = tc;
      } else if (c >= 0xff76 && c <= 0xff7a) {
        // halfwidth ka-ko
        uint32_t tc = (c - 0xff76) * 2 + 0x30ab;
        if (i < onum - 1 && ucs[i+1] == 0xff9e) {
          tc++;
          i++;
        }
        ucs[nnum++] = tc;
      } else if (c >= 0xff7b && c <= 0xff7f) {
        // halfwidth sa-so
        uint32_t tc = (c - 0xff7b) * 2 + 0x30b5;
        if (i < onum - 1 && ucs[i+1] == 0xff9e) {
          tc++;
          i++;
        }
        ucs[nnum++] = tc;
      } else if (c >= 0xff80 && c <= 0xff84) {
        // halfwidth ta-to
        uint32_t tc = (c - 0xff80) * 2 + 0x30bf + (c >= 0xff82 ? 1 : 0);
        if (i < onum - 1 && ucs[i+1] == 0xff9e) {
          tc++;
          i++;
        }
        ucs[nnum++] = tc;
      } else if (c >= 0xff85 && c <= 0xff89) {
        // halfwidth na-no
        ucs[nnum++] = c - 0xcebb;
      } else if (c >= 0xff8a && c <= 0xff8e) {
        // halfwidth ha-ho
        uint32_t tc = (c - 0xff8a) * 3 + 0x30cf;
        if (i < onum - 1) {
          if (ucs[i+1] == 0xff9e) {
            tc++;
            i++;
          } else if (ucs[i+1] == 0xff9f) {
            tc += 2;
            i++;
          }
        }
        ucs[nnum++] = tc;
      } else if (c >= 0xff8f && c <= 0xff93) {
        // halfwidth ma-mo
        ucs[nnum++] = c - 0xceb1;
      } else if (c >= 0xff94 && c <= 0xff96) {
        // halfwidth ya-yo
        ucs[nnum++] = (c - 0xff94) * 2 + 0x30e4;
      } else if (c >= 0xff97 && c <= 0xff9b) {
        // halfwidth ra-ro
        ucs[nnum++] = c - 0xceae;
      } else if (c == 0xff9c) {
        // halfwidth wa
        ucs[nnum++] = 0x30ef;
      } else if (c == 0xff9d) {
        // halfwidth nn
        ucs[nnum++] = 0x30f3;
      } else {
        // otherwise
        ucs[nnum++] = c;
      }
    } else {
      // otherwise
      ucs[nnum++] = c;
    }
  }
  kc::strucstoutf(ucs, dest);
}


// parse arguments of import command
static int32_t runimport(int argc, char** argv) {
  bool argbrk = false;
  const char* path = NULL;
  const char* srcpath = NULL;
  for (int32_t i = 2; i < argc; i++) {
    if (!argbrk && argv[i][0] == '-') {
      if (!std::strcmp(argv[i], "--")) {
        argbrk = true;
      } else {
        usage();
      }
    } else if (!path) {
      argbrk = true;
      path = argv[i];
    } else if (!srcpath) {
      srcpath = argv[i];
    } else {
      usage();
    }
  }
  if (!path || !srcpath) usage();
  int32_t rv = procimport(path, srcpath);
  return rv;
}


// parse arguments of search command
static int32_t runsearch(int argc, char** argv) {
  bool argbrk = false;
  const char* path = NULL;
  const char* query = NULL;
  int64_t max = 10;
  int32_t mode = 0;
  bool iu = false;
  for (int32_t i = 2; i < argc; i++) {
    if (!argbrk && argv[i][0] == '-') {
      if (!std::strcmp(argv[i], "--")) {
        argbrk = true;
      } else if (!std::strcmp(argv[i], "-max")) {
        if (++i >= argc) usage();
        max = kc::atoix(argv[i]);
      } else if (!std::strcmp(argv[i], "-f")) {
        mode = 'f';
      } else if (!std::strcmp(argv[i], "-a")) {
        mode = 'a';
      } else if (!std::strcmp(argv[i], "-m")) {
        mode = 'm';
      } else if (!std::strcmp(argv[i], "-r")) {
        mode = 'r';
      } else if (!std::strcmp(argv[i], "-tm")) {
        mode = 'M';
      } else if (!std::strcmp(argv[i], "-tr")) {
        mode = 'R';
      } else if (!std::strcmp(argv[i], "-iu")) {
        iu = true;
      } else {
        usage();
      }
    } else if (!path) {
      argbrk = true;
      path = argv[i];
    } else if (!query) {
      query = argv[i];
    } else {
      usage();
    }
  }
  if (!path || !query) usage();
  const char* qbuf;
  if (iu) {
    size_t qsiz;
    qbuf = kc::urldecode(query, &qsiz);
    query = qbuf;
  } else {
    qbuf = NULL;
  }
  int32_t rv = procsearch(path, query, max, mode);
  delete[] qbuf;
  return rv;
}


// perform import command
static int32_t procimport(const char* path, const char* srcpath) {
  kc::TextDB srcdb;
  if (!srcdb.open(srcpath, kc::TextDB::OREADER)) {
    dberrprint(&srcdb, "DB::open failed");
    return 1;
  }
  kc::TreeDB destdb;
  destdb.tune_options(kc::TreeDB::TSMALL | kc::TreeDB::TLINEAR | kc::TreeDB::TCOMPRESS);
  if (!destdb.open(path, kc::TreeDB::OWRITER | kc::TreeDB::OCREATE | kc::TreeDB::OTRUNCATE)) {
    dberrprint(&destdb, "DB::open failed");
    return 1;
  }
  bool err = false;
  class MapReduceImpl : public kc::MapReduce {
   public:
    MapReduceImpl(kc::TreeDB* destdb) :
        destdb_(destdb), lock_(), err_(false), mapcnt_(0), redcnt_(0) {}
    bool error() {
      return err_;
    }
   private:
    bool map(const char* kbuf, size_t ksiz, const char* vbuf, size_t vsiz) {
      bool err = false;
      std::vector<std::string> fields;
      kc::strsplit(std::string(vbuf, vsiz), '\t', &fields);
      if (fields.size() >= 5) {
        std::string key;
        normalizequery(fields[0], &key);
        std::string value;
        kc::strprintf(&value, "%s\t%s\t%s\t%s",
                      fields[1].c_str(), fields[2].c_str(),
                      fields[3].c_str(), fields[4].c_str());
        if (!emit(key.data(), key.size(), value.data(), value.size())) err = true;
      }
      int64_t cnt = mapcnt_.add(1) + 1;
      if (cnt % 10000 == 0) {
        std::string message = kc::strprintf("processed %lld entries", (long long)cnt);
        if (!log("map", message.c_str())) err = true;
      }
      return !err;
    }
    bool reduce(const char* kbuf, size_t ksiz, ValueIterator* iter) {
      bool err = false;
      std::vector<IndexedRecord> records;
      const char* vbuf;
      size_t vsiz;
      while ((vbuf = iter->next(&vsiz)) != NULL) {
        std::vector<std::string> fields;
        kc::strsplit(std::string(vbuf, vsiz), '\t', &fields);
        if (fields.size() >= 4) {
          int64_t rank = kc::atoi(fields[0].c_str());
          std::string text;
          kc::strprintf(&text, "%s\t%s\t%s",
                        fields[1].c_str(), fields[2].c_str(), fields[3].c_str());
          IndexedRecord rec = { rank, text };
          records.push_back(rec);
        }
      }
      std::sort(records.begin(), records.end());
      if (records.size() > 1000) records.resize(1000);
      std::vector<IndexedRecord>::iterator rit = records.begin();
      std::vector<IndexedRecord>::iterator ritend = records.end();
      int32_t seq = 0;
      while (rit != ritend) {
        std::string key(kbuf, ksiz);
        kc::strprintf(&key, "\t%03lld", (long long)++seq);
        if (!destdb_->set(key.data(), key.size(), rit->text.data(), rit->text.size())) {
          err = true;
          err_ = true;
        }
        rit++;
      }
      int64_t cnt = redcnt_.add(1) + 1;
      if (cnt % 10000 == 0) {
        std::string message = kc::strprintf("processed %lld entries", (long long)cnt);
        if (!log("reduce", message.c_str())) err = true;
      }
      return !err;
    }
    bool log(const char* name, const char* message) {
      kc::ScopedMutex lock(&lock_);
      std::cout << name << ": " << message << std::endl;
      return true;
    }
   private:
    kc::TreeDB* destdb_;
    kc::Mutex lock_;
    bool err_;
    kc::AtomicInt64 mapcnt_;
    kc::AtomicInt64 redcnt_;
  };
  MapReduceImpl mr(&destdb);
  if (!mr.execute(&srcdb, "", kc::MapReduce::XPARAMAP | kc::MapReduce::XPARAFLS)) {
    dberrprint(&srcdb, "MapReduce::execute failed");
    err = true;
  }
  if (mr.error()) {
    dberrprint(&srcdb, "MapReduce::execute failed");
    err = true;
  }
  if (!destdb.close()) {
    dberrprint(&destdb, "DB::close failed");
    err = true;
  }
  if (!srcdb.close()) {
    dberrprint(&srcdb, "DB::close failed");
    err = true;
  }
  return err ? 1 : 0;
}


// perform search command
static int32_t procsearch(const char* path, const char* query, int64_t max, int32_t mode) {
  kc::TreeDB db;
  if (!db.open(path, kc::TreeDB::OREADER)) {
    dberrprint(&db, "DB::open failed");
    return 1;
  }
  std::string nquery;
  normalizequery(query, &nquery);
  bool err = false;
  if (mode == 'a') {
    class VisitorImpl : public kc::DB::Visitor {
     public:
      VisitorImpl(const std::string& query, int64_t max) :
          qbuf_(query.data()), qsiz_(query.size()), max_(max), thres_(0), lock_(), queue_() {
        if (qsiz_ > 256) qsiz_ = 256;
        thres_ = qsiz_ / 3;
        if (thres_ < 3) thres_ = 3;
      }
     private:
      const char* visit_full(const char* kbuf, size_t ksiz,
                             const char* vbuf, size_t vsiz, size_t* sp) {
        if (ksiz < 1) return NOP;
        ksiz--;
        while (ksiz > 0 && kbuf[ksiz] != '\t') {
          ksiz--;
        }
        if (ksiz > 256) ksiz = 256;
        const char* qbuf = qbuf_;
        size_t qsiz = qsiz_;
        size_t dsiz = qsiz + 1;
        uint32_t* tbl = new uint32_t[(ksiz+1)*dsiz];
        for (size_t i = 0; i <= ksiz; i++) {
          tbl[i*dsiz] = i;
        }
        for (size_t i = 1; i <= qsiz; i++) {
          tbl[i] = i;
        }
        kbuf--;
        qbuf--;
        for (size_t i = 1; i <= ksiz; i++) {
          for (size_t j = 1; j <= qsiz; j++) {
            uint32_t ac = tbl[(i-1)*dsiz+j] + 1;
            uint32_t bc = tbl[i*dsiz+j-1] + 1;
            uint32_t cc = tbl[(i-1)*dsiz+j-1] + (kbuf[i] != qbuf[j]);
            ac = ac < bc ? ac : bc;
            tbl[i*dsiz+j] = ac < cc ? ac : cc;
          }
        }
        uint32_t ed = tbl[ksiz*dsiz+qsiz];
        delete[] tbl;
        if (ed <= thres_) {
          kc::ScopedMutex lock(&lock_);
          if ((int64_t)queue_.size() < max_) {
            AmbiguousRecord rec = { ed, std::string(vbuf, vsiz) };
            queue_.push(rec);
          } else {
            const AmbiguousRecord& top = queue_.top();
            if (ed < top.ed) {
              queue_.pop();
              AmbiguousRecord rec = { ed, std::string(vbuf, vsiz) };
              queue_.push(rec);
            }
          }
        }
        return NOP;
      }
      void visit_after() {
        std::vector<AmbiguousRecord> recs;
        while (!queue_.empty()) {
          recs.push_back(queue_.top());
          queue_.pop();
        }
        std::vector<AmbiguousRecord>::reverse_iterator rit = recs.rbegin();
        std::vector<AmbiguousRecord>::reverse_iterator ritend = recs.rend();
        while (rit != ritend) {
          std::cout << rit->text << "\t" << rit->ed << std::endl;
          ++rit;
        }
      }
      const char* qbuf_;
      size_t qsiz_;
      int64_t max_;
      uint32_t thres_;
      kc::Mutex lock_;
      std::priority_queue<AmbiguousRecord> queue_;
    };
    VisitorImpl visitor(nquery, max);
    if (!db.scan_parallel(&visitor, 8)) {
      dberrprint(&db, "DB::close failed");
      err = true;
    }
  } else if (mode == 'm') {
    class VisitorImpl : public kc::DB::Visitor {
     public:
      VisitorImpl(const std::string& query, int64_t max) :
          query_(query), max_(max), lock_(), queue_() {}
     private:
      const char* visit_full(const char* kbuf, size_t ksiz,
                             const char* vbuf, size_t vsiz, size_t* sp) {
        if (ksiz < 1) return NOP;
        ksiz--;
        while (ksiz > 0 && kbuf[ksiz] != '\t') {
          ksiz--;
        }
        std::string key(kbuf, ksiz);
        if (key.find(query_) != std::string::npos) {
          kc::ScopedMutex lock(&lock_);
          if ((int64_t)queue_.size() < max_) {
            PlainRecord rec = { key, std::string(vbuf, vsiz) };
            queue_.push(rec);
          } else {
            const PlainRecord& top = queue_.top();
            if (key < top.key) {
              queue_.pop();
              PlainRecord rec = { key, std::string(vbuf, vsiz) };
              queue_.push(rec);
            }
          }
        }
        return NOP;
      }
      void visit_after() {
        std::vector<PlainRecord> recs;
        while (!queue_.empty()) {
          recs.push_back(queue_.top());
          queue_.pop();
        }
        std::vector<PlainRecord>::reverse_iterator rit = recs.rbegin();
        std::vector<PlainRecord>::reverse_iterator ritend = recs.rend();
        while (rit != ritend) {
          std::cout << rit->text << std::endl;
          ++rit;
        }
      }
      std::string query_;
      int64_t max_;
      kc::Mutex lock_;
      std::priority_queue<PlainRecord> queue_;
    };
    VisitorImpl visitor(nquery, max);
    if (!db.scan_parallel(&visitor, 8)) {
      dberrprint(&db, "DB::close failed");
      err = true;
    }
  } else if (mode == 'M') {
    class VisitorImpl : public kc::DB::Visitor {
     public:
      VisitorImpl(const std::string& query, int64_t max) :
          query_(query), max_(max), lock_(), queue_() {}
     private:
      const char* visit_full(const char* kbuf, size_t ksiz,
                             const char* vbuf, size_t vsiz, size_t* sp) {
        if (ksiz < 1) return NOP;
        const char* rbuf = vbuf;
        size_t rsiz = vsiz;
        while (rsiz > 0 && *rbuf != '\t') {
          rbuf++;
          rsiz--;
        }
        if (rsiz > 0 && *rbuf == '\t') {
          rbuf++;
          rsiz--;
        }
        while (rsiz > 0 && *rbuf != '\t') {
          rbuf++;
          rsiz--;
        }
        if (rsiz > 0 && *rbuf == '\t') {
          rbuf++;
          rsiz--;
        }
        std::string value;
        normalizequery(std::string(rbuf, rsiz), &value);
        if (value.find(query_) != std::string::npos) {
          ksiz--;
          while (ksiz > 0 && kbuf[ksiz] != '\t') {
            ksiz--;
          }
          std::string key(kbuf, ksiz);
          kc::ScopedMutex lock(&lock_);
          if ((int64_t)queue_.size() < max_) {
            PlainRecord rec = { key, std::string(vbuf, vsiz) };
            queue_.push(rec);
          } else {
            const PlainRecord& top = queue_.top();
            if (key < top.key) {
              queue_.pop();
              PlainRecord rec = { key, std::string(vbuf, vsiz) };
              queue_.push(rec);
            }
          }
        }
        return NOP;
      }
      void visit_after() {
        std::vector<PlainRecord> recs;
        while (!queue_.empty()) {
          recs.push_back(queue_.top());
          queue_.pop();
        }
        std::vector<PlainRecord>::reverse_iterator rit = recs.rbegin();
        std::vector<PlainRecord>::reverse_iterator ritend = recs.rend();
        while (rit != ritend) {
          std::cout << rit->text << std::endl;
          ++rit;
        }
      }
      std::string query_;
      int64_t max_;
      kc::Mutex lock_;
      std::priority_queue<PlainRecord> queue_;
    };
    VisitorImpl visitor(nquery, max);
    if (!db.scan_parallel(&visitor, 8)) {
      dberrprint(&db, "DB::close failed");
      err = true;
    }
  } else if (mode == 'r') {
    class VisitorImpl : public kc::DB::Visitor {
     public:
      VisitorImpl(const std::string& query, int64_t max) :
          regex_(), max_(max), lock_(), queue_() {
        regex_.compile(query, kc::Regex::MATCHONLY);
      }
     private:
      const char* visit_full(const char* kbuf, size_t ksiz,
                             const char* vbuf, size_t vsiz, size_t* sp) {
        if (ksiz < 1) return NOP;
        ksiz--;
        while (ksiz > 0 && kbuf[ksiz] != '\t') {
          ksiz--;
        }
        std::string key(kbuf, ksiz);
        if (regex_.match(key)) {
          kc::ScopedMutex lock(&lock_);
          if ((int64_t)queue_.size() < max_) {
            PlainRecord rec = { key, std::string(vbuf, vsiz) };
            queue_.push(rec);
          } else {
            const PlainRecord& top = queue_.top();
            if (key < top.key) {
              queue_.pop();
              PlainRecord rec = { key, std::string(vbuf, vsiz) };
              queue_.push(rec);
            }
          }
        }
        return NOP;
      }
      void visit_after() {
        std::vector<PlainRecord> recs;
        while (!queue_.empty()) {
          recs.push_back(queue_.top());
          queue_.pop();
        }
        std::vector<PlainRecord>::reverse_iterator rit = recs.rbegin();
        std::vector<PlainRecord>::reverse_iterator ritend = recs.rend();
        while (rit != ritend) {
          std::cout << rit->text << std::endl;
          ++rit;
        }
      }
      kc::Regex regex_;
      int64_t max_;
      kc::Mutex lock_;
      std::priority_queue<PlainRecord> queue_;
    };
    VisitorImpl visitor(nquery, max);
    if (!db.scan_parallel(&visitor, 8)) {
      dberrprint(&db, "DB::close failed");
      err = true;
    }
  } else if (mode == 'R') {
    class VisitorImpl : public kc::DB::Visitor {
     public:
      VisitorImpl(const std::string& query, int64_t max) :
          regex_(), max_(max), lock_(), queue_() {
        regex_.compile(query, kc::Regex::MATCHONLY);
      }
     private:
      const char* visit_full(const char* kbuf, size_t ksiz,
                             const char* vbuf, size_t vsiz, size_t* sp) {
        if (ksiz < 1) return NOP;
        const char* rbuf = vbuf;
        size_t rsiz = vsiz;
        while (rsiz > 0 && *rbuf != '\t') {
          rbuf++;
          rsiz--;
        }
        if (rsiz > 0 && *rbuf == '\t') {
          rbuf++;
          rsiz--;
        }
        while (rsiz > 0 && *rbuf != '\t') {
          rbuf++;
          rsiz--;
        }
        if (rsiz > 0 && *rbuf == '\t') {
          rbuf++;
          rsiz--;
        }
        std::string value;
        normalizequery(std::string(rbuf, rsiz), &value);
        if (regex_.match(value)) {
          ksiz--;
          while (ksiz > 0 && kbuf[ksiz] != '\t') {
            ksiz--;
          }
          std::string key(kbuf, ksiz);
          kc::ScopedMutex lock(&lock_);
          if ((int64_t)queue_.size() < max_) {
            PlainRecord rec = { key, std::string(vbuf, vsiz) };
            queue_.push(rec);
          } else {
            const PlainRecord& top = queue_.top();
            if (key < top.key) {
              queue_.pop();
              PlainRecord rec = { key, std::string(vbuf, vsiz) };
              queue_.push(rec);
            }
          }
        }
        return NOP;
      }
      void visit_after() {
        std::vector<PlainRecord> recs;
        while (!queue_.empty()) {
          recs.push_back(queue_.top());
          queue_.pop();
        }
        std::vector<PlainRecord>::reverse_iterator rit = recs.rbegin();
        std::vector<PlainRecord>::reverse_iterator ritend = recs.rend();
        while (rit != ritend) {
          std::cout << rit->text << std::endl;
          ++rit;
        }
      }
      kc::Regex regex_;
      int64_t max_;
      kc::Mutex lock_;
      std::priority_queue<PlainRecord> queue_;
    };
    VisitorImpl visitor(nquery, max);
    if (!db.scan_parallel(&visitor, 8)) {
      dberrprint(&db, "DB::close failed");
      err = true;
    }
  } else {
    std::string qstr(nquery);
    if (mode == 'f') qstr.append("\t");
    kc::TreeDB::Cursor* cur = db.cursor();
    cur->jump(qstr);
    char* kbuf;
    size_t ksiz;
    const char* vbuf;
    size_t vsiz;
    while (max > 0 && (kbuf = cur->get(&ksiz, &vbuf, &vsiz, true)) != NULL) {
      if (ksiz >= qstr.size() && !std::memcmp(kbuf, qstr.data(), qstr.size())) {
        std::cout.write(vbuf, vsiz);
        std::cout << std::endl;
      } else {
        max = 0;
      }
      delete[] kbuf;
      max--;
    }
    delete cur;
  }
  if (!db.close()) {
    dberrprint(&db, "DB::close failed");
    err = true;
  }
  return err ? 1 : 0;
}



// END OF FILE
