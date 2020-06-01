#include "9cc.h"

Func *functions;
Str *strings;
Node *code[100];
Token *token;

Token *consume(TokenKind TK) {
    if(token->kind != TK)
        return false;
    else {
        Token *tok = token;
        token = token->next;
        return tok;
    }
}

Token *consume_op(char *s) {
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

bool is_number() {
    if (token->kind != TK_NUM) {
        return false;
    } else {
        return true;
    }
}

// 与えられたTypeのbyte数を返す
size_t sizeoftype(Type *type) {
    switch (type->ty) {
        case CHAR: return 1;
        case INT: return 4;
        case PTR: return 8;
        case ARRAY: {
            Type *tmptype = type;
            size_t n = 1;
            while (tmptype->ty == ARRAY) {
                n *= tmptype->array_size;
                tmptype = tmptype->ptr_to;
            }
            return n*sizeoftype(tmptype);
        }
    }
}

// 与えられたTypeの一要素分のbyte数を返す
size_t sizeofeltype(Type *type) {
    switch (type->ty) {
        case CHAR: return 1;
        case INT: return 4;
        case PTR: return 8;
        case ARRAY: {
            Type *tmptype = type;
            while (tmptype->ty == ARRAY) {
                tmptype = tmptype->ptr_to;
            }
            return sizeofeltype(tmptype);
        }
    }
}

// 変数を名前でローカル変数の中から検索する。
// 見つからなかった場合は0を返す(未宣言).
LVar *find_lvar(Token *tok) {
    for (LVar *var = locals; var; var = var->next){
        if (var->len == tok->len && !memcmp(tok->str, var->name, var->len)){
            return var;
        }
    }
    return 0;
}

// 宣言されたローカル変数のオフセットを決める localsはグローバル変数
LVar *set_lvar(Token *tok, Type *type) {
    LVar *lvar = calloc(1, sizeof(LVar));   // 新しいローカル変数
    lvar->next = locals;
    lvar->name = tok->str;
    lvar->len = tok->len;
    lvar->type = type;
    LVar *head = lvar;
    for (;;) {
        lvar->offset += sizeoftype(type);
        if (!lvar->next) break;
        lvar = lvar->next;
    }
    locals = head;
    return locals;
}

// 変数をグローバル変数の中から名前で検索。
// 見つからなかったらエラーを報告
GVar *find_gvar(Token *tok) {
    for (GVar *var = globals; var; var = var->next) {
        if (var->len == tok->len && !memcmp(tok->str, var->name, var->len)){
            return var;
        }
    }
    error("%.*s: 定義されていない変数です.", tok->len, tok->str);
}

GVar *set_gvar(Token *tok, Type *type, bool isass, int ass) {
    GVar *gvar = calloc(1, sizeof(GVar));
    gvar->next = globals;
    gvar->name = tok->str;
    gvar->len = tok->len;
    gvar->type = type;
    gvar->isassigned = isass;
    gvar->val = ass;
    globals = gvar;
    return gvar;
}

// 関数を名前で検索する。見つからなかった場合はerrorを返す(未宣言). (ファイルスコープ)
Func *find_func(Token *tok) {
    for (Func *func = functions; func; func = func->next){
        if (func->len == tok->len && !memcmp(tok->str, func->name, func->len)){
            return func;
        }
    }
    return false;

    char *undef_var = calloc(tok->len+1, sizeof(char));
    strncpy(undef_var, tok->str, tok->len);
    undef_var[tok->len] = '\0';
    error("%s: 定義されていない関数です.", undef_var);
}

// 宣言された関数を連結リストfunctionsに格納
void set_func(Token *tok, Type *type) {
    unsigned long n;
    Func *func = calloc(1, sizeof(Func));
    func->next = functions;
    func->name = tok->str;
    func->len = tok->len;
    func->type = type;
    functions = func;
}

// 次のトークンが入力の終わりである時trueを返す
bool at_eof() {
    return token->kind == TK_EOF;
}

Type *read_array2(Type *type) {
    Type head;
    head.ptr_to = NULL;
    Type *cur = &head;
    while (consume_op("[")) {
        Type *tmp = calloc(1, sizeof(Type));
        tmp->ty = ARRAY;
        tmp->array_size = expect_number();
        cur->ptr_to = tmp;
        cur = cur->ptr_to;
        expect_op("]");
    }
    cur->ptr_to = type;
    return head.ptr_to; 
}
Type *read_pointer2(TypeKind tk);
// 宣言された変数の型を調べて引数で渡されたNodeのtypeに格納
void define_type(Node *node, TypeKind tk) {
    Token *tok;
    LVar *lvar;
    Node *nod;
    
    Type *type = read_pointer2(tk);
    tok = consume(TK_IDENT);
    if (!tok) error("識別子として不正です");
    node->type = read_array2(type);
    lvar = set_lvar(tok, node->type);
    node->offset = lvar->offset;
    node->offptr = &(lvar->offset);
    node->kind = ND_LVAR;
}

// 四則演算時の変数の型の引継ぎ
Type *cast_type (Node *node1, Node *node2) {
    Type *type1 = node1->type;
    Type *type2 = node2->type;
    TypeKind ty1 = type1->ty;
    TypeKind ty2 = type2->ty;
    if (ty1 == CHAR) {
        return type2;
    } else if (ty2 == CHAR) {
        return type1;
    } else if (ty1 == INT) {
        return type2;
    } else if (ty2 == INT) {
        return type1;
    } else {
        error("演算の型が不正です");
    }
}

bool aresametype(Type *type1, Type *type2) {
    Type *ty1 = type1;
    Type *ty2 = type2;
    for (;;) {
        if (ty1->ty != ty2->ty ) return false;
        if (ty1->ptr_to && ty2->ptr_to) {
            ty1 = ty1->ptr_to;
            ty2 = ty2->ptr_to;
            continue;
        } 
        return true;
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
    node->type->ty =INT;
    return node;
}

// ("int" | "char")
TypeKind eltype() {
    TypeKind tk;
    if (consume(TK_INT)) tk = INT;
    else if (consume(TK_CHAR)) tk = CHAR;
    else error("eltype が不正です");
    return tk;
}

// ("*")* 
Type *read_pointer2(TypeKind tk) {
    Type head;
    head.ptr_to = NULL;
    Type *cur = &head;
    while (consume_op("*")) {
        Type *tmp = calloc(1, sizeof(Type));
        tmp->ty = PTR;
        tmp->array_size = 1;
        cur->ptr_to = tmp;
        cur = cur->ptr_to;
    }
    cur->ptr_to = calloc(1, sizeof(Type));
    cur->ptr_to->ty = tk;
    return head.ptr_to;
}

Node *readdef(TypeKind tk) {
    Node *node = calloc(1, sizeof(Node));
    Type *type = read_pointer2(tk);
    Token *tok = consume(TK_IDENT);
    if (!tok) error("識別子名が不正です");
    node->name = tok->str;
    node->len = tok->len;
    node->kind = ND_LVAR;
    node->isdef = true;
    type = node->type;
    node->type = read_array2(type);
    LVar *lvar = set_lvar(tok, node->type);
    node->name = lvar->name;
    node->len = lvar->len;
    if (consume_op("=")) {
        Node *nod = calloc(1, sizeof(Node));
        nod->kind = ND_ASSIGN;
        node->lhs = node;
        node->rhs = assign();
        node = nod;
    }
    return node;
}

/* primary = "(" expr ")" ( "[" add "]" )?
           | num ( "[" add "]" )?
           | string ( "[" add "]" )?
           | ident "(" { assign ("," assign)* }? ")"   関数呼び出し
           | ident  ( "[" add "]" )?  変数
           | ("char" | "int") ("*")* ident ("[" num "]")* 変数名 定義 (readdef)
*/
Node *primary() {
    Func *func;
    Token *tok;
    Node *node;
    int i;
    if (consume_op("(")) {
    // "(" expr ")"
	    node = expr();
        expect_op(")");
    } else {
        node = calloc(1, sizeof(Node));
        if (is_number()) {
            node = new_node_num(expect_number());
        } else if (tok = consume(TK_STR)) {
            Str *str = calloc(1, sizeof(Str));
            str->next = strings;
            str->name = tok->str;
            str->len = tok->len + 1;
            str->name[tok->len] = '\0';
            if (str->next) {
                str->id = str->next->id;
            } else {
                str->id = 0;
            }
            strings = str;
            node->kind = ND_STR;
            node->name = str->name;
            node->val = str->id;
            node->type = calloc(1, sizeof(Type));
            node->type->ty = ARRAY;
            node->type->ptr_to = calloc(1, sizeof(Type));
            node->type->ptr_to->ty = CHAR;
            node->type->array_size = str->len;
        } else if (tok = consume(TK_IDENT)) {
            // 変数/関数呼び出し
            if (consume_op("(")) {
                // 関数の呼び出し
                if (func = find_func(tok)) {
                    node->type = func->type;
                } 
                node->kind = ND_FUNCALL;
                // 関数名の文字列
                node->name = tok->str;
                node->len = tok->len;
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
            } else {
                Type type;
                // 宣言無しの変数
                LVar *lvar = find_lvar(tok); // 宣言済みかの確認
                if (lvar) {
                    // ローカル変数
                    node->kind = ND_LVAR;
                    node->offset = lvar->offset;
                    node->offptr = &(lvar->offset);
                    node->type = lvar->type;
                    node->isdef = false;
                } else {
                    // グローバル変数で探す
                    GVar *gvar = find_gvar(tok); // 見つからなかったらここでエラー
                    node->kind = ND_GVAR;
                    node->type = gvar->type;
                    node->name = gvar->name;
                    node->len = gvar->len;
                    node->isdef = false;
                }
            }
        } else {
            Node head;
            Node *curnode = &head;
            Node *tmp;
            // 変数の宣言
            // ("int" | "char") ("*")* ident ("[" num "]")*
            TokenKind tk = eltype();
            for (;;) {
                tmp = calloc(1, sizeof(Node));
                // ("*")*
                Type *type = read_pointer2(tk);

                // ident
                Token *tok = consume(TK_IDENT);
                if (!tok) error("識別子として不正です");

                // ("[" num "]")*
                //read_array(node);
                type = read_array2(type);
                LVar *lvar = set_lvar(tok, type);
                tmp->type = type;
                tmp->offset = lvar->offset;
                tmp->offptr = &(lvar->offset);
                tmp->kind = ND_LVAR;
                tmp->isdef = true;

                //define_type(node, tk);
                if (consume_op("=")) {
                    Node *nod = new_node(ND_ASSIGN, tmp, assign());
                    nod->type = tmp->type;
                    tmp = nod;
                }
                curnode->nextdef = tmp;
                if (!consume_op(",")) { return head.nextdef; break; }
                else { if (tmp->lhs) {curnode = tmp->lhs;} else { curnode = tmp;} }
            }
        }
    }
    while (consume_op("[")) {
        // node [add] == *(node + add)
        Node *nd = calloc(1, sizeof(Node));
        // nd->lhs == node + add
        nd->lhs = new_node(ND_ADD, node, add());
        nd->lhs->type = cast_type(nd->lhs->lhs, nd->lhs->rhs);
        // nd == *(lhs)
        nd->kind = ND_DEREF;
        nd->type = nd->lhs->type->ptr_to;
        node = nd;
        expect_op("]");
    }
    return node;
}	 

// unary = ("+")? primary
//       | ("-") unary
//       | ("*" | "&") unary
//       | "sizeof" unary
// 単項の+, -。 -x は 0-x に変換
Node *unary() {
    if (consume_op("+")) {
	    return primary();
    } else if (consume_op("-")) {
	    Node *node = new_node(ND_SUB, new_node_num(0), unary());
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
        node->type->ty = PTR;
        node->type->ptr_to = node->lhs->type;
        return node;
    } else if (consume(TK_SIZEOF)) {
        Node *node = unary();
        return new_node_num(sizeoftype(node->type));
    } else {
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
            if (aresametype(node->lhs->type, node->rhs->type))
                node->type = node->lhs->type;
            else 
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
            node->type->ty = CHAR;
        } else if (consume_op("<=")) {
	        node = new_node(ND_LEQ, node, add());
            node->type = calloc(1, sizeof(Type));
            node->type->ty = CHAR;
        } else if (consume_op(">")) {
	        node = new_node(ND_LESS, add(), node);
            node->type = calloc(1, sizeof(Type));
            node->type->ty = CHAR;
        } else if (consume_op("<")) {
	        node = new_node(ND_LESS, node, add());
            node->type = calloc(1, sizeof(Type));
            node->type->ty = CHAR;
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
            node->type->ty = CHAR;
        } else if (consume_op("!=")) {
	        node = new_node(ND_NEQ, node, relational());
            node->type = calloc(1, sizeof(Type));
            node->type->ty = CHAR;
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

void setoffset() {
    int max = locals->offset;
    for (;;) {
        locals->offset = max - locals->offset + sizeoftype(locals->type);
        if (!locals->next) break;
        locals = locals->next;
    }
}

/* defvar = ident "(" [("int" | "char") ident ("," ("int" | "char") ident)* ]? ")" "{" stmt* "}"
           | ident ( "=" assign )? ("," defvar | ";")
           | ident ("[" num "]")* ("," defvar | ";")
*/
Node *defvar(TypeKind tk) {
    //TypeKind tk;
    LVar *lvar;
    GVar *gvar;
    Node *node = calloc(1, sizeof(Node));
    int i = 0;

    // ("*")*
    Type *type = read_pointer2(tk);
    // ident
    Token *tok = consume(TK_IDENT);
    if (!tok) error("識別子名が不正です");
    node->name = tok->str;
    node->len = tok->len;

    // 関数宣言
    if (consume_op("(")) {
        locals = calloc(1, sizeof(LVar));
        node->kind = ND_FUNCDEF;
        node->type = type;
        set_func(tok, type);
        // 関数の引数(最大6個まで)
        for (i=0; i<6; i++) {
            if (consume(TK_INT)) tk = INT;
            else if (consume(TK_CHAR)) tk = CHAR;
            else break;
            node->fargs[i] = calloc(1, sizeof(Node));
            node->fargs[i]->kind = ND_LVAR;
            define_type(node->fargs[i], tk);
            if (!consume_op(",")) break; 
        }
        expect_op(")");
        if (consume_op("{")) {
            // 関数定義
            node->isdef = true;
            i = 0;
            while (!consume_op("}")) {
                node->stmts[i++] = stmt();
                if (at_eof()) {
                    expect_op("}");
                }
            }
            for (lvar=locals; lvar->next; lvar=lvar->next);
            node->offset = lvar->offset;
            node->offptr = &(locals->offset);
            //setoffset();
            return node;
        }
        // 関数内でのローカル変数の分のスペース
    } else {
        // グローバル変数
        GVar *gvar;
        bool isassigned;
        int val;
        node->kind = ND_GVAR;
        type = read_array2(type);
        if (consume_op("=")) {
            isassigned = true;
            val = expect_number();
        }
        gvar = set_gvar(tok, type, isassigned, val);
        node->type = type;
        node->name = gvar->name;
        node->len = gvar->len;
        node->isdef = true;
    }
    if (consume_op(",")) node->nextdef = defvar(tk);
    else expect_op(";");
    return node;
}

// defeltype = ("int" | "char") defvar
Node *defeltype() {
    TypeKind tk;
    tk = eltype();
    return defvar(tk);
}

// program = deffunc*
// deffunc毎にNode *code[100]に格納
void program() {
    int i = 0;
    while(!at_eof())
        code[i++] = defeltype();
    code[i] = NULL;
}
