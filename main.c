#include "9cc.h"

unsigned long i;
char *user_input;
Token *token;
LVar *locals;
Node *code[100];

int main(int argc, char **argv){
    if (argc != 2) {
	    error("引数の個数が正しくありません");
	return 1;
    }
     
    globals = calloc(1, sizeof(GVar));
    strings = calloc(1, sizeof(Str));
    
    // 入力プログラム
    user_input = argv[1];

    // トークナイズ => Token *token
    tokenize(user_input);
    // パース => Node *code[100]
    program();

    // アセンブリの前半部分を出力
    printf(".intel_syntax noprefix\n");
    printf(".data\n");

    while (globals->next) {
        printf("%s:\n", globals->name);
        printf("    .zero %lu\n", sizeoftype(globals->type));
        globals = globals->next;
    }

    for (i=0; strings->next; strings=strings->next) {
        printf(".L.data.%d:\n", strings->id);
        printf("    .string \"%s\"\n", strings->name);
    }
    printf(".text\n");
    
    // 先頭の関数から順に
    // 抽象構文木を下りながらコード生成
    for (int i = 0; code[i]; i++) {
        gen(code[i]);

        // 式の評価結果としてスタックにひとつの値が残っているはずなので、
        // スタックが溢れないようにポップしておく
        if (code[i]->kind != ND_GVARDEF) 
            printf("    pop rax\n");
    }
    
    return 0;
}
