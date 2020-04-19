#include "9cc.h"

int is_alnum(char c) {
	return isalnum(c) || (c == '_');
}

void error(char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);

	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	exit(1);
}

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
Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;
	tok->len = len;
    cur->next = tok;
    return tok;
}

void tokenize(char *p) {
    Token head;
    head.next = NULL;
    Token *cur = &head;

    while (*p) {
		if (isspace(*p)) {
	    	p++;
	    	continue;
		}

		// return
		if (!strncmp(p, "return", 6) && !is_alnum(p[6])) {
			cur = new_token(TK_RETURN, cur, "return", 6);
			p += 6;
			continue;
		}

		// if
		if (!strncmp(p, "if", 2) && !is_alnum(p[2])) {
			cur = new_token(TK_IF, cur, "if", 2);
			p += 2;
			continue;
		}

		// else
		if (!strncmp(p, "else", 4) && !is_alnum(p[4])) {
			cur = new_token(TK_ELSE, cur, "else", 4);
			p += 4;
			continue;
		}

		// while
		if (!strncmp(p, "while", 5) && !is_alnum(p[5])) {
			cur = new_token(TK_WHILE, cur, "while", 5);
			p += 5;
			continue;
		}

		// for
		if (!strncmp(p, "for", 3) && !is_alnum(p[3])) {
			cur = new_token(TK_FOR, cur, "for", 3);
			p += 3;
			continue;
		}

		// ２文字の記号
		if (!strncmp(p, "==", 2) ||
	    	!strncmp(p, "!=", 2) ||
	    	!strncmp(p, "<=", 2) ||
	    	!strncmp(p, ">=", 2) ) {
	    	cur = new_token(TK_RESERVED, cur, p, 2);
			p += 2;
	    	continue;
		}

		// １文字の記号
		if (strchr("+-*/()=><;{},&", *p)) {
	    	cur = new_token(TK_RESERVED, cur, p++, 1);
	    	continue;
		}

		// "alpha (alpha | num)*" からなる識別子
		if (isalpha(*p) || *p == '_') {
			char *tmp = p;
			// p ~ tmp-1 までがアルファベットからなる識別子
			for (tmp; is_alnum(*tmp); tmp++);
			cur = new_token(TK_IDENT, cur, p, tmp - p);
			p = tmp;
			continue;
		}

		// 整数 (int)
		if (isdigit(*p)) {
	    	cur = new_token(TK_NUM, cur, p, 1);
	    	cur->val = strtol(p, &p, 10);
	    	continue;
		}

		// その他
        error_at(cur->str+1, "トークナイズできません");
    }

	// 入力の終わり
    new_token(TK_EOF, cur, p, 1);
    token = head.next;
	return;
}

