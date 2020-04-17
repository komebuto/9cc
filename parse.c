#include "9cc.h"

char *user_input;
Token *token;
Node *code[100];

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

char *consume_ident() {
    if (token->kind != TK_IDENT) {
        return 0;
    } else {
        char *str = token->str;
        token = token->next;
        return str;
    }
}

bool at_eof() {
    return token->kind == TK_EOF;
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

Node *primary() {
    if (consume("(")) {
	    Node *node = expr();
        expect(")");
	    return node;
    } else {
        char *str = consume_ident();
        if (str) {
            Node *node = calloc(1, sizeof(Node));
            node->kind = ND_LVAR;
            node->offset = (*str - 'a' + 1) * 8;
            return node;
        }
        return new_node_num(expect_number());
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

Node *assign() {
    Node *node = equality();
    if (consume("="))
        node = new_node(ND_ASSIGN, node, assign());
    return node;
}

Node *expr() {
    return assign();
}

Node *stmt() {
    Node *node = expr();
    expect(";");
    return node;
}

void program() {
    int i = 0;
    while(!at_eof())
        code[i++] = stmt();
    code[i] = NULL;
}
