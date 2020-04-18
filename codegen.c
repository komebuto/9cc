#include "9cc.h"

unsigned long nbegin;
unsigned long nelse;
unsigned long nend;

// レジスタ
// RAX: 関数の返り値
// AL:  RAXの下位 8 bits
// RDI: 関数の第一引数
// RSI: 関数の第二引数
// RSP: スタックポインタ（レジスタによるスタックのエミュレート）
// RBP: ベースレジスタ（現在の関数フレームの開始位置を常にさしているレジスタ）

// 命令
// mov (1), (2): (1) <= (2)
// mov (1), [(2)]: (1) <= (2)番地の値をロード
// mov [(1)], (2): (1)番地にストア <= (2) 
// add (1), (2): (1) <= (1) + (2)
// sub (1), (2): (1) <= (1) - (2)
// imul (1), (2): (1) <= (1) * (2)
// cqo     : (RDX RAX) <= (0 RAX)
// idiv (1): RAX(商) ... RDX(余り) <= (RDX RAX)(128 bits) ÷ (1)

// call (fuction): 1. call の次の命令のアドレスをスタックにプッシュ
//                 2. call の引数として与えられたアドレスにジャンプ
// ret     :  1. RAXの値を返す
//			  2. スタックからアドレス(call命令でプッシュした値)を１つポップ
//            3. そのアドレスにジャンプ
// push (1): (1)をスタックにpush
// pop (1) :  スタックから(1)にpop

// cmp (1), (2): (1)と(2)の比較結果をフラグレジスタにセット
// sete(==)/setne(!=)/setl(<)/setle(<=)/ (1): 
//               フラグレジスタにある演算結果の値を (1)(8 bits) にセット
// movzb (1), (2): (2)以外の(1)の残りの部分をゼロクリア ((2)は(1)の部分レジスタ)
// je .L[a~Z]XXX: cmpの結果等しければ, .L[a~Z]XXX: までジャンプ (XXXは通し番号)
// jmp .L[a~Z]XXX: .L[a~Z]XXX: までジャンプ (XXXは通し番号)

// 代入の左辺がアドレスを指すものかどうかを調べる ((a+1) = 2 のような不正な代入式を排除)
// その変数の番地をスタックにプッシュ
void gen_lval(Node *node) {
	if (node->kind != ND_LVAR)
		error("代入の左辺値が変数ではありません");
	printf("    mov rax, rbp\n");				// 変数のポインタはベースポインタからの
	printf("    sub rax, %d\n", node->offset);  // オフセットとして得ている
	printf("    push rax\n");			        // スタックにプッシュ
}

