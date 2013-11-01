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


#include <kcdict.h>
#include <kcutil.h>

namespace kc = kyotocabinet;


// global variables
const char* g_progname;                  // program name


// function prototypes
int main(int argc, char** argv);
static void usage();
static void oprintf(const char* format, ...);
static void oputchar(char c);
static void eprintf(const char* format, ...);
static int32_t runimport(int argc, char** argv);
static int32_t procimport(const char* path);


// main routine
int main(int argc, char** argv) {
  g_progname = argv[0];
  kc::setstdiobin();
  if (argc < 2) usage();
  int32_t rv = 0;
  if (!std::strcmp(argv[1], "create")) {
    rv = runimport(argc, argv);
  } else {
    usage();
  }
  return 0;
}


// print the usage and exit
static void usage() {
  eprintf("%s: the command line utility of the word dictionary\n",
          g_progname);
  eprintf("\n");
  eprintf("usage:\n");
  eprintf("  %s import path\n", g_progname);
  eprintf("  %s search path expr\n", g_progname);
  eprintf("\n");
  std::exit(1);
}


// print formatted information string and flush the buffer
static void oprintf(const char* format, ...) {
  std::string msg;
  va_list ap;
  va_start(ap, format);
  kc::vstrprintf(&msg, format, ap);
  va_end(ap);
  std::cout << msg;
  std::cout.flush();
}


// print a character and flush the buffer
static void oputchar(char c) {
  std::cout << c;
  std::cout.flush();
}


// print formatted error string and flush the buffer
static void eprintf(const char* format, ...) {
  std::string msg;
  va_list ap;
  va_start(ap, format);
  kc::vstrprintf(&msg, format, ap);
  va_end(ap);
  std::cerr << msg;
  std::cerr.flush();
}


// parse arguments of import command
static int32_t runimport(int argc, char** argv) {
  bool argbrk = false;
  const char* path = NULL;
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
    } else {
      usage();
    }
  }
  if (!path) usage();
  int32_t rv = procimport(path);
  return rv;
}


// perform import command
static int32_t procimport(const char* path) {
  return 0;
}



// END OF FILE
