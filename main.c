#include "9cc.h"

char *user_input;
Token *token;
LVar *locals;
Node *code[100];

int main(int argc, char **argv){
    if (argc != 2) {
	    error("引数の個数が正しくありません");
	return 1;
    }
    
    locals = calloc(1, sizeof(LVar));  
    
    // 入力プログラム
    user_input = argv[1];

    // トークナイズ => Token *token
    tokenize(user_input);
    // パース => Node *code[100]
    program();

    // アセンブリの前半部分を出力
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    
    // 先頭の関数から順に
    // 抽象構文木を下りながらコード生成
    for (int i = 0; code[i]; i++) {
        gen(code[i]);

        // 式の評価結果としてスタックにひとつの値が残っているはずなので、
        // スタックが溢れないようにポップしておく
        printf("    pop rax\n");
    }
    
    return 0;
}
