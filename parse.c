#include <ctype.h>
#include <stdio.h> 
#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "9cc.h"

char *user_input;
Token *token;

// 新しいノードを作成する関数
// ノードが記号の場合
Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

// ノードが数の場合
Node *new_node_num(int val) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_NUM;
    node->val = val;
    return node;
}

Node *expr() {
    Node *node = equality();
    return node;
}

Node *equality() {
    Node *node = relational();

    for (;;) {
	if (consume("=="))
	    node = new_node(ND_EQ, node, relational());
        else if (consume("!="))
	    node = new_node(ND_NEQ, node, relational());
	else
	    return node;
    }
}

// 大なり、小なり.
// アセンブリコードで setl, setle のみを扱うので
// 大なりは両辺を入れ替えて小なりとして扱う.
Node *relational() {
    Node *node = add();

    for (;;) {
	if (consume(">="))
            node = new_node(ND_LEQ, add(), node);
	else if (consume("<="))
	    node = new_node(ND_LEQ, node, add());
	else if (consume(">"))
	    node = new_node(ND_LESS, add(), node);
	else if (consume("<"))
	    node = new_node(ND_LESS, node, add());
	else
	    return node;
    }
}

Node *add() {
    Node *node = mul();

    for (;;) {
	if (consume("+"))
	    node = new_node(ND_ADD, node, mul());
	else if (consume("-"))
	    node = new_node(ND_SUB, node, mul());
	else
	    return node;
    }
}

Node *mul() {
    Node *node = unary();

    for (;;) {
	if (consume("*"))
	    node = new_node(ND_MUL, node, unary());
	else if (consume("/"))
	    node = new_node(ND_DIV, node, unary());
	else
	    return node;
    }
}

// 単項の+, -。 -x は 0-x に変換
Node *unary() {
    if (consume("+"))
	return primary();
    else if (consume("-"))
	return new_node(ND_SUB, new_node_num(0), primary());
    return primary();
}


Node *primary() {
    if (consume("(")) {
	Node *node = expr();
        expect(")");
	return node;
    } else {
        return new_node_num(expect_number());
    }
}	    

