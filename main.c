#include "9cc.h"

char *user_input;
Token *token;
Node *code[100];

int main(int argc, char **argv){
    if (argc != 2) {
	error("引数の個数が正しくありません");
	return 1;
    }

    // error_at関数で使う。入力プログラム
    user_input = argv[1];
    // トークナイズしてパースする
    token = tokenize(argv[1]);
    program();
    
    // アセンブリの前半部分を出力
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    // プロローグ
    // 変数26個分の領域を確保する
    printf("    push rbp\n");
    printf("    mov rbp, rsp\n");
    printf("    sub rsp, 208\n");
    
    // 先頭の式から順に
    // 抽象構文木を下りながらコード生成
    for (int i = 0; code[i]; i++) {
        gen(code[i]);

        // 式の評価結果としてスタックにひとつの値が残っているはずなので、
        // スタックが溢れないようにポップしておく
        printf("    pop rax\n");
    }
    
    // エピローグ
    // スタックトップに式全体の値がある
    // RAXにロードして関数からの返り値とする
    printf("    mov rsp, rbp\n");
    printf("    pop rbp\n");
    printf("    ret\n");
    return 0;
}
