#include "9cc.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

// 指定されたファイルの内容を返す
char *read_file(char *path) {
  // ファイルを開く
  FILE *fp = fopen(path, "r");
  if (!fp)
    error("cannot open %s: %s", path, strerror(errno));

  // ファイルの長さを調べる
  if (fseek(fp, 0, SEEK_END) == -1)
    error("%s: fseek: %s", path, strerror(errno));
  size_t size = ftell(fp);
  if (fseek(fp, 0, SEEK_SET) == -1)
    error("%s: fseek: %s", path, strerror(errno));

  // ファイル内容を読み込む
  char *buf = calloc(1, size + 2);
  fread(buf, size, 1, fp);

  // ファイルが必ず"\n\0"で終わっているようにする
  if (size == 0 || buf[size - 1] != '\n')
    buf[size++] = '\n';
  buf[size] = '\0';
  fclose(fp);
  return buf;
}

// 入力ファイル名
char *filename;

// エラーの起きた場所を報告するための関数
// 下のようなフォーマットでエラーメッセージを表示する
//
// foo.c:10: x = y + + 5;
//                   ^ 式ではありません
void error_at(char *loc, char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
  // locが含まれている行の開始地点と終了地点を取得
  char *line = loc;
  while (user_input < line && line[-1] != '\n')
    line--;

  char *end = loc;
  while (*end != '\n')
    end++;

  // 見つかった行が全体の何行目なのかを調べる
  int line_num = 1;
  for (char *p = user_input; p < line; p++)
    if (*p == '\n')
      line_num++;

  // 見つかった行を、ファイル名と行番号と一緒に表示
  int indent = fprintf(stderr, "%s:%d: ", filename, line_num);
  fprintf(stderr, "%.*s\n", (int)(end - line), line);

  // エラー箇所を"^"で指し示して、エラーメッセージを表示
  int pos = loc - line + indent;
  fprintf(stderr, "%*s", pos, ""); // pos個の空白を出力
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// 与えられた文字がアルファベット、数字、_(アンダースコア)の時真を返す
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
	char *tmpchar;

    while (*p) {
		if (isspace(*p)) {
	    	p++;
	    	continue;
		}

		// return
		if (!strncmp(p, "return", 6) && !is_alnum(p[6])) {
			cur = new_token(TK_RETURN, cur, p, 6);
			p += 6;
			continue;
		}

		// if
		if (!strncmp(p, "if", 2) && !is_alnum(p[2])) {
			cur = new_token(TK_IF, cur, p, 2);
			p += 2;
			continue;
		}

		// else
		if (!strncmp(p, "else", 4) && !is_alnum(p[4])) {
			cur = new_token(TK_ELSE, cur, p, 4);
			p += 4;
			continue;
		}

		// while
		if (!strncmp(p, "while", 5) && !is_alnum(p[5])) {
			cur = new_token(TK_WHILE, cur, p, 5);
			p += 5;
			continue;
		}

		// for
		if (!strncmp(p, "for", 3) && !is_alnum(p[3])) {
			cur = new_token(TK_FOR, cur, p, 3);
			p += 3;
			continue;
		}

		// int
		if (!strncmp(p, "int", 3) && !is_alnum(p[3])) {
			cur = new_token(TK_INT, cur, p, 3);
			p += 3;
			continue;
		}

		// char
		if (!strncmp(p, "char", 4) && !is_alnum(p[4])) {
			cur = new_token(TK_CHAR, cur, p, 4);
			p += 4;
			continue;
		}

		// sizeof
		if (!strncmp(p, "sizeof", 6) && !is_alnum(p[6])) {
			cur = new_token(TK_SIZEOF, cur, p, 6);
			p += 6;
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

		if (!strncmp(p, "\"", 1)) {
			tmpchar = p+1;
			// p+1 ~ tmpchar-1 までが "quote" の中身
			for (; *tmpchar != '\"'; tmpchar++) {
				if (*tmpchar == '\0') {
					error_at(p, "クオーテーションが閉じていません");
				}
			}
			cur = new_token(TK_STR, cur, p+1, tmpchar-p-1);
			p = tmpchar+1;
			continue;
		}

		// １文字の記号
		if (strchr("+-*/()=><;{},&[]", *p)) {
	    	cur = new_token(TK_RESERVED, cur, p++, 1);
	    	continue;
		}

		// 識別子 ( first charcter is alphabet or '_' and 
		// following charcteres can be either alphabet, number, or '_')
		if (isalpha(*p) || *p == '_') {
			tmpchar = p;
			// p ~ tmpchar-1 までが識別子
			for (; is_alnum(*tmpchar); tmpchar++);
			cur = new_token(TK_IDENT, cur, p, tmpchar - p);
			p = tmpchar;
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

void tokenizefile(char *path) {
	filename = path;
	user_input = read_file(path);
	tokenize(user_input);
	return;
}
