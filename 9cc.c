#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// トークンの種類
typedef enum{
    TK_RESERVED, // 記号
    TK_NUM,	 // 整数トークン
    TK_EOF,	 // 入力の終わりを表すトークン
} TokenKind;

// これで、struct Token (変数) ではなく、Token (変数) と宣言できる
typedef struct Token Token;

struct Token {
    TokenKind kind; 	// トークンの型
    Token *next;	// 次の入力トークン
    int val;		// kindがTK_NUMの場合、その数値
    char *str;		// トークン文字列
    int len;	 	// トークンの長さ
};

// 現在着目しているトークン
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

// 入力プログラム
char *user_input;

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


// 抽象構文木のノードの種類
typedef enum {
    ND_ADD, // +
    ND_SUB, // -
    ND_MUL, // *
    ND_DIV, // /
    ND_NUM, // 数
    ND_EQ,  // ==
    ND_NEQ, // !=
    ND_LESS,// <
    ND_LEQ, // <=
} NodeKind;

typedef struct Node Node;

// 抽象構文木のノードの型
struct Node {
    NodeKind kind; // ノードの型
    Node *lhs;	   // left hand side
    Node *rhs;	   // right hand side
    int val;	   // kindがND_NUMの場合の値
};

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

Node *equality(), *relational(), *add(), *mul(), *unary(), *primary();

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

// 抽象構文木からコンパイル（スタックマシン）
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


int main(int argc, char **argv){
    if (argc != 2) {
	error("引数の個数が正しくありません");
	return 1;
    }

    // error_at関数で使う。入力プログラム
    user_input = argv[1];
    // トークナイズしてパースする
    token = tokenize(argv[1]);
    Node *node = expr();
    
    // アセンブリの前半部分を出力
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");
    
    // 抽象構文木を下りながらコード生成
    gen(node);
    
    // スタックトップに式全体の値がある
    // RAXにロードして関数からの返り値とする
    printf("    pop rax\n");
    printf("    ret\n");
    return 0;
}
