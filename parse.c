#include "9cc.h"

LVar *locals;
Node *code[100];

bool consume(TokenKind TK) {
    if(token->kind != TK)
        return false;
    else {
        token = token->next;
        return true;
    }
}

bool consume_op(char *s) {
    if (strlen(s) != token->len ||
        memcmp(token->str, s, token->len))
        return false;
    else {
        return consume(TK_RESERVED);
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
//bool consume_op(char *op) {
//    consume(op, TK_RESERVED);
//}

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

// 変数を名前で検索する。見つからなかった場合はerrorを返す(未宣言).
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

// 宣言されたローカル変数のオフセットを決める localsはグローバル変数
LVar *set_lvar(Token *tok, Type *type) {
    unsigned long n;
    LVar *lvar = calloc(1, sizeof(LVar));
    lvar->next = locals;
    lvar->name = tok->str;
    lvar->len = tok->len;
    lvar->type = type;
    lvar->offset = locals->offset;
    locals = lvar;
    if (type->kind == INT) {
        locals->offset += 4;
    } else if (type->kind == PTR) {
        locals->offset += 8;
    } else if (type->kind == ARRAY) {
        n = 1;
        while (type->kind == ARRAY) {
            n *= type->array_size;
            type = type->ptr_to;
        }
        if (type->kind == INT) {
            locals->offset += 4*n;
        } else if (type->kind == PTR) {
            locals->offset += 8*n;
        }
    }
    return lvar;
}

// 次のトークンが入力の終わりである時trueを返す
bool at_eof() {
    return token->kind == TK_EOF;
}

// 宣言された変数の型を調べて引数で渡されたNodeのtypeに格納
void define_type(Node *node) {
    Token *tok;
    LVar *lvar;
    Type *types = calloc(1, sizeof(Type));
    Type *tmp;
    node->type = types;
    while (consume_op("*")) {
        types->kind = PTR;
        types->ptr_to = calloc(1, sizeof(Type));
        types = types->ptr_to;
    }
    types->kind = INT;
    tok = consume_ident();
    if (!tok) error("識別子として不正です");
    while (consume_op("[")) {
        tmp = calloc(1, sizeof(Type));
        tmp->kind = ARRAY;
        tmp->ptr_to = node->type;
        tmp->array_size = expect_number();
        node->type = tmp;
        expect_op("]");
    }
    lvar = set_lvar(tok, node->type);
    node->offset = lvar->offset;
    node->kind = ND_LVAR;
}

// 四則演算時の変数の型の引継ぎ
Type *cast_type (Node *node1, Node *node2) {
    Type *type1 = node1->type;
    Type *type2 = node2->type;
    if (type1->kind == PTR && type2->kind == PTR) {
        error("ポインタ同士の計算です");
    } else if (type1->kind == PTR) {
        return type1;
    } else {
        return type2;
    }
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
    node->type = calloc(1, sizeof(Type));
    node->type->kind =INT;
    return node;
}

// primary = "(" expr ")" 
//         | "int" ("*")* ident ("[" num "]")* 変数名 定義
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
        if (consume(TK_INT)) {
            // 変数の宣言
            define_type(node);
            return node;
        } else if (tok = consume_ident()) {
            // 変数or関数呼び出し
            if (!consume_op("(")) {
                // 宣言無しの変数
                lvar = find_lvar(tok); // 宣言済みかの確認
                node->kind = ND_LVAR;
                node->offset = lvar->offset;
                node->type = lvar->type;
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
            node = new_node_num(expect_number());
            return node;
        }
    }
}	 

// unary = ("+" | "-")? primary
//       | ("*" | "&") unary
//       | "sizeof" unary
// 単項の+, -。 -x は 0-x に変換
Node *unary() {
    if (consume_op("+")) {
	    return primary();
    } else if (consume_op("-")) {
	    Node *node = new_node(ND_SUB, new_node_num(0), primary());
        node->type = node->lhs->type;
        return node;
    } else if (consume_op("*")) {
        Node *node = calloc(1, sizeof(Node));
        node->kind = ND_DEREF;
        node->lhs = unary();
        // 参照外し
        node->type = node->lhs->type->ptr_to;
        return node;
    } else if (consume_op("&")) {
        Node *node = calloc(1, sizeof(Node));
        node->kind = ND_ADDR;
        node->lhs = unary();
        // アドレス
        node->type = calloc(1, sizeof(Type));
        node->type->kind = PTR;
        node->type->ptr_to = node->lhs->type;
        return node;
    } else if (consume(TK_SIZEOF)) {
        Node *node = unary();
        if (node->type->kind == INT) {
            return new_node_num(4);
        } else if (node->type->kind = PTR) {
            return new_node_num(8);
        }
    }else {
        return primary();
    }
}   

// mul = unary ("*" unary | "/" unary)*
Node *mul() {
    Node *node = unary();
    for (;;) {
	    if (consume_op("*")) {
	        node = new_node(ND_MUL, node, unary());
            node->type = cast_type(node->rhs, node->lhs);
        } else if (consume_op("/")) {
	        node = new_node(ND_DIV, node, unary());
            node->type = cast_type(node->rhs, node->lhs);
        } else {
	        return node;
        }
    }
}

// add = mul ("+" mul | "-" mul)*
Node *add() {
    Node *node = mul();
    for (;;) {
	    if (consume_op("+")) {
	        node = new_node(ND_ADD, node, mul());
            node->type = cast_type(node->rhs, node->lhs);
        } else if (consume_op("-")) {
	        node = new_node(ND_SUB, node, mul());
            node->type = cast_type(node->rhs, node->lhs);
        } else
	        return node;
    }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
// アセンブリコードで setl, setle のみを扱うので
// 大なりは両辺を入れ替えて小なりとして扱う.
Node *relational() {
    Node *node = add();
    for (;;) {
	    if (consume_op(">=")) {
            node = new_node(ND_LEQ, add(), node);
            node->type = calloc(1, sizeof(Type));
            node->type->kind = INT;
        } else if (consume_op("<=")) {
	        node = new_node(ND_LEQ, node, add());
            node->type = calloc(1, sizeof(Type));
            node->type->kind = INT;
        } else if (consume_op(">")) {
	        node = new_node(ND_LESS, add(), node);
            node->type = calloc(1, sizeof(Type));
            node->type->kind = INT;
        } else if (consume_op("<")) {
	        node = new_node(ND_LESS, node, add());
            node->type = calloc(1, sizeof(Type));
            node->type->kind = INT;
        } else { 
	        return node;
        }
    }
}

// equality = relational ("==" relational | "!=" relational)*
Node *equality() {
    Node *node = relational();
    for (;;) {
	    if (consume_op("==")) {
	        node = new_node(ND_EQ, node, relational());
            node->type = calloc(1, sizeof(Type));
            node->type->kind = INT;
        } else if (consume_op("!=")) {
	        node = new_node(ND_NEQ, node, relational());
            node->type = calloc(1, sizeof(Type));
            node->type->kind = INT;
        } else {
	        return node;
        }
    }
}

// assign = equality ("=" assign)?
Node *assign() {
    Node *node = equality();
    Type *type = node->type;
    if (consume_op("="))
        node = new_node(ND_ASSIGN, node, assign());
        node->type = type;
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
    if (consume(TK_RETURN)) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_RETURN;
        node->lhs = expr();
        expect_op(";");
        return node;
    } else if (consume(TK_IF)) {
        // if (cond) lhs else rhs
        node = calloc(1, sizeof(Node));
        node->kind = ND_IF;
        expect_op("(");
        node->cond = expr(); 
        expect_op(")");
        node->lhs = stmt();
        if (consume(TK_ELSE)){
            node->rhs = stmt();
        }
        return node;
    } else if (consume(TK_WHILE)) {
        // while (cond) lhs
        node = calloc(1, sizeof(Node));
        node->kind = ND_WHILE;
        expect_op("(");
        node->cond = expr();
        expect_op(")");
        node->lhs = stmt();
        return node;
    } else if (consume(TK_FOR)) {
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
    for (i=0; consume(TK_INT) && i<6; i++) {
        node->fargs[i] = calloc(1, sizeof(Node));
        node->fargs[i]->kind = ND_LVAR;
        define_type(node->fargs[i]);
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
