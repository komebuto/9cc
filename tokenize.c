#include "9cc.h"

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

void tokenize() {
    Token head;
    head.next = NULL;
    Token *cur = &head;

    while (*user_input) {
		if (isspace(*user_input)) {
	    	user_input++;
	    	continue;
		}

		// ２文字の記号
		if (!strncmp(user_input, "==", 2) ||
	    	!strncmp(user_input, "!=", 2) ||
	    	!strncmp(user_input, "<=", 2) ||
	    	!strncmp(user_input, ">=", 2) ) {
	    	cur = new_token(TK_RESERVED, cur, user_input, 2);
	    	user_input += 2;
	    	continue;
		}

		// １文字の記号
		if (strchr("+-*/()=><;", *user_input)) {
	    	cur = new_token(TK_RESERVED, cur, user_input++, 1);
	    	continue;
		}

		// アルファベットからなる識別子
		if (isalpha(*user_input)) {
			char *tmp = user_input;
			// user_input ~ tmp-1 までがアルファベットからなる識別子
			for (tmp; isalpha(*tmp); tmp++);
			cur = new_token(TK_IDENT, cur, user_input, tmp - user_input);
			user_input = tmp;
			continue;
		}

		// 整数 (int)
		if (isdigit(*user_input)) {
	    	cur = new_token(TK_NUM, cur, user_input, 1);
	    	cur->val = strtol(user_input, &user_input, 10);
	    	continue;
		}

		// その他
        error_at(cur->str+1, "トークナイズできません");
    }

	// 入力の終わり
    new_token(TK_EOF, cur, user_input, 1);
    token = head.next;
	return;
}

