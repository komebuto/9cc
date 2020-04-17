#include "9cc.h"

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

// 新しいトークンを作成してcurにつなげる
Token *new_token(TokenKind kind, Token *cur, char *str) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;
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
	    	cur = new_token(TK_RESERVED, cur, p);
			cur->len = 2;
	    	p += 2;
	    	continue;
		}

		if (strchr("+-*/()=><;", *p)) {
	    	cur = new_token(TK_RESERVED, cur, p++);
			cur->len = 1;
	    	continue;
		}
    
		if ('a' <= *p && *p <= 'z') {
			cur = new_token(TK_IDENT, cur, p++);
			cur->len = 1;
			continue;
		}

		if (isdigit(*p)) {
	    	cur = new_token(TK_NUM, cur, p);
			cur->len = 1;
	    	cur->val = strtol(p, &p, 10);
	    	continue;
		}

        error_at(cur->str+1, "トークナイズできません");
    }

    new_token(TK_EOF, cur, p);
    return head.next;
}

