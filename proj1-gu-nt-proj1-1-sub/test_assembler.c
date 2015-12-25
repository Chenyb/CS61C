#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <CUnit/Basic.h>

#include "src/utils.h"
#include "src/tables.h"
#include "src/translate_utils.h"
#include "src/translate.h"

const char* TMP_FILE = "test_output.txt";
const int BUF_SIZE = 1024;

/****************************************
 *  Helper functions 
 ****************************************/

int do_nothing() {
    return 0;
}

int init_log_file() {
    set_log_file(TMP_FILE);
    return 0;
}

int check_lines_equal(char **arr, int num) {
    char buf[BUF_SIZE];

    FILE *f = fopen(TMP_FILE, "r");
    if (!f) {
        CU_FAIL("Could not open temporary file");
        return 0;
    }
    for (int i = 0; i < num; i++) {
        if (!fgets(buf, BUF_SIZE, f)) {
            CU_FAIL("Reached end of file");
            return 0;
        }
        CU_ASSERT(!strncmp(buf, arr[i], strlen(arr[i])));
    }
    fclose(f);
    return 0;
}

/****************************************
 *  Test cases for translate_utils.c 
 ****************************************/

void test_translate_reg() {
    CU_ASSERT_EQUAL(translate_reg("$0"), 0);
    CU_ASSERT_EQUAL(translate_reg("$at"), 1);
    CU_ASSERT_EQUAL(translate_reg("$v0"), 2);
    CU_ASSERT_EQUAL(translate_reg("$a0"), 4);
    CU_ASSERT_EQUAL(translate_reg("$a1"), 5);
    CU_ASSERT_EQUAL(translate_reg("$a2"), 6);
    CU_ASSERT_EQUAL(translate_reg("$a3"), 7);
    CU_ASSERT_EQUAL(translate_reg("$t0"), 8);
    CU_ASSERT_EQUAL(translate_reg("$t1"), 9);
    CU_ASSERT_EQUAL(translate_reg("$t2"), 10);
    CU_ASSERT_EQUAL(translate_reg("$t3"), 11);
    CU_ASSERT_EQUAL(translate_reg("$s0"), 16);
    CU_ASSERT_EQUAL(translate_reg("$s1"), 17);
    CU_ASSERT_EQUAL(translate_reg("$3"), -1);
    CU_ASSERT_EQUAL(translate_reg("asdf"), -1);
    CU_ASSERT_EQUAL(translate_reg("hey there"), -1);
}

void test_translate_num() {
    long int output;

    CU_ASSERT_EQUAL(translate_num(&output, "35", -1000, 1000), 0);
    CU_ASSERT_EQUAL(output, 35);
    CU_ASSERT_EQUAL(translate_num(&output, "145634236", 0, 9000000000), 0);
    CU_ASSERT_EQUAL(output, 145634236);
    CU_ASSERT_EQUAL(translate_num(&output, "0xC0FFEE", -9000000000, 9000000000), 0);
    CU_ASSERT_EQUAL(output, 12648430);
    CU_ASSERT_EQUAL(translate_num(&output, "72", -16, 72), 0);
    CU_ASSERT_EQUAL(output, 72);
    CU_ASSERT_EQUAL(translate_num(&output, "72", -16, 71), -1);
    CU_ASSERT_EQUAL(translate_num(&output, "72", 72, 150), 0);
    CU_ASSERT_EQUAL(output, 72);
    CU_ASSERT_EQUAL(translate_num(&output, "72", 73, 150), -1);
    CU_ASSERT_EQUAL(translate_num(&output, "35x", -100, 100), -1);
    CU_ASSERT_EQUAL(translate_num(&output, "0", -1000, 1000), 0);
    CU_ASSERT_EQUAL(output, 0);
    CU_ASSERT_EQUAL(translate_num(&output, "-5", -1000, 1000), 0);
    CU_ASSERT_EQUAL(output, -5);
}

/****************************************
 *  Test cases for tables.c 
 ****************************************/

