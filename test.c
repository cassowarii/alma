#include <stdlib.h>
#include <stdio.h>
#include <check.h>
#include "alma.h"
#include "ast.h"
#include "ustrings.h"
#include "eval.h"
#include "scope.h"
#include "lib.h"
#include "parse.h"
#include "compile.h"
#include "registry.h"
#include "grammar.tab.h"

#define ALMATESTINTRO(filename) \
    FILE *in = fopen(filename, "r"); \
    ADeclSeqNode *program = NULL; \
    ASymbolTable symtab = NULL; \
    AStack *stack = stack_new(20); \
    AScope *lib_scope = scope_new(NULL); \
    AScope *scope = scope_new(lib_scope); \
    AFuncRegistry *reg = registry_new(20); \
    parse_file(in, &program, &symtab); \
    lib_init(symtab, lib_scope);

#define ALMATESTCLEAN() \
    free_registry(reg); \
    free_stack(stack); \
    free_decl_seq(program); \
    free_scope(scope); \
    free_lib_scope(lib_scope); \
    free_symbol_table(&symtab);

int ustr_check(AUstr *ustr, const char *test) {
    AUstr *tester = parse_string(test, strlen(test));
    int result = ustr_eq(ustr, tester);
    free_ustring(tester);
    return result;
}

START_TEST(test_stack_push) {
    ALMATESTINTRO("tests/simplepush.alma");

    ck_assert(program->first != NULL);

    ACompileStatus stat = compile(scope, reg, program);
    ck_assert_int_eq(stat, compile_success);

    eval_sequence(stack, scope, program->first->node);

    ck_assert_int_eq(stack->size, 5);
    ck_assert(stack_get(stack, 0)->type == str_val);
    ck_assert(ustr_check(stack_get(stack, 0)->data.str, "hello world"));
    ck_assert(stack_get(stack, 1)->type == int_val);
    ck_assert(stack_get(stack, 1)->data.i == 1);
    ck_assert(stack_get(stack, 2)->type == int_val);
    ck_assert(stack_get(stack, 2)->data.i == 2);
    ck_assert(stack_get(stack, 3)->type == int_val);
    ck_assert(stack_get(stack, 3)->data.i == 3);
    ck_assert(stack_get(stack, 4)->type == int_val);
    ck_assert(stack_get(stack, 4)->data.i == 4);

    ALMATESTCLEAN();
} END_TEST

START_TEST(test_stack_push2) {
    ALMATESTINTRO("tests/simplepush.alma");

    ck_assert(program->first->next != NULL);

    eval_sequence(stack, scope, program->first->next->node);

    ck_assert_int_eq(stack->size, 3);
    ck_assert(stack_get(stack, 0)->type == proto_block);
    ck_assert(stack_get(stack, 1)->type == proto_list);
    ck_assert(stack_get(stack, 2)->type == sym_val);
    ck_assert_str_eq(stack_get(stack, 2)->data.sym->name, "asdf");

    ALMATESTCLEAN();
} END_TEST

START_TEST(test_stack_pop_print) {
    ALMATESTINTRO("tests/simplepop.alma");

    ACompileStatus stat = compile(scope, reg, program);
    ck_assert_int_eq(stat, compile_success);

    printf("\nThe next thing printed should be ‘hi4’.\n");

    eval_sequence(stack, scope, program->first->node);

    ck_assert_int_eq(stack->size, 0);

    ALMATESTCLEAN();
} END_TEST

START_TEST(test_addition) {
    ALMATESTINTRO("tests/basicmath.alma");

    ACompileStatus stat = compile(scope, reg, program);
    ck_assert_int_eq(stat, compile_success);

    eval_sequence(stack, scope, program->first->node);

    ck_assert_int_eq(stack->size, 1);
    ck_assert_int_eq(stack_get(stack, 0)->data.i, 9);

    ALMATESTCLEAN();
} END_TEST

START_TEST(test_addition2) {
    ALMATESTINTRO("tests/basicmath.alma");

    ACompileStatus stat = compile(scope, reg, program);
    ck_assert_int_eq(stat, compile_success);

    eval_sequence(stack, scope, program->first->next->node);

    ck_assert_int_eq(stack->size, 1);
    ck_assert_int_eq(stack_get(stack, 0)->data.i, 27);

    ALMATESTCLEAN();
} END_TEST

START_TEST(test_apply) {
    ALMATESTINTRO("tests/applyfunc.alma");

    ACompileStatus stat = compile(scope, reg, program);
    ck_assert_int_eq(stat, compile_success);

    AFunc *mainfunc = scope_find_func(scope, symtab, "main");

    ck_assert(mainfunc != NULL);

    eval_word(stack, scope, mainfunc);

    ck_assert_int_eq(stack->size, 1);
    ck_assert_int_eq(stack_get(stack, 0)->data.i, 9);

    ALMATESTCLEAN();
} END_TEST

START_TEST(test_duplicate_func_error) {
    ALMATESTINTRO("tests/dupfunc.alma");

    printf("\nThe next thing printed should be an error message.\n");

    ACompileStatus stat = compile(scope, reg, program);
    /* Compilation should fail due to duplicate function name. */
    ck_assert_int_eq(stat, compile_fail);

    ALMATESTCLEAN();
} END_TEST

START_TEST(test_unknown_func_error) {
    ALMATESTINTRO("tests/unknownfunc.alma");

    printf("\nThe next thing printed should be an error message.\n");

    ACompileStatus stat = compile(scope, reg, program);
    /* Compilation should fail due to unknown function name. */
    ck_assert_int_eq(stat, compile_fail);

    ALMATESTCLEAN();
} END_TEST

START_TEST(test_definition) {
    ALMATESTINTRO("tests/definition.alma");

    ACompileStatus stat = compile(scope, reg, program);
    ck_assert_int_eq(stat, compile_success);

    eval_sequence(stack, scope, program->first->node);

    ck_assert_int_eq(stack->size, 1);
    ck_assert_int_eq(stack_get(stack, 0)->data.i, 24);

    ALMATESTCLEAN();
} END_TEST

START_TEST(test_let) {
    ALMATESTINTRO("tests/let.alma");

    ACompileStatus stat = compile(scope, reg, program);
    ck_assert_int_eq(stat, compile_success);

    eval_sequence(stack, scope, program->first->node);

    ck_assert_int_eq(stack->size, 1);
    ck_assert_int_eq(stack_get(stack, 0)->data.i, 12);

    ALMATESTCLEAN();
} END_TEST

Suite *simple_suite(void) {
    Suite *s;
    TCase *tc_core, *tc_comp;

    s = suite_create("Basics");

    /* core test case */
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_stack_push);
    tcase_add_test(tc_core, test_stack_push2);
    tcase_add_test(tc_core, test_stack_pop_print);
    tcase_add_test(tc_core, test_addition);
    tcase_add_test(tc_core, test_addition2);
    tcase_add_test(tc_core, test_apply);
    suite_add_tcase(s, tc_core);

    /* test compilation */
    tc_comp = tcase_create("Compilation");

    tcase_add_test(tc_comp, test_duplicate_func_error);
    tcase_add_test(tc_comp, test_unknown_func_error);
    tcase_add_test(tc_comp, test_definition);
    tcase_add_test(tc_comp, test_let);
    suite_add_tcase(s, tc_comp);

    return s;
}

int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = simple_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
