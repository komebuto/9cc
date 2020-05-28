#include "9cc.h"

unsigned long nbegin;
unsigned long nelse;
unsigned long nend;
unsigned long nrsp;
char *reg_arg[6] = {"di", "si", "dx", "cx", "8", "9"};
char *r64_arg[6] = {"rdi", "rsi", "rdx", "rcx",  "r8",  "r9"};  // 64 bits
char *r32_arg[6] = {"edi", "esi", "edx", "ecx", "r8d", "r9d"};  // 32 bits
char *r16_arg[6] = { "di",  "si",  "dx",  "cx", "r8w", "r9w"};  // 16 bits
char *r8_arg[6]  = {"dil", "sil",  "dl",  "cl", "r8b", "r9b"};  //  8 bits

// 変数に値を代入する際変数の値
char prefix(Node *node) {
	TypeKind ty = node->type->ty;
	if (ty == PTR || ty == ARRAY) {
		return 'r';
	} else if (ty == INT) {
		return 'e';
	}
}
char pre;

// レジスタ
// RAX: 関数の返り値
// AL:  RAXの下位 8 bits
// RDI: 関数の第一引数
// RSI: 関数の第二引数
// RDX: 関数の第三引数
// RCX: 関数の第四引数
// R8:  関数の第五引数
// R9:  関数の第六引数
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
//           (1)はレジスタ. (数字にするとエラー)

// call (fuction): 1. call の次の命令のアドレスをスタックにプッシュ
//                 2. call の引数として与えられたアドレスにジャンプ
// ret     :  1. スタックからアドレス(call命令でプッシュした値)を１つポップ
//            2. そのアドレスにジャンプ
// push (1): (1)をスタックにpush
// pop (1) :  スタックから(1)にpop

// cmp (1), (2): (1)と(2)の比較結果をフラグレジスタにセット
// sete(==)/setne(!=)/setl(<)/setle(<=)/ (1): 
//               フラグレジスタにある演算結果の値を (1)(8 bits) にセット
// movzb (1), (2): (2)以外の(1)の残りの部分をゼロクリア ((2)は(1)の部分レジスタ)
// je .L[a~Z]XXX: cmpの結果等しければ, .L[a~Z]XXX: までジャンプ (XXXは通し番号)
// jmp .L[a~Z]XXX: .L[a~Z]XXX: までジャンプ (XXXは通し番号)

// 変数のアドレスの計算 スタックにpush
void gen_lval(Node *node) {
	if (node->kind == ND_LVAR) {
		printf("    mov rax, rbp\n");				// 変数のポインタはベースポインタからの
		printf("    sub rax, %d\n", node->offset);  // オフセットとして得ている
		printf("    push rax\n");			        // スタックにプッシュ
	} else if (node->kind == ND_DEREF) {
		// * lhs
		gen(node->lhs);
	} else if (node->kind == ND_GVARCALL) {
		printf("    push offset %s\n", node->name);
	} else {
		error("代入の左辺が不正です");
	}
}

