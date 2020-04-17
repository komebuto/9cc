#include <stdio.h>
#include <stdbool.h>
#include "9cc.h"

// パーサー
void gen(Node *node) {
    if (node->kind == ND_NUM) {
	printf("    push %d\n", node->val);
	return;
    }

    gen(node->lhs);
    gen(node->rhs);
    
    // popは右辺が先（スタックマシンの上側）
    printf("    pop rdi\n");
    printf("    pop rax\n");

    switch (node->kind) {
	case ND_ADD:
	    printf("    add rax, rdi\n");
	    break;
	case ND_SUB:
	    printf("    sub rax, rdi\n");
	    break;
	case ND_MUL:
	    printf("    imul rax, rdi\n");
	    break;
	case ND_DIV:
	    printf("    cqo\n");
	    printf("    idiv rdi\n");
	    break;
	
	case ND_EQ:
	    printf("    cmp rax, rdi\n");
	    printf("    sete al\n");	   // ALはRAXの下位8ビットを指す 
	    printf("    movzb rax, al\n"); // ALを残して, RAXの上位56ビットをゼロクリア
	    break;
	case ND_NEQ:
	    printf("    cmp rax, rdi\n");
	    printf("    setne al\n");
	    printf("    movzb rax, al\n");
	    break;
	case ND_LESS:
	    printf("    cmp rax, rdi\n");
	    printf("    setl al\n");
	    printf("    movzb rax, al\n");
	    break;
	case ND_LEQ:
	    printf("    cmp rax, rdi\n");
	    printf("    setle al\n");
	    printf("    movzb rax, al\n");
	    break;
    }
    
    printf("    push rax\n");
}
