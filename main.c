#include <stdio.h>
#include <stdbool.h>
#include "9cc.h"

char *user_input;
Token *token;

int main(int argc, char **argv){
    if (argc != 2) {
	error("引数の個数が正しくありません");
	return 1;
    }

    // error_at関数で使う。入力プログラム
    user_input = argv[1];
    // トークナイズしてパースする
    token = tokenize(argv[1]);
    Node *node = expr();
    
    // アセンブリの前半部分を出力
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");
    
    // 抽象構文木を下りながらコード生成
    gen(node);
    
    // スタックトップに式全体の値がある
    // RAXにロードして関数からの返り値とする
    printf("    pop rax\n");
    printf("    ret\n");
    return 0;
}
