#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <sys/stat.h>

#include "beargit.h"
#include "util.h"

/* Implementation Notes:
 *
 * - Functions return 0 if successful, 1 if there is an error.
 * - All error conditions in the function description need to be implemented
 *   and written to stderr. We catch some additional errors for you in main.c.
 * - Output to stdout needs to be exactly as specified in the function description.
 * - Only edit this file (beargit.c)
 * - You are given the following helper functions:
 *   * fs_mkdir(dirname): create directory <dirname>
 *   * fs_rm(filename): delete file <filename>
 *   * fs_mv(src,dst): move file <src> to <dst>, overwriting <dst> if it exists
 *   * fs_cp(src,dst): copy file <src> to <dst>, overwriting <dst> if it exists
 *   * write_string_to_file(filename,str): write <str> to filename (overwriting contents)
 *   * read_string_from_file(filename,str,size): read a string of at most <size> (incl.
 *     NULL character) from file <filename> and store it into <str>. Note that <str>
 *     needs to be large enough to hold that string.
 *  - You NEED to test your code. The autograder we provide does not contain the
 *    full set of tests that we will run on your code. See "Step 5" in the homework spec.
 */

/* beargit init
 *
 * - Create .beargit directory
 * - Create empty .beargit/.index file
 * - Create .beargit/.prev file containing 0..0 commit id
 *
 * Output (to stdout):
 * - None if successful
 */

int beargit_init(void) {
  fs_mkdir(".beargit");

  FILE* findex = fopen(".beargit/.index", "w");
  fclose(findex);
  
  write_string_to_file(".beargit/.prev", "cccccccccccccccccccccccccccccccccccccccc");

  return 0;
}


/* beargit add <filename>
 * 
 * - Append filename to list in .beargit/.index if it isn't in there yet
 *
 * Possible errors (to stderr):
 * >> ERROR: File <filename> already added
 *
 * Output (to stdout):
 * - None if successful
 */

int beargit_add(const char* filename) {
  FILE* findex = fopen(".beargit/.index", "r");
  FILE *fnewindex = fopen(".beargit/.newindex", "w");

  char line[FILENAME_SIZE];
  while(fgets(line, sizeof(line), findex)) {
    strtok(line, "\n");
    if (strcmp(line, filename) == 0) {
      fprintf(stderr, "ERROR: File %s already added\n", filename);
      fclose(findex);
      fclose(fnewindex);
      fs_rm(".beargit/.newindex");
      return 3;
    }

    fprintf(fnewindex, "%s\n", line);
  }

  fprintf(fnewindex, "%s\n", filename);
  fclose(findex);
  fclose(fnewindex);

  fs_mv(".beargit/.newindex", ".beargit/.index");

  return 0;
}


/* beargit rm <filename>
 * 
 * See "Step 2" in the homework 1 spec.
 *
 */

int beargit_rm(const char* filename) {
  /* COMPLETE THE REST */
  FILE* findex = fopen(".beargit/.index", "r");
  FILE *fnewindex = fopen(".beargit/.newindex", "w");

  char line[FILENAME_SIZE];
  int exist = 0;
  while(fgets(line, sizeof(line), findex)) {
    strtok(line, "\n");
    if (strcmp(line, filename) == 0) {
      exist += 1;
    }
    else {
      fprintf(fnewindex, "%s\n", line);
    }
  }
  if (exist == 0) {
    fprintf(stderr, "ERROR: File %s not tracked\n", filename);
  }
  fclose(findex);
  fclose(fnewindex);

  fs_mv(".beargit/.newindex", ".beargit/.index");
  return 0;

  }

/* beargit commit -m <msg>
 *
 * See "Step 3" in the homework 1 spec.
 *
 */

// const char* go_bears = "GO BEARS!";

// int is_commit_msg_ok(const char* msg) {
//   /* COMPLETE THE REST */
//   if (strstr(msg, go_bears) != NULL) {
//     return 1;
//   }
//   return 0;
// }

// void next_commit_id(char* commit_id) {
//   /* COMPLETE THE REST */
// }

// int beargit_commit(const char* msg) {
//   if (!is_commit_msg_ok(msg)) {
//     fprintf(stderr, "ERROR: Message must contain \"%s\"\n", go_bears);
//     return 1;
//   }

