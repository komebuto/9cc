#include <ctype.h>
#include <stdio.h> 
#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

extern char *user_input;

// TokenKind
typedef enum {
    TK_RESERVED, // 記号
    TK_IDENT,    // 識別子
    TK_NUM,      // 整数
    TK_EOF,      // 入力の終わりを表すトークン
} TokenKind;

// Token
typedef struct Token Token;
struct Token {
    TokenKind kind;
    Token *next;
    int val;
    char *str;
    int len;
};

// 注目しているToken
extern Token *token;

// エラーを報告する関数
void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);

// Tokenを読み進めながら内容を判定するための関数
bool consume(char *op);
void expect(char *op);
int expect_number();
bool at_eof();

// 新しいトークンを作る関数
Token *new_token(TokenKind kind, Token *cur, char *str, int len);
// user_inputをトークナイズして先頭のトークンをtokenに代入
void tokenize(void);

// NodeKind
typedef enum {
    ND_NUM,     // number
    ND_LVAR,    // local variable
    ND_ADD,     // +
    ND_SUB,     // -
    ND_MUL,     // *
    ND_DIV,     // /
    ND_ASSIGN,  // =
    ND_EQ,      // ==
    ND_NEQ,     // !=
    ND_LESS,    // <
    ND_LEQ,     // <=
} NodeKind;

// Node
typedef struct Node Node;
struct Node {
    NodeKind kind; // ノードの型
    Node *lhs;     // left hand side
    Node *rhs;     // right hand side
    int val;       // kindがND_NUMの場合の値
    int offset;    // kindがND_LVARの場合のみ使用
};

// 新しいノードを作る関数
Node *new_node(NodeKind kind, Node *lhs, Node *rhs);
Node *new_node_num(int val);

// パーサーで使う関数
void program(void);
Node *stmt(void);
Node *expr(void);
Node *assign(void);
Node *equality(void);
Node *relational(void); 
Node *add(void);
Node *mul(void);
Node *unary(void);
Node *primary(void);

// コードジェネレーター
void gen(Node *node);

// argv[1]に与えられたコードをコンパイル
int main(int argc, char **argv);
