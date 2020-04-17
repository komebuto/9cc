#include <ctype.h>
#include <stdio.h> 
#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

extern char *user_input;

// トークンの種類
typedef enum {
    TK_RESERVED, // 記号
    TK_IDENT,    // 識別子
    TK_NUM,      // 整数
    TK_EOF,      // 入力の終わりを表すトークン
} TokenKind;

typedef struct Token Token;
struct Token {
    TokenKind kind;
    Token *next;
    int val;
    char *str;
    int len;
};

extern Token *token;

void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);

bool consume(char *op);
void expect(char *op);
int expect_number();
bool at_eof();
Token *new_token(TokenKind kind, Token *cur, char *str);
Token *tokenize(char *p);

// Node
typedef enum {
    ND_ADD,  // +
    ND_SUB,  // -
    ND_MUL,  // *
    ND_DIV,  // /
    ND_NUM,  // number
    ND_LVAR, // local variable
    ND_ASSIGN,  // =
    ND_EQ,   // ==
    ND_NEQ,  // !=
    ND_LESS, // <
    ND_LEQ,  // <=
} NodeKind;

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

void gen(Node *node);

int main(int argc, char **argv);
