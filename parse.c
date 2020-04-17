#include <ctype.h>
#include <stdio.h> 
#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "9cc.h"

char *user_input;
Token *token;

// エラーを報告するための関数
// printfと同じ引数を取る
void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

// エラー箇所を報告する
void error_at(char *loc, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    int pos = loc - user_input;
    fprintf(stderr, "%s\n", user_input);
    fprintf(stderr, "%*s", pos, "");
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

// 次のトークンが期待している記号の時には、トークンを１つ読み進めて
// 真を返す。それ以外の場合には偽を返す。
bool consume(char *op) {
    if (token->kind != TK_RESERVED || 
	strlen(op) != token->len ||
	memcmp(token->str, op, token->len))
        return false;
    else {
	token = token->next;
        return true;
    }
}

// 次のトークンが期待している記号の時には、トークンを１つ読み進める。
// それ以外の場合にはエラーを報告する。
void expect(char *op) {
    if (token->kind != TK_RESERVED ||
        strlen(op) != token->len ||
	memcmp(token->str, op, token->len))
	error_at(token->str, "'%c'ではありません", op);
    else 
	token = token->next;
}

// 次のトークンが数値の場合、トークンを１つ読み進めてその数値を返す。
// それ以外の場合にはエラーを報告する。
int expect_number() {
    if (token->kind != TK_NUM) {
	error_at(token->str, "数ではありません");
    } else {
	int val = token->val;
    	token = token->next;
	return val;
    }
}

bool at_eof() {
    return token->kind == TK_EOF;
}

// 新しいトークンを作成してcurにつなげる
Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;
    tok->len = len;
    cur->next = tok;
    return tok;
}

Token *tokenize(char *p) {
    Token head;
    head.next = NULL;
    Token *cur = &head;

    while (*p) {
	if (isspace(*p)) {
	    p++;
	    continue;
	}
        
	if (strncmp(p, "==", 2) == 0 ||
	    strncmp(p, "!=", 2) == 0 ||
	    strncmp(p, "<=", 2) == 0 ||
	    strncmp(p, ">=", 2) == 0) {
	    cur = new_token(TK_RESERVED, cur, p, 2);
	    p += 2;
	    continue;
	}

	if (strchr("+-*/()><", *p)) {
	    cur = new_token(TK_RESERVED, cur, p++, 1);
	    continue;
	}

	if (strchr("+-*/()><", *p)) {
	    cur = new_token(TK_RESERVED, cur, p++, 1);
	    continue;
	}

	if (isdigit(*p)) {
	    cur = new_token(TK_NUM, cur, p, 1);
	    cur->val = strtol(p, &p, 10);
	    continue;
	}
    
        error_at(cur->str+1, "トークナイズできません");
    }

    new_token(TK_EOF, cur, p, 1);
    return head.next;
}

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

