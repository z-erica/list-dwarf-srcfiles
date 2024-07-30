/* This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.

 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.

 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <https://www.gnu.org/licenses/>. 
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <libdwarf.h>

static void usage(FILE *f) {
  extern char const *__progname;
  fprintf(f, "usage: %s [OPTION] FILENAME\n"
"\n"
"TODO\n"
"\n"
"      -h  print this message and exit.\n",
    __progname);
}

int scan_dwarf_debug(Dwarf_Debug dbg, Dwarf_Error *error) {
  int rc;

  // iterate though CUs (compilation units)
  for (;;) {
    Dwarf_Die die;

    // metadata we don't need
    Dwarf_Unsigned header_length;
    Dwarf_Half version_stamp;
    Dwarf_Off abbrev_offset;
    Dwarf_Half address_size;
    Dwarf_Half length_size;
    Dwarf_Half extension_size;
    Dwarf_Sig8 type_signature;
    Dwarf_Unsigned typeoffset;
    Dwarf_Unsigned next_cu_header_offset;
    Dwarf_Half header_cu_type;

    rc = dwarf_next_cu_header_e(dbg, 1, &die,
        &header_length, &version_stamp, &abbrev_offset,
        &address_size, &length_size, &extension_size,
        &type_signature, &typeoffset,
        &next_cu_header_offset, &header_cu_type,
        error);
    if (rc == DW_DLV_ERROR) return rc;
    if (rc == DW_DLV_NO_ENTRY) break;

    // now iterate through source files referenced by the CU
    char **srcfiles;
    Dwarf_Signed filecount;
    rc = dwarf_srcfiles(die, &srcfiles, &filecount, error);
    if (rc == DW_DLV_ERROR) return rc;

    for (int i = 0; i < filecount; i += 1) {
      // sanity check: should contain no inline newlines
      assert(!strchr(srcfiles[i], '\n'));

      printf("%s\n", srcfiles[i]);
    }

    // NOTE: libdwarf does not mention whether srcfiles should be freed by us, so assume not
    dwarf_dealloc_die(die);
  }

  return DW_DLV_OK;
}

int main(int argc, char **argv) {
  int c;
  while ((c = getopt(argc, argv, "h")) > 0) {
    switch (c) {
      case 'h':
        usage(stdout);
        return 0;

      default:
        usage(stderr);
        return 1;
    }
  }

  if (optind != argc-1) {
    usage(stderr);
    return 1;
  }

  char *path = argv[optind];
  Dwarf_Debug dbg;
  Dwarf_Error error;
  int rc;

  // ensure the file exists, since libdwarf won't specifically report this
  struct stat statbuf;
  rc = stat(path, &statbuf);
  if (rc != 0) {
    perror("stat");
    return 1;
  }

  rc = dwarf_init_path(path, NULL, 0, DW_GROUPNUMBER_ANY, NULL, NULL, &dbg, &error);
  if (rc == DW_DLV_ERROR) {
    fprintf(stderr, "could not initialize libdwarf: %s\n", dwarf_errmsg(error));
    return 1;
  } else if (rc != DW_DLV_OK) {
    fprintf(stderr, "could not initialize libdwarf (unsupported file format?)\n");
    return 1;
  }

  rc = scan_dwarf_debug(dbg, &error);
  if (rc == DW_DLV_ERROR) {
    fprintf(stderr, "could not scan file: %s\n", dwarf_errmsg(error));
    return 1;
  }

  dwarf_finish(dbg);
}
