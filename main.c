#include "9cc.h"

unsigned long nbegin;
unsigned long nelse;
unsigned long nend;
char *user_input;
Token *token;
LVar *locals;
Func *functions;
Node *code[100];

int main(int argc, char **argv){
    if (argc != 2) {
	    error("引数の個数が正しくありません");
	return 1;
    }
    
    nbegin = 0;
    nelse = 0;
    nend = 0;
    locals = calloc(1, sizeof(LVar));  
    functions = calloc(1, sizeof(Func));
    
    // 入力プログラム
    user_input = argv[1];

    // トークナイズ => Token *token
    tokenize(user_input);
    
    // パース => Node *code[100]
    program();

    // アセンブリの前半部分を出力
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    // プロローグ
    // 変数26個分の領域を確保する
    printf("    push rbp\n");       // 呼び出し元の関数のベースポインタをpush
    printf("    mov rbp, rsp\n");   // そのベースポインタを指すようにRBPを変更
    printf("    sub rsp, %d\n", locals->offset);   // 変数の数分の領域を確保
    
    // 先頭の式から順に
    // 抽象構文木を下りながらコード生成
    for (int i = 0; code[i]; i++) {
        gen(code[i]);

        // 式の評価結果としてスタックにひとつの値が残っているはずなので、
        // スタックが溢れないようにポップしておく
        printf("    pop rax\n");
    }
    
    // エピローグ
    // 最後の式の結果がRAXに残っているのでそれが返り値となる
    printf("    mov rsp, rbp\n");  // 呼び出し元のベースポインタをRSPが指すように変更(popするため)
    printf("    pop rbp\n");       // 呼び出し元のベースポインタをRBPに代入して, 元に戻る
    printf("    ret\n");           // 呼び出した関数の返り値(RAX)をret
    return 0;
}