void test_table_1() {
    int retval;

    SymbolTable* tbl = create_table(SYMTBL_UNIQUE_NAME);
    CU_ASSERT_PTR_NOT_NULL(tbl);

    retval = add_to_table(tbl, "abc", 8);
    CU_ASSERT_EQUAL(retval, 0);
    retval = add_to_table(tbl, "efg", 12);
    CU_ASSERT_EQUAL(retval, 0);
    retval = add_to_table(tbl, "q45", 16);
    CU_ASSERT_EQUAL(retval, 0);
    retval = add_to_table(tbl, "q45", 24); 
    CU_ASSERT_EQUAL(retval, -1); 
    retval = add_to_table(tbl, "bob", 14); 
    CU_ASSERT_EQUAL(retval, -1); 

    retval = get_addr_for_symbol(tbl, "abc");
    CU_ASSERT_EQUAL(retval, 8); 
    retval = get_addr_for_symbol(tbl, "q45");
    CU_ASSERT_EQUAL(retval, 16); 
    retval = get_addr_for_symbol(tbl, "ef");
    CU_ASSERT_EQUAL(retval, -1);
    
    free_table(tbl);

    char* arr[] = { "Error: name 'q45' already exists in table.",
                    "Error: address is not a multiple of 4." };
    check_lines_equal(arr, 2);
    SymbolTable* tbl2 = create_table(SYMTBL_NON_UNIQUE);
    CU_ASSERT_PTR_NOT_NULL(tbl2);

    retval = add_to_table(tbl2, "q45", 16);
    CU_ASSERT_EQUAL(retval, 0);
    retval = add_to_table(tbl2, "q45", 24);  
    CU_ASSERT_EQUAL(retval, 0);

    free_table(tbl2);
}

void test_table_2() {
    int retval, max = 100;

    SymbolTable* tbl = create_table(SYMTBL_UNIQUE_NAME);
    CU_ASSERT_PTR_NOT_NULL(tbl);

    char buf[10];
    for (int i = 0; i < max; i++) {
        sprintf(buf, "%d", i);
        retval = add_to_table(tbl, buf, 4 * i);
        CU_ASSERT_EQUAL(retval, 0);
    }

    for (int i = 0; i < max; i++) {
        sprintf(buf, "%d", i);
        retval = get_addr_for_symbol(tbl, buf);
        CU_ASSERT_EQUAL(retval, 4 * i);
    }

    free_table(tbl);
}

/****************************************
 *  Test cases for translate.c 
 ****************************************/
void test_rtype() {   

    FILE* fstout = fopen(TMP_FILE, "w");
    if (!fstout) {
        CU_FAIL("Could not open temporary file");
        return;
    }
    char *args1[3] = {"$s0", "$s1", "$s2"};
    char *args2[3] = {"$t0", "$s2", "$t3"};
    char *args3[3] = {"$a0", "$a0", "$zero"};
    char *args4[3] = {"$v0", "$a2", "$0"};
    char *args5[3] = {"$t2", "$t2", "$a1"};
    char *args6[3] = {"$s2", "$t0", "$a1"};
    char *args7[3] = {"$a0", "$s0", "$t1"};
    char *args8[3] = {"$v0", "$a2", "$zero"};
    char *args9[3] = {"$v2", "$a2", "$zero"};
    char *args10[3] = {"$a5", "$a0", "$zero"};
    char *args11[3] = {"$t4", "$a0", "$zero"};
    char *args12[3] = {"$a4", "$a0", "$zero"};


    int err1 = translate_inst(fstout, "addu", args1, 3, 0, NULL, NULL);
    CU_ASSERT_EQUAL(err1, 0);
    int err2 = translate_inst(fstout, "slt", args2, 3, 0, NULL, NULL);
    CU_ASSERT_EQUAL(err2, 0);
    int err3 = translate_inst(fstout, "sltu", args3, 3, 0, NULL, NULL);
    CU_ASSERT_EQUAL(err3, 0);
    int err4 = translate_inst(fstout, "or", args4, 3, 0, NULL, NULL);
    CU_ASSERT_EQUAL(err4, 0);
    int err5 = translate_inst(fstout, "addu", args5, 3, 0, NULL, NULL);    
    CU_ASSERT_EQUAL(err5, 0);
    int err6 = translate_inst(fstout, "slt", args6, 3, 0, NULL, NULL);    
    CU_ASSERT_EQUAL(err6, 0);
    int err7 = translate_inst(fstout, "sltu", args7, 3, 0, NULL, NULL);    
    CU_ASSERT_EQUAL(err7, 0);
    int err8 = translate_inst(fstout, "or", args8, 3, 0, NULL, NULL);    
    CU_ASSERT_EQUAL(err8, 0);
    int err9 = translate_inst(fstout, "addu", args9, 3, 0, NULL, NULL);    
    CU_ASSERT_EQUAL(err9, -1);
    int err10 = translate_inst(fstout, "slt", args10, 3, 0, NULL, NULL);    
    CU_ASSERT_EQUAL(err10, -1);
    // int err11 = translate_inst(fstout, "sltu", args11, 3, 0, NULL, NULL);    
    // CU_ASSERT_EQUAL(err11, -1);
    int err12 = translate_inst(fstout, "or", args12, 3, 0, NULL, NULL);    
    CU_ASSERT_EQUAL(err12, -1);
    int err13 = translate_inst(fstout, "or", args12, 2, 0, NULL, NULL);    
    CU_ASSERT_EQUAL(err13, -1);



    fclose(fstout);
    char* ans[] = {"02328021", "024b402a", "0080202b", "00c01025","01455021", "0105902a","0209202b","00c01025"};
    check_lines_equal(ans, 8);
}

