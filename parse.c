#include "9cc.h"

LVar *locals;
Node *code[100];

bool consume(char *s, TokenKind TK) {
    if (token->kind != TK ||
        strlen(s) != token->len ||
        memcmp(token->str, s, token->len))
        return false;
    else {
        token = token->next;
        return true;
    }
}

void expect(char *s, TokenKind TK) {
    if (token->kind != TK ||
        strlen(s) != token->len ||
	    memcmp(token->str, s, token->len))
	    error_at(token->str, "'%s'ではありません", s);
    else 
	    token = token->next;
}

// 次のトークンが期待している記号の時には、トークンを１つ読み進めて
// 真を返す。それ以外の場合には偽を返す。
bool consume_op(char *op) {
    consume(op, TK_RESERVED);
}

// 次のトークンが期待している記号の時には、トークンを１つ読み進める。
// それ以外の場合にはエラーを報告する。
void expect_op(char *op) {
    expect(op, TK_RESERVED);
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

// 次のトークンが識別子の場合そのトークンを返す。
// それ以外の場合は 0 を返す。
Token *consume_ident() {
    if (token->kind != TK_IDENT) {
        return 0;
    } else {
        Token *tok = token;
        token = token->next;
        return tok;
    }
}

// 変数を名前で検索する。見つからなかった場合はerrorを返す。
LVar *find_lvar(Token *tok) {
    for (LVar *var = locals; var; var = var->next){
        if (var->len == tok->len && !memcmp(tok->str, var->name, var->len)){
            return var;
        }
    }
    char *undef_var = calloc(tok->len+1, sizeof(char));
    strncpy(undef_var, tok->str, tok->len);
    undef_var[tok->len] = '\0';
    error("%s: 定義されていない変数です.", undef_var);
}

// 宣言されたローカル変数のオフセットを決める offsetはグローバル変数
LVar *set_lvar(Token *tok) {
    LVar *lvar = calloc(1, sizeof(LVar));
    lvar->next = locals;
    lvar->name = tok->str;
    lvar->len = tok->len;
    lvar->offset = locals->offset + 8;
    locals = lvar;
    return lvar;
}

// 
int consume_return() {
    if (token->kind != TK_RETURN || token->len != 6) {
        return 0;
    } else {
        token = token->next;
        return 1;
    }
}

int consume_if() {
    if (token->kind != TK_IF || token->len != 2) {
        return 0;
    } else {
        token = token->next;
        return 1;
    }
}

int consume_else() {
    if (token->kind != TK_ELSE || token->len != 4) {
        return 0;
    } else {
        token = token->next;
        return 1;
    }
}

int consume_while() {
    if (token->kind != TK_WHILE || token->len != 5) {
        return 0;
    } else {
        token = token->next;
        return 1;
    }
}

int consume_for() {
    if (token->kind != TK_FOR || token->len != 3) {
        return 0;
    } else {
        token = token->next;
        return 1;
    }
}

int consume_int() {
    if (token->kind != TK_INT || token->len != 3) {
        return 0;
    } else {
        token = token->next;
        return 1;
    }
}

// 次のトークンが入力の終わりである時trueを返す
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

// primary = "(" expr ")" 
//         | "int" ident  変数名 定義
//         | ident [ "(" { assign ("," assign)* }? ")" ]? 変数or関数呼び出し
//         | num
Node *primary() {
    Token *tok;
    Node *node;
    LVar *lvar;
    int i;
    if (consume_op("(")) {
    // "(" expr ")"
	    node = expr();
        expect_op(")");
	    return node;
    } else {
        node = calloc(1, sizeof(Node));
        if (consume_int()) {
            // 変数の宣言
            tok = consume_ident();
            if (!tok) error("宣言の後が識別子として不正です");
            lvar = set_lvar(tok);
            node->kind = ND_LVAR;
            node->offset = lvar->offset;
            return node;
        } else if (tok = consume_ident()) {
            // 変数or関数呼び出し
            if (!consume_op("(")) {
                // 宣言無しの変数
                lvar = find_lvar(tok); // 宣言済みかの確認
                node->kind = ND_LVAR;
                node->offset = lvar->offset;
                return node;
            } else {
                // 関数の呼び出し
                node->kind = ND_FUNCALL;
                // 関数名の文字列
                node->name = calloc(tok->len+1, sizeof(char));
                strncpy(node->name, tok->str, tok->len);
                node->name[tok->len] = '\0';
                if (!consume_op(")")) {
                    // 引数あり
                    i = 0;
                    node->fargs[i++] = assign();
                    while (!consume_op(")") && i<6) {
                        expect_op(",");
                        node->fargs[i++] = assign();
                    }
                }
                return node;
            }
        } else {
            // 数字
            return new_node_num(expect_number());
        }
    }
}	 

// unary = ("+" | "-")? primary
//       | ("*" | "&") unary
// 単項の+, -。 -x は 0-x に変換
Node *unary() {
    if (consume_op("+"))
	    return primary();
    else if (consume_op("-"))
	    return new_node(ND_SUB, new_node_num(0), primary());
    else if (consume_op("*")) {
        Node *node = calloc(1, sizeof(Node));
        node->kind = ND_DEREF;
        node->lhs = unary();
        return node;
    } else if (consume_op("&")) {
        Node *node = calloc(1, sizeof(Node));
        node->kind = ND_ADDR;
        node->lhs = unary();
        return node;
    }
    return primary();
}   

// mul = unary ("*" unary | "/" unary)*
Node *mul() {
    Node *node = unary();
    for (;;) {
	    if (consume_op("*"))
	        node = new_node(ND_MUL, node, unary());
	    else if (consume_op("/"))
	        node = new_node(ND_DIV, node, unary());
	    else
	        return node;
    }
}

// add = mul ("+" mul | "-" mul)*
Node *add() {
    Node *node = mul();
    for (;;) {
	    if (consume_op("+"))
	        node = new_node(ND_ADD, node, mul());
	    else if (consume_op("-"))
	        node = new_node(ND_SUB, node, mul());
    	else
	        return node;
    }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
// アセンブリコードで setl, setle のみを扱うので
// 大なりは両辺を入れ替えて小なりとして扱う.
Node *relational() {
    Node *node = add();
    for (;;) {
	    if (consume_op(">="))
            node = new_node(ND_LEQ, add(), node);
	    else if (consume_op("<="))
	        node = new_node(ND_LEQ, node, add());
	    else if (consume_op(">"))
	        node = new_node(ND_LESS, add(), node);
	    else if (consume_op("<"))
	        node = new_node(ND_LESS, node, add());
	    else
	        return node;
    }
}

// equality = relational ("==" relational | "!=" relational)*
Node *equality() {
    Node *node = relational();
    for (;;) {
	if (consume_op("=="))
	    node = new_node(ND_EQ, node, relational());
        else if (consume_op("!="))
	    node = new_node(ND_NEQ, node, relational());
	else
	    return node;
    }
}

// assign = equality ("=" assign)?
Node *assign() {
    Node *node = equality();
    if (consume_op("="))
        node = new_node(ND_ASSIGN, node, assign());
    return node;
}

// expr = assign
Node *expr() {
    return assign();
}

// stmt = expr ";" 
//      | "return" expr ";"
//      | "if" "(" expr ")" stmt ("else" stmt)?
//      | "while" "(" expr ")" stmt
//      | "for" "(" expr? ";" expr? ";" expr? ")" stmt
//      | "{" stmt* "}"
Node *stmt() {
    Node *node;
    if (consume_return()) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_RETURN;
        node->lhs = expr();
        expect_op(";");
        return node;
    } else if (consume_if()) {
        // if (cond) lhs else rhs
        node = calloc(1, sizeof(Node));
        node->kind = ND_IF;
        expect_op("(");
        node->cond = expr(); 
        expect_op(")");
        node->lhs = stmt();
        if (consume_else()) {
            node->rhs = stmt();
        }
        return node;
    } else if (consume_while()) {
        // while (cond) lhs
        node = calloc(1, sizeof(Node));
        node->kind = ND_WHILE;
        expect_op("(");
        node->cond = expr();
        expect_op(")");
        node->lhs = stmt();
        return node;
    } else if (consume_for()) {
        // for (lhs; cond; rhs) body
        node = calloc(1, sizeof(Node));
        node->kind = ND_FOR;
        expect_op("(");
        if (!consume_op(";")) {
            node->lhs = expr();
            expect_op(";");
        }
        if (!consume_op(";")) {
            node->cond = expr();
            expect_op(";");
        }
        if (!consume_op(")")) {
            node->rhs = expr();
            expect_op(")");
        }
        node->body = stmt();
        return node;
    } else if (consume_op("{")) {
        node = calloc(1, sizeof(Node));
        int i = 0;
        while(!consume_op("}")){
            node->stmts[i++] = stmt();
        }
        node->stmts[i] = 0;
        node->kind = ND_BLOCK;
        return node;
    } else {
        node = expr();
        expect_op(";");
        return node;
    }
}

// deffunc = "int" ident "(" ["int" ident ("," "int" ident)* ]? ")" "{" stmt* "}"
Node *deffunc() {
    expect("int", TK_INT);
    Token *tok = consume_ident();
    if (!tok) error("関数名が不正です");
    locals = calloc(1, sizeof(LVar)); // 関数毎にローカル変数を持つ
    Node *node = calloc(1, sizeof(Node));
    int i = 0;

    node->kind = ND_FUNCDEF;

    // 関数名文字列
    node->name = calloc(tok->len+1, sizeof(char));
    strncpy(node->name, tok->str, tok->len);
    node->name[tok->len] = '\0';

    expect_op("(");
    for (i=0; consume("int", TK_INT) && i<6; i++) {
        tok = consume_ident();
        node->fargs[i] = calloc(1, sizeof(Node));
        node->fargs[i]->kind = ND_LVAR;
        // 引数をlocal variableとして, RBPからのoffsetを求める
        LVar *lvar = set_lvar(tok);
        node->fargs[i]->offset = lvar->offset;
        if (!consume_op(",")) {
            break;
        } 
    }
    expect_op(")");
    expect_op("{");
    i = 0;
    while (!consume_op("}")) {
        node->stmts[i++] = stmt();
        if (at_eof()) {
            expect_op("}");
        }
    }
    return node;
}

// program = deffunc*
// deffunc毎にNode *code[100]に格納
void program() {
    int i = 0;
    while(!at_eof())
        code[i++] = deffunc();
    code[i] = NULL;
}
