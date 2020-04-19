#include <ctype.h>
#include <stdio.h> 
#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

extern unsigned long nbegin;
extern unsigned long nelse;
extern unsigned long nend;
extern unsigned long nrsp;
extern char *user_input;
extern char *reg_arg[6];    // 関数の引数用のレジスタ名

// TokenKind
typedef enum {
    TK_RESERVED, // 記号
    TK_IDENT,    // 識別子
    TK_NUM,      // 整数
    TK_RETURN,   // return
    TK_IF,       // if
    TK_ELSE,     // else
    TK_WHILE,    // while
    TK_FOR,      // for
    TK_EOF,      // 入力の終わりを表すトークン
    TK_INT,      // int型
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

// Local variable
typedef struct LVar LVar;
struct LVar {
    LVar *next; // 次の変数かNULL
    char *name; // 変数の名前
    int len;    // 名前の長さ
    int offset; // RBPからのオフセット
};

extern LVar *locals;

// 新しいトークンを作る関数
Token *new_token(TokenKind kind, Token *cur, char *str, int len);
// user_inputをトークナイズして先頭のトークンをtokenに代入
void tokenize(char *p);

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
    ND_RETURN,  // return
    ND_IF,      // if
    ND_WHILE,   // while
    ND_FOR,     // for
    ND_BLOCK,   // { ... }
    ND_FUNCALL,   // Function call
    ND_FUNCDEF,    // Function define 
    ND_DEREF,   // * (参照外し)
    ND_ADDR,    // & (アドレス)
} NodeKind;

// Node
typedef struct Node Node;
struct Node {
    NodeKind kind;      // ノードの型
    Node *lhs;          // left hand side
    Node *rhs;          // right hand side
    Node *cond;         // 制御文に使用
    Node *body;         // kindがND_FORの場合に使用
    Node *stmts[100];   // ND_BLOCK, ND_FUNCDEFのbody stmt毎に格納
    int val;            // ND_NUM
    int offset;         // ND_LVAR
    char *name;         // ND_FUNC* 関数名文字列
    Node *fargs[6];     // ND_FUNC* 関数の引数 (最大引数6個)
};

// エラーを報告する関数
void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);

// Tokenを読み進めながら内容を判定するための関数
bool consume(char *op);
void expect(char *op);
int expect_number();
bool at_eof();

// 新しいノードを作る関数
Node *new_node(NodeKind kind, Node *lhs, Node *rhs);
Node *new_node_num(int val);

// パーサーで使う関数
void program(void);
Node *func(void);
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