void test_shift() {
    FILE* fstout = fopen(TMP_FILE, "w");
    if (!fstout) {
        CU_FAIL("Could not open temporary file");
        return;
    }

    char *args1[3] = {"$s0", "$s1", "2"};
    char *args2[3] = {"$v0", "$a2", "18"};
    char *args3[3] = {"$s0", "$s1", "33"};
    char *args4[3] = {"$s2", "$a0", "$s2"};

    int err1 = translate_inst(fstout, "sll", args1, 3, 0, NULL, NULL);
    CU_ASSERT_EQUAL(err1, 0);
    int err2 = translate_inst(fstout, "sll", args2, 3, 0, NULL, NULL);
    CU_ASSERT_EQUAL(err2, 0);
    int err3 = translate_inst(fstout, "sll", args3, 3, 0, NULL, NULL);
    CU_ASSERT_EQUAL(err3, -1);
    int err4 = translate_inst(fstout, "sll", args4, 3, 0, NULL, NULL);
    CU_ASSERT_EQUAL(err4, -1);

    fclose(fstout);
    char* ans[] = {"00118080", "00061480"};
    check_lines_equal(ans, 2);
}

void test_addiu() {
    FILE* fstout = fopen(TMP_FILE, "w");
    if (!fstout) {
        CU_FAIL("Could not open temporary file");
        return;
    }
    
    char *args1[3] = {"$s0", "$s1", "1000"};
    char *args2[3] = {"$v0", "$a2", "0"};
//adding negative or 1000000?
    int err1 = translate_inst(fstout, "addiu", args1, 3, 0, NULL, NULL);
    CU_ASSERT_EQUAL(err1, 0);
    int err2 = translate_inst(fstout, "addiu", args2, 3, 0, NULL, NULL);
    CU_ASSERT_EQUAL(err2, 0);

    fclose(fstout);
    char* ans[] = {"263003e8", "24c20000"};
    check_lines_equal(ans, 2);

}

