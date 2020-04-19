#include "9cc.h"

LVar *locals;
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

// 変数を名前で検索する。見つからなかった場合はNULLを返す。
LVar *find_lvar(Token *tok) {
    for (LVar *var = locals; var; var = var->next){
        if (var->len == tok->len && !memcmp(tok->str, var->name, var->len)){
            return var;
        }
    }
    return NULL;
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
//         | ident [ "(" { assign ("," assign) }? ")" ]?  変数または関数
//         | num
Node *primary() {
    if (consume("(")) {
	    Node *node = expr();
        expect(")");
	    return node;
    } else {
        Token *tok = consume_ident();
        if (tok) {
            // 変数or関数
            Node *node = calloc(1, sizeof(Node));
            if (consume("(")) {
                // 関数
                node->kind = ND_FUNC;
                node->name = calloc(tok->len+1, sizeof(char));
                strncpy(node->name, tok->str, tok->len);
                node->name[tok->len] = '\0';
                expect(")");
            } else {
                // 変数
                node->kind = ND_LVAR;
                // その変数のRBPからのオフセットの情報を得る
                LVar *lvar = find_lvar(tok);
                if (lvar) {
                    node->offset = lvar->offset;
                } else {
                    // 同時にLVar *localsの連結リストを作成
                    lvar = calloc(1, sizeof(LVar));
                    lvar->next = locals;
                    lvar->name = tok->str;
                    lvar->len = tok->len;
                    lvar->offset = locals->offset + 8;
                    locals = lvar;
                    node->offset = lvar->offset;
                }
            }
            return node;
        }
        return new_node_num(expect_number());
    }
}	 

// unary = ("+" | "-")? primary
// 単項の+, -。 -x は 0-x に変換
Node *unary() {
    if (consume("+"))
	    return primary();
    else if (consume("-"))
	    return new_node(ND_SUB, new_node_num(0), primary());
    return primary();
}   

// mul = unary ("*" unary | "/" unary)*
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

// add = mul ("+" mul | "-" mul)*
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

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
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

// equality = relational ("==" relational | "!=" relational)*
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

// assign = equality ("=" assign)?
Node *assign() {
    Node *node = equality();
    if (consume("="))
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
        expect(";");
        return node;
    } else if (consume_if()) {
        // if (cond) lhs else rhs
        node = calloc(1, sizeof(Node));
        node->kind = ND_IF;
        expect("(");
        node->cond = expr(); 
        expect(")");
        node->lhs = stmt();
        if (consume_else()) {
            node->rhs = stmt();
        }
        return node;
    } else if (consume_while()) {
        // while (cond) lhs
        node = calloc(1, sizeof(Node));
        node->kind = ND_WHILE;
        expect("(");
        node->cond = expr();
        expect(")");
        node->lhs = stmt();
        return node;
    } else if (consume_for()) {
        // for (lhs; cond; rhs) body
        node = calloc(1, sizeof(Node));
        node->kind = ND_FOR;
        expect("(");
        if (!consume(";")) {
            node->lhs = expr();
            expect(";");
        }
        if (!consume(";")) {
            node->cond = expr();
            expect(";");
        }
        if (!consume(")")) {
            node->rhs = expr();
            expect(")");
        }
        node->body = stmt();
        return node;
    } else if (consume("{")) {
        node = calloc(1, sizeof(Node));
        int i = 0;
        while(!consume("}")){
            node->stmts[i++] = stmt();
        }
        node->stmts[i] = 0;
        node->kind = ND_BLOCK;
        return node;
    } else {
        node = expr();
        expect(";");
        return node;
    }
}

// program = stmt*
// stmt毎にNode *code[100]に格納
void program() {
    int i = 0;
    while(!at_eof())
        code[i++] = stmt();
    code[i] = NULL;
}
