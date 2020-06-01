#include "9cc.h"

/*
.text: プログラムのコードであることを示す
.data: プログラムのデータ部分であることを示す（グローバル変数）
.globl (label): ラベルをリンク時に見えるようにする
.byte n: 8ビットのnという値を格納する領域を確保
.word n : 16ビットのnという値を格納する領域を確保
.long n : 32ビットのnという値を格納する領域を確保
.quad n : 64ビットのnという値を格納する領域を確保
.align 4 : 4バイトごとの境界に合わせる（効率が上がる(?)）
*/

unsigned long i;
char *user_input;
Token *token;
LVar *locals;
GVar *globals;
Node *code[100];
Str *strings;

int main(int argc, char **argv){
    if (argc != 2) error("引数の個数が正しくありません");
     
    globals = calloc(1, sizeof(GVar));
    strings = calloc(1, sizeof(Str));
    
    // 入力プログラム
    user_input = argv[1];

    // トークナイズ => Token *token
    tokenizefile(user_input);
    // パース => Node *code[100]
    program();

    // アセンブリの前半部分を出力
    printf(".intel_syntax noprefix\n");
    printf(".data\n");

    while (globals->next) {
        printf("%.*s:\n", globals->len, globals->name);
        if (!globals->isassigned) printf("    .zero %lu\n", sizeoftype(globals->type));
        else {
            long val = globals->val;
            switch (sizeoftype(globals->type)) {
                case 8: printf("    .quad %ld\n", val); break;
                case 4: printf("    .long %d\n", (int)val); break;
                case 2: printf("    .word %d\n", (short)val); break;
                case 1: printf("    .byte %d\n", (char)val); break;
            }
        }
        globals = globals->next;
    }

    for (i=0; strings->next; strings=strings->next) {
        printf(".L.data.%d:\n", strings->id);
        printf("    .string \"%s\"\n", strings->name);
    }
    printf(".text\n");
    printf(".align 4\n");
    
    // 先頭の関数から順に
    // 抽象構文木を下りながらコード生成
    for (int i = 0; code[i]; i++) {
        gen(code[i]);

        // 式の評価結果としてスタックにひとつの値が残っているはずなので、
        // スタックが溢れないようにポップしておく
        if (code[i]->kind != ND_GVAR && code[i]->isdef) 
            printf("    pop rax\n");
    }
    
    return 0;
}