// コードジェネレータ
void gen(Node *node) {
	unsigned long nbegin_tmp;
	unsigned long nelse_tmp;
	unsigned long nend_tmp;

	if (node->kind == ND_RETURN) {
		gen(node->lhs);
		printf("    pop rax\n"); // lhsの計算結果をpop
		printf("    mov rsp, rbp\n");
		printf("    pop rbp\n");
		printf("    ret\n");
		return;
	}
    
	switch (node->kind) {
		case ND_NUM:
			printf("    push %d\n", node->val); // 数字の場合の値をpush
			return;
		case ND_LVAR:							// 与えられた変数を値に置き換える
			gen_lval(node);						// 変数のアドレスをpush
			printf("    pop rax\n");			// そのアドレスをraxにpop
			printf("    mov rax, [rax]\n");		// rax番地の値をraxにロード
			printf("    push rax\n");			// ロードされた値をpush
			return;
		case ND_ASSIGN:
			gen_lval(node->lhs);  // 左辺の変数のアドレスをpush
			gen(node->rhs);       // 右辺

			printf("    pop rdi\n");  // 代入式の右辺
			printf("    pop rax\n");  // 代入式の左辺（変数のアドレス）
			printf("    mov [rax], rdi\n");  // [rax]番地にrdiの値をストア
			printf("    push rdi\n"); // 代入式全体の評価値をpush
			return;
	}

	switch (node->kind) {
		case ND_IF:
			gen(node->cond);        // 条件式の計算(結果はスタックトップ)
			printf("    pop rax\n"); 
			printf("    cmp rax, 0\n");
			if (!node->rhs) {
				// else文なし
				nend_tmp = nend++;
				printf("    je .Lend%lu\n", nend_tmp);
				gen(node->lhs);
				printf(".Lend%lu:\n", nend_tmp);
				return;
			} else {
				// else文あり
				nelse_tmp = nelse++;
				nend_tmp = nend++;
				printf("    je .Lelse%lu\n", nelse_tmp);
				gen(node->lhs);
				printf("    jmp .Lend%lu\n", nend_tmp);
				printf(".Lelse%lu:\n", nelse_tmp);
				gen(node->rhs);
				printf(".Lend%lu:\n", nend_tmp);
				return;
			}
		case ND_WHILE:
			nbegin_tmp = nbegin++;
			nend_tmp = nend++;
			printf(".Lbegin%lu:\n", nbegin_tmp);
			gen(node->cond);
			printf("    pop rax\n");
			printf("    cmp rax, 0\n");
			printf("    je .Lend%lu\n", nend_tmp);
			gen(node->lhs);
			printf("    jmp .Lbegin%lu\n", nbegin_tmp);
			printf(".Lend%lu:\n", nend_tmp);
			printf("    push 0\n"); // while 内のstmtが評価されなかった場合 (while(0))
									// この後main関数内でpopされてしまう分を入れておく.
			return;
		case ND_FOR:
			nbegin_tmp = nbegin++;
			nend_tmp = nend++;
			if (node->lhs) {
				gen(node->lhs);
			}
			printf(".Lbegin%lu:\n", nbegin_tmp);
			if (node->cond) {
				gen(node->cond);
			} else {
				// 条件式が空の時は真を返す
				printf("    push 1\n"); 
			}
			printf("    pop rax\n");
			printf("    cmp rax, 0\n");
			printf("    je .Lend%lu\n", nend_tmp);
			gen(node->body);
			if (node->rhs) {
				gen(node->rhs);
			}
			printf("    jmp .Lbegin%lu\n", nbegin_tmp);
			printf(".Lend%lu:\n", nend_tmp);
			return;
		case ND_BLOCK:
			for (int i = 0; node->stmts[i]; i++) {
				gen(node->stmts[i]);
				// 各ステートメントで評価値がスタックに一つ残っている
				// ブロック内最後のステートメント以外の値はpopしておく
				if (!node->stmts[i+1]) {
					printf("    pop rax\n");
				}
			}	
			return;
	}

    gen(node->lhs);
    gen(node->rhs);
    
    // 第一引数をRAXに, 第二引数をRDIにpopして, 計算をして結果をpushする.
    printf("    pop rdi\n");
    printf("    pop rax\n");

    switch (node->kind) {
	case ND_ADD:
	    printf("    add rax, rdi\n");
	    break;
	case ND_SUB:
	    printf("    sub rax, rdi\n");
	    break;
	case ND_MUL:
	    printf("    imul rax, rdi\n");
	    break;
	case ND_DIV:
	    printf("    cqo\n");
	    printf("    idiv rdi\n");
	    break;
	
	case ND_EQ:
	    printf("    cmp rax, rdi\n");
	    printf("    sete al\n");	   // ALはRAXの下位8ビットを指す 
	    printf("    movzb rax, al\n"); // ALを残して, RAXの上位56ビットをゼロクリア
	    break;
	case ND_NEQ:
	    printf("    cmp rax, rdi\n");
	    printf("    setne al\n");
	    printf("    movzb rax, al\n");
	    break;
	case ND_LESS:
	    printf("    cmp rax, rdi\n");
	    printf("    setl al\n");
	    printf("    movzb rax, al\n");
	    break;
	case ND_LEQ:
	    printf("    cmp rax, rdi\n");
	    printf("    setle al\n");
	    printf("    movzb rax, al\n");
	    break;
    }
    
    printf("    push rax\n");  // RAXにある計算結果をpush
	return;
}