//   char commit_id[COMMIT_ID_SIZE];
//   read_string_from_file(".beargit/.prev", commit_id, COMMIT_ID_SIZE);
//   next_commit_id(commit_id);

//   /* COMPLETE THE REST */

//   return 0;
// }
const char* go_bears = "GO BEARS!";

int is_commit_msg_ok(const char* msg) {
  /* COMPLETE THE REST */
  if (strstr(msg, go_bears) != NULL) {
    return 1;
  }
  return 0;
}

int helper_id(char* commit_id) {
  if (*commit_id == '6' || *commit_id == '0') {
    return 0;
  } else if (*commit_id == '1') {
    return 1;
  } else {
    return 2;
  }
}
char helper_tos(int temps) {
  if (temps == 0) {
    return '6';
  } else if (temps == 1) {
    return '1';
  } else {
    return 'c';
  }

}

int next_commit_id(char* commit_id) {
  int i;
  int temp = 0;
  int tempp = 1;
  char result[COMMIT_ID_SIZE];
  for(i = 0; i < strlen(commit_id); i++) {
    temp += tempp*helper_id(&commit_id[i]);
    tempp *= 3;
  }
  temp += 1;
  tempp = temp;
  for (i = 0; i < strlen(commit_id); i++) {
    result[i] = helper_tos(tempp % 3);
    tempp /= 3;
  }
  return result;
}

int beargit_commit(const char* msg) {
  if (!is_commit_msg_ok(msg)) {
    fprintf(stderr, "ERROR: Message must contain \"%s\"\n", go_bears);
    return 1;
  }
  char commit_id[COMMIT_ID_SIZE];
  read_string_from_file(".beargit/.prev", commit_id, COMMIT_ID_SIZE);
  int nextId = next_commit_id(commit_id);
  char newid[1000] = ".beargit/.";
  char newidd[1000] = ".beargit/.";
  char newmsg[1000] = ".beargit/.";
  strcat(newmsg, nextId);
  strcat(newmsg, "/.msg");
  strcat(newidd, nextId);
  strcat(newid, nextId);
  strcat(newidd, "/.prev");
  fs_mkdir(newid);
  strcat(newid, "/.index");
  fs_cp(".beargit/.index", newid);
  fs_cp("beargit/.prev", newidd);
  char line[FILENAME_SIZE];
  FILE* findex = fopen(".beargit/.index", "r");
  while(fgets(line, sizeof(line), findex)) {
    strtok(line, "\n");
    char newfile[1000] = ".beargit/.";
    char oldfile[1000] = ".beargit/.";
    strcat(newfile, nextId);
    strcat(newfile, "/.");
    strcat(oldfile, line);
    strcat(newfile, line);
    fs_cp(oldfile, newfile);
  }
  write_string_to_file(newmsg, msg);
  write_string_to_file(".beargit/.prev", nextId);


  return 0;
}
/* beargit status
 *
 * See "Step 1" in the homework 1 spec.
 *
 */

int beargit_status() {
  /* COMPLETE THE REST */
  FILE* findex = fopen(".beargit/.index", "r");
  char line[FILENAME_SIZE];
  int n = 0;
  fprintf(stdout, "Tracked files:\n\n");
  while(fgets(line, sizeof(line), findex)) {
    strtok(line,"\n");
    n++;
    fprintf(stdout, "  %s\n", line);
  }

  fprintf(stdout, "\n%d files total\n", n );
  fclose(findex);
  return 0;
}

/* beargit log
 *
 * See "Step 4" in the homework 1 spec.
 *
 */

int beargit_log() {
  /* COMPLETE THE REST */
  char id[COMMIT_ID_SIZE];
  char msg[10000];
  read_string_from_file(".beargit/.prev", id, COMMIT_ID_SIZE);
  if (strcmp(id, "cccccccccccccccccccccccccccccccccccccccc") == 0) {
    fprintf(stderr, "ERROR: There are no commits!\n");
    return 1;
  } else {
    fprintf(stdout, "\n");
    while (strcmp(id, "cccccccccccccccccccccccccccccccccccccccc") != 0) {
      char *s;
      sprintf(s,".%s", id);
      read_string_from_file(".beargit/s/.msg", msg, 10000);
      fprintf(stdout, "commit %s\n    %s\n", id, msg);
      read_string_from_file(".beargit/s/.prev", id, COMMIT_ID_SIZE);
    }
  }
  return 0;
}