void test_jr() {
    FILE* fstout = fopen(TMP_FILE, "w");
    if (!fstout) {
        CU_FAIL("Could not open temporary file");
        return;
    }

    char *args1[1] = {"$ra"};
    char *args2[1] = {"$sp"};
    char *args3[1] = {"$t0"};
    char *args4[1] = {"$a1"};
    char *args5[1] = {"$zero"};
    char *args6[1] = {"$v0"};
    char *args7[1] = {"$v3"};
    char *args8[1] = {"$t4"};

    int err1 = translate_inst(fstout, "jr", args1, 1, 0, NULL, NULL);
    CU_ASSERT_EQUAL(err1, 0);
    int err2 = translate_inst(fstout, "jr", args2, 1, 0, NULL, NULL);
    CU_ASSERT_EQUAL(err2, 0);
    int err3 = translate_inst(fstout, "jr", args3, 1, 0, NULL, NULL);
    CU_ASSERT_EQUAL(err3, 0);
    int err4 = translate_inst(fstout, "jr", args4, 1, 0, NULL, NULL);
    CU_ASSERT_EQUAL(err4, 0);
    int err5 = translate_inst(fstout, "jr", args5, 1, 0, NULL, NULL);
    CU_ASSERT_EQUAL(err5, 0);
    int err6 = translate_inst(fstout, "jr", args6, 1, 0, NULL, NULL);
    CU_ASSERT_EQUAL(err6, 0);
    int err7 = translate_inst(fstout, "jr", args7, 3, 0, NULL, NULL);
    CU_ASSERT_EQUAL(err7, -1);
    int err8 = translate_inst(fstout, "jr", args8, 3, 0, NULL, NULL);
    CU_ASSERT_EQUAL(err8, -1);

    fclose(fstout);
    char* ans[] = {"03e00008", "03a00008","01000008", "00a00008", "00000008", "00400008"};
    check_lines_equal(ans, 6);

}

void test_mem() {
    FILE* fstout = fopen(TMP_FILE, "w");
    if (!fstout) {
        CU_FAIL("Could not open temporary file");
        return;
    }

    char *args1[3] = {"$s0", "0", "$ra"};
    char *args2[3] = {"$t0", "2", "$sp"};
    char *args3[3] = {"$a0", "3", "$a1"};
    char *args4[3] = {"$v0", "4", "$0"};
    char *args5[3] = {"$t2", "1000", "$a1"};
    char *args6[3] = {"$s2", "-5", "$a1"};
    char *args7[3] = {"$s2", "$0", "$a1"};


    int err1 = translate_inst(fstout, "lw", args1, 3, 0, NULL, NULL);
    CU_ASSERT_EQUAL(err1, 0);
    int err2 = translate_inst(fstout, "sw", args2, 3, 0, NULL, NULL);
    CU_ASSERT_EQUAL(err2, 0);
    int err3 = translate_inst(fstout, "lb", args3, 3, 0, NULL, NULL);
    CU_ASSERT_EQUAL(err3, 0);
    int err4 = translate_inst(fstout, "sb", args4, 3, 0, NULL, NULL);
    CU_ASSERT_EQUAL(err4, 0);
    int err5 = translate_inst(fstout, "lbu", args5, 3, 0, NULL, NULL);
    CU_ASSERT_EQUAL(err5, 0);
    int err6 = translate_inst(fstout, "lbu", args6, 3, 0, NULL, NULL);
    CU_ASSERT_EQUAL(err6, 0);
    int err7 = translate_inst(fstout, "lbu", args7, 3, 0, NULL, NULL);
    CU_ASSERT_EQUAL(err7, -1);

    fclose(fstout);
    char* ans[] = {"8ff00000", "afa80002","80a40003", "a0020004", "90aa03e8", "90b2fffb"};
    check_lines_equal(ans, 6);
}

void test_lui() {
    FILE* fstout = fopen(TMP_FILE, "w");
    if (!fstout) {
        CU_FAIL("Could not open temporary file");
        return;
    }

    char *args1[2] = {"$s0", "0"};
    char *args2[2] = {"$t0", "65535"};
    char *args3[2] = {"$a0", "-1"};
    char *args4[2] = {"$v0", "100000"};

    int err1 = translate_inst(fstout, "lui", args1, 2, 0, NULL, NULL);
    CU_ASSERT_EQUAL(err1, 0);
    int err2 = translate_inst(fstout, "lui", args2, 2, 0, NULL, NULL);
    CU_ASSERT_EQUAL(err2, 0);
    int err3 = translate_inst(fstout, "lui", args3, 2, 0, NULL, NULL);
    CU_ASSERT_EQUAL(err3, -1);
    int err4 = translate_inst(fstout, "lui", args4, 2, 0, NULL, NULL);
    CU_ASSERT_EQUAL(err4, -1);

    fclose(fstout);
    char* ans[] = {"3c100000", "3c08ffff"};
    check_lines_equal(ans, 2);

}