// コードジェネレータ
void gen(Node *node) {
	unsigned long nbegin_tmp;
	unsigned long nelse_tmp;
	unsigned long nend_tmp;
	int i;
	char *reg;
	Type *tmptype;

	switch (node->kind) {
		case ND_FUNCDEF:
			// name ( fargs[6] ) { stmts[100] }
			// プロローグ
    		// 変数の領域を確保する
			printf(".globl %s\n", node->name);
			printf("%s:\n", node->name);  // name
    		printf("    push rbp\n");       // 呼び出し元の関数のベースポインタをpush
    		printf("    mov rbp, rsp\n");   // そのベースポインタを指すようにRBPを変更
			// fargs
			for (i=0; i<6 && node->fargs[i]; i++) {
				printf("    mov rsp, rbp\n");
				printf("    sub rsp, %d\n", node->fargs[i]->offset); // 変数の場所を確保
				printf("    mov [rsp], %c%s\n", 
					   prefix(node->fargs[i]), reg_arg[i]); // そこにレジスタの値を代入
			}
			// stmts
			i = 0;
			while (node->stmts[i]) {
				gen(node->stmts[i++]);
			}

			// エピローグ
    		// 最後の式の結果がRAXに残っているのでそれが返り値となる
    		printf("    mov rsp, rbp\n");  // 呼び出し元のベースポインタをRSPが指すように変更(popするため)
    		printf("    pop rbp\n");       // 呼び出し元のベースポインタをRBPに代入して, 元に戻る
    		printf("    ret\n");           // 呼び出した関数の返り値(RAX)をret
			return;

		case ND_FUNCALL:
			for (i=0; i<6 && node->fargs[i]; i++){
				// 引数の評価
				gen(node->fargs[i]);
				// 引数を担当レジスタに設定
				printf("    pop r%s\n", reg_arg[i]);
			}
			// RSPを16の倍数にする 
			printf("    mov r10, rdx\n");	  // 引数RDXを避難
			printf("    mov r11, 16\n");	  // 割る数 R11 = 16
			printf("    mov rax, rsp\n");     // RAX = RSP 
			printf("    cqo\n");		      // (RDX RAX) = (0 RAX)
			printf("    idiv r11\n");         // (0 RAX)÷RDI = RAX ... RDX
			printf("    sub rsp, rdx\n");     // 余りをrspから引いて16の倍数とする
			printf("    push rdx\n");         // 余りをストック
			printf("    mov rdx, r10\n");     // 避難した第三引数RDXを戻す
			printf("    call %s\n", node->name); // call func()
			// call前に引いた分戻す
			printf("    pop r10\n");
			printf("    add rsp, r10\n");      
			printf("    push rax\n");         // 関数の戻り値をストック
			return;
		case ND_NUM:
			printf("    push %d\n", node->val); // 数字の場合の値をpush
			return;
		case ND_LVAR:							           // 与えられた変数を値に置き換える
		case ND_GVARCALL:
			gen_lval(node);						           // 変数のアドレスをpush
			if (node->type->ty != ARRAY) {
				printf("    pop rax\n");			           // そのアドレスをraxにpop
				switch (onesizeoftype(node->type)) {		   // raxアドレスの値をその型のサイズに合わせてロード
					case 8: 
						printf("    mov rax, [rax]\n");
						break;
					case 4: 
						printf("    mov eax, [rax]\n");
						break;
					case 2: 
						printf("    mov ax, [rax]\n");
						printf("    movzw rax, ax\n");
						break;
					case 1: 
						printf("    mov al, [rax]\n");
						printf("    movzb rax, al\n");
						break;
				}
				printf("    push rax\n");		               // ロードされた値をpush
			} else {
				for (tmptype = node->type->ptr_to; tmptype->ty == ARRAY; tmptype = tmptype->ptr_to){
					printf("    mov rax, rsp\n");
					printf("    push rax\n");
				}
			}
			return;
		case ND_GVARDEF: return;
		case ND_ASSIGN:
			gen_lval(node->lhs);                                 // 左辺の変数のアドレスをpush
			gen(node->rhs);                                      // 右辺
			printf("    pop rdi\n");                             // 代入式の右辺
			printf("    pop rax\n");                             // 代入式の左辺（変数のアドレス）
			switch (onesizeoftype(node->lhs->type)) {
				case 8: 
					printf("    mov [rax], rdi\n");
					break;
				case 4:
					printf("    mov [rax], edi\n");
					break;
				case 2: 
					printf("    mov [rax], di\n");
					printf("    movzw rdi, di\n");
					break;
				case 1: 
					printf("    mov [rax], dil\n");
					printf("    movzb rdi, dil\n");
					break;
			}
			printf("    push rdi\n");                            // 代入式全体の評価値をpush
			return;
		case ND_RETURN:
		// "return" rhs
			gen(node->lhs);
			printf("    pop rax\n"); // lhsの計算結果をpop RAXにセット
			printf("    mov rsp, rbp\n");
			printf("    pop rbp\n");
			printf("    ret\n");
			return;
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
		case ND_DEREF:
			// * lhs
			gen(node->lhs);
			printf("    pop rax\n");
			printf("    mov rax, [rax]\n");
			printf("    push rax\n");
			return;
		case ND_ADDR:
			// & lhs
			gen_lval(node->lhs);
			return;
		case ND_STR:
			printf("    push offset .L.data.%d\n", node->val);
			return;
	}

	// 二項演算子
    gen(node->lhs);
    gen(node->rhs);
    // 第一引数をRAXに, 第二引数をRDIにpopして, 計算をして結果をpushする.
    printf("    pop rdi\n");
    printf("    pop rax\n");
    switch (node->kind) {
	case ND_ADD:
		// 式の型に応じて足すバイト数を調整する
		// OTHERtypeはARRAY又はPTR
		if (node->type->ty == INT || node->type->ty == CHAR) {
			// INT/CHAR + INT/CHAR
			printf("    add rax, rdi\n");
		} else {
			if (node->lhs->type->ty == INT || node->type->ty == CHAR) {
				// INT/CHAR + OTHERtype
				printf("    imul rax, %lu\n", sizeoftype(node->type->ptr_to));
				printf("    add rdi, rax\n");
				printf("    mov rax, rdi\n");
			} else {
				// OTHERtype + INT/CHAR
				printf("    imul rdi, %lu\n", sizeoftype(node->type->ptr_to));
				printf("    add rax, rdi\n");
			}
		}
	    break;
	case ND_SUB:
		if (node->type->ty == INT) {
			printf("    sub rax, rdi\n");
		} else {
			printf("    imul rdi, %lu\n", sizeoftype(node->type->ptr_to));
			printf("    sub rax, rdi\n");
		}
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