void test_ori() {
    FILE* fstout = fopen(TMP_FILE, "w");
    if (!fstout) {
        CU_FAIL("Could not open temporary file");
        return;
    }

    char *args1[3] = {"$sp", "$t2","0"};
    char *args2[3] = {"$t2", "$v0", "65535"};
    char *args3[3] = {"$a0", "$zero", "-1"};
    char *args4[3] = {"$t1", "a2", "100000"};

    int err1 = translate_inst(fstout, "ori", args1, 3, 0, NULL, NULL);
    CU_ASSERT_EQUAL(err1, 0);
    int err2 = translate_inst(fstout, "ori", args2, 3, 0, NULL, NULL);
    CU_ASSERT_EQUAL(err2, 0);
    int err3 = translate_inst(fstout, "ori", args3, 3, 0, NULL, NULL);
    CU_ASSERT_EQUAL(err3, -1);
    int err4 = translate_inst(fstout, "ori", args4, 3, 0, NULL, NULL);
    CU_ASSERT_EQUAL(err4, -1);
    fclose(fstout);
    char* ans[] = {"355d0000", "344affff"};
    check_lines_equal(ans, 2);

}


void test_jump() {
    FILE* fstout = fopen(TMP_FILE, "w");
    if (!fstout) {
        CU_FAIL("Could not open temporary file");
        return;
    }
    SymbolTable* t1 = create_table(0);
    add_to_table(t1, "test1", 4194308);
    add_to_table(t1, "test2", 4194316);
    char *args1[1] = {"test1"};
    char *args2[1] = {"test2"};
    char *args3[1] = {"test3"};

    int err1 = translate_inst(fstout, "j", args1, 1, 4194304, NULL, t1);
    CU_ASSERT_EQUAL(err1, 0);
    int err2 = translate_inst(fstout, "j", args2, 1, 4194316, NULL, t1);
    CU_ASSERT_EQUAL(err2, 0);
    int err3 = translate_inst(fstout, "jal", args1, 1, 4194320, NULL, t1);
    CU_ASSERT_EQUAL(err3, 0);
    int err4 = translate_inst(fstout, "jal", args2, 1, 4194324, NULL, t1);
    CU_ASSERT_EQUAL(err4, 0);
    // int err5 = translate_inst(fstout, "j", args3, 1, 4194304, NULL, t1);
    // CU_ASSERT_EQUAL(err5, -1);
    int err6 = translate_inst(fstout, "jal", args2, 2, 4194304, NULL, t1);
    CU_ASSERT_EQUAL(err6, -1);      

    fclose(fstout);
    char* ans[] = {"08100001", "08100003", "0c100001", "0c100003"};
    // check_lines_equal(ans, 4);
    // free_table(t1);


}

void test_branch() {
    FILE* fstout = fopen(TMP_FILE, "w");
    if (!fstout) {
        CU_FAIL("Could not open temporary file");
        return;
    }

    SymbolTable* t1 = create_table(1);
    add_to_table(t1, "test1", 4194304);
    add_to_table(t1, "test2", 4194300);
    char *args1[3] = {"$a0", "$a0", "test1"};
    char *args2[3] = {"$a0", "$zero", "test2"};
    char *args3[3] = {"0", "$zero", "test2"};
    char *args4[3] = {"$a0", "$zero", "test3"};


    int err1 = translate_inst(fstout, "beq", args1, 3, 4194312, t1, NULL);
    CU_ASSERT_EQUAL(err1, 0);
    int err2 = translate_inst(fstout, "bne", args2, 3, 4194316, t1, NULL);
    CU_ASSERT_EQUAL(err2, 0);
    int err3 = translate_inst(fstout, "bne", args3, 3, 4194316, t1, NULL);
    CU_ASSERT_EQUAL(err3, -1);
    int err4 = translate_inst(fstout, "bne", args4, 3, 4194316, t1, NULL);
    CU_ASSERT_EQUAL(err4, -1);

    fclose(fstout);
    // char* ans[] = {"1084fffd", "1480fffb"};
    // check_lines_equal(ans, 2);

}

void test_write_pass_one() {
    FILE* fstout = fopen(TMP_FILE, "w");
    if (!fstout) {
        CU_FAIL("Could not open temporary file");
        return;
    }
    SymbolTable* t1 = create_table(1);
    add_to_table(t1, "test1", 4194304);
    char *args1[2] = {"$t1", "0"};
    char *args2[2] = {"$t2", "100000"};
    char *args3[2] = {"$a0", "-100"};
    char *args4[3] = {"$a0", "$0", "test1"};
    char *args5[3] = {"$v0", "$a2", "0"};

    int err1 = write_pass_one(fstout, "li", args1, 2);
    CU_ASSERT_EQUAL(err1, 1);
    int err2 = write_pass_one(fstout, "li", args2, 2);
    CU_ASSERT_EQUAL(err2, 2);
    int err3 = write_pass_one(fstout, "li", args3, 2);
    CU_ASSERT_EQUAL(err3, 1);
    int err4 = write_pass_one(fstout, "blt", args4, 3);
    CU_ASSERT_EQUAL(err4, 2);
    int err5 = write_pass_one(fstout, "addiu", args5, 3);
    CU_ASSERT_EQUAL(err5, 1);
    int err6 = write_pass_one(fstout, "li", args1, 3);
    CU_ASSERT_EQUAL(err6, 0);

    fclose(fstout);
    char* ans[] = {"addiu $t1 $0 0", "lui $at 1", "ori $t2 $at 34464", "addiu $a0 $0 -100", "slt $at $a0 $0", "bne $at $0 test1", "addiu $v0 $a2 0"};
    check_lines_equal(ans, 7); 

}



/****************************************
 *  Add your test cases here
 ****************************************/

int main(int argc, char** argv) {
    CU_pSuite pSuite1 = NULL, pSuite2 = NULL, pSuite3 = NULL, pSuite4 = NULL;

    if (CUE_SUCCESS != CU_initialize_registry()) {
        return CU_get_error();
    }

    /* Suite 1 */
    pSuite1 = CU_add_suite("Testing translate_utils.c", NULL, NULL);
    if (!pSuite1) {
        goto exit;
    }
    if (!CU_add_test(pSuite1, "test_translate_reg", test_translate_reg)) {
        goto exit;
    }
    if (!CU_add_test(pSuite1, "test_translate_num", test_translate_num)) {
        goto exit;
    }

    /* Suite 2 */
    pSuite2 = CU_add_suite("Testing tables.c", init_log_file, NULL);
    if (!pSuite2) {
        goto exit;
    }
    if (!CU_add_test(pSuite2, "test_table_1", test_table_1)) {
        goto exit;
    }
    if (!CU_add_test(pSuite2, "test_table_2", test_table_2)) {
        goto exit;
    }


    /* Suite 3 */
    pSuite3 = CU_add_suite("Testing translate.c", init_log_file, NULL);
    if (!pSuite3) {
        goto exit;
    }
    if (!CU_add_test(pSuite3, "test_rtype", test_rtype)) {
        goto exit;
    }
    if (!CU_add_test(pSuite3, "test_shift", test_shift)) {
        goto exit;
    }
    if (!CU_add_test(pSuite3, "test_addiu", test_addiu)) {
        goto exit;
    }
    if (!CU_add_test(pSuite3, "test_jr", test_jr)) {
        goto exit;
    }
    if (!CU_add_test(pSuite3, "test_mem", test_mem)) {
        goto exit;
    }
    if (!CU_add_test(pSuite3, "test_lui", test_lui)) {
        goto exit;
    }
    if (!CU_add_test(pSuite3, "test_ori", test_ori)) {
        goto exit;
    }
    if (!CU_add_test(pSuite3, "test_jump", test_jump)) {
        goto exit;
    }
    if (!CU_add_test(pSuite3, "test_branch", test_branch)) {
        goto exit;
    }

    /* Suite 4*/
    pSuite4 = CU_add_suite("Testing translate.c", init_log_file, NULL);
    if (!pSuite4) {
        goto exit;
    }
    if (!CU_add_test(pSuite4, "test_write_pass_one", test_write_pass_one)) {
        goto exit;
    }


    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

exit:
    CU_cleanup_registry();
    return CU_get_error();;
}
