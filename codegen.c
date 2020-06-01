#include "9cc.h"

extern LVar *locals;
extern GVar *globals;

unsigned long nbegin;
unsigned long nelse;
unsigned long nend;
unsigned long nrsp;
int offsetmax;
char *r64_arg[6] = {"rdi", "rsi", "rdx", "rcx",  "r8",  "r9"};  // 64 bits
char *r32_arg[6] = {"edi", "esi", "edx", "ecx", "r8d", "r9d"};  // 32 bits
char *r16_arg[6] = { "di",  "si",  "dx",  "cx", "r8w", "r9w"};  // 16 bits
char *r8_arg[6]  = {"dil", "sil",  "dl",  "cl", "r8b", "r9b"};  //  8 bits

int invoffset(Node *node) {
	//return offsetmax - ( node->offset - sizeoftype(node->type) );
	//if (node->lvar) return (*(node->lvar))->offset;
	return *(node->offptr);
}
int getoffset(Node *node) {
	return node->offset;
}

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
void gen_addr(Node *node) {
	if (node->kind == ND_LVAR) {
		if (node->isdef && node->nextdef) gen(node->nextdef);
		// ローカル変数のポインタはベースポインタからのオフセットとして得ている
		printf("    lea rax, [rbp-%d]\n", invoffset(node));				
		printf("    push rax\n");		// スタックにアドレスをプッシュ
	} else if (node->kind == ND_DEREF) {
		// * lhs
		gen(node->lhs);
	} else if (node->kind == ND_GVAR) {
		printf("    push offset %.*s\n", node->len, node->name);
	} else {
		error("代入の左辺が不正です");
	}
}

// スタックトップがポイントしている要素をロード
void load(Node *node) {
	if (node->type->ty == ARRAY) {
		return;
	}
	printf("    pop rax\n");
	switch (sizeofeltype(node->type)) {
		case 1: printf("    movsx rax, byte ptr [rax]\n"); break;
		case 2: printf("    movsx rax, word ptr [rax]\n"); break;
		case 4: printf("    movsx rax, dword ptr [rax]\n"); break;
		case 8: printf("    mov rax, [rax]\n"); break;
	}
	printf("    push rax\n");
	return;
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
			//offsetmax = node->offset;
			// name ( fargs[6] ) { stmts[100] }
			// プロローグ
    		// 変数の領域を確保する
			if (!node->isdef) return;
			printf(".globl %.*s\n", node->len, node->name);
			printf("%.*s:\n", node->len, node->name);  // name
    		printf("    push rbp\n");       // 呼び出し元の関数のベースポインタをpush
    		printf("    mov rbp, rsp\n");   // そのベースポインタを指すようにRBPを変更
			printf("    sub rsp, %d\n", node->offset);   // 関数内の変数の分だけスタックを伸ばす
			// fargs
			for (i=0; i<6 && node->fargs[i]; i++) {
				printf("    lea rax, [rbp-%d]\n", invoffset(node->fargs[i]));
				//printf("    sub rax, %d\n", invoffset(node->fargs[i]));//->offset); // 変数の場所を確保
				switch (sizeoftype(node->fargs[i]->type)) {
					case 8:
						printf("    mov [rax], %s\n", r64_arg[i]); // そこにレジスタの値を代入
						break;
					case 4:
						printf("    mov [rax], %s\n", r32_arg[i]);
						break;
					case 2:
						printf("    mov [rax], %s\n", r16_arg[i]);
						break;
					case 1:
						printf("    mov [rax], %s\n", r8_arg[i]);
						break;
				}
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
				printf("    pop %s\n", r64_arg[i]);
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
			printf("    call %.*s\n", node->len, node->name); // call func()
			// call前に引いた分戻す
			printf("    pop r10\n");
			printf("    add rsp, r10\n");      
			/*switch (sizeofeltype(node->type)) {
				case 1:
					printf("    push al\n");
					break;
				case 2:
					printf("    push ax\n");
					break;
				case 4:
					printf("    push eax\n");
					break;
				case 8:
					printf("    push rax\n");
					break;
			} */
			printf("    push rax\n");         // 関数の戻り値をストック
			return;
		case ND_NUM:
			printf("    push %d\n", node->val); // 数字の場合の値をpush
			return;
		case ND_LVAR:							           // 与えられた変数を値に置き換える
		case ND_GVAR:
			if (node->isdef) {
				if (node->nextdef) gen(node->nextdef);
				return;
			} else {
				gen_addr(node);		// 変数のアドレスをpush
				load(node);			// そのアドレスをロードしてpush
				return;
			}
		case ND_ASSIGN:
			gen_addr(node->lhs);                          // 左辺の変数のアドレスをpush
			gen(node->rhs);                               // 右辺
			printf("    pop rdi\n");                             // 代入式の右辺
			printf("    pop rax\n");                             // 代入式の左辺（変数のアドレス）
			switch (sizeofeltype(node->lhs->type)) {
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
		// "return" lhs
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
			load(node);
			return;
		case ND_ADDR:
			// & lhs
			gen_addr(node->lhs);
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
			if (node->lhs->type->ty == INT || node->lhs->type->ty == CHAR) {
				// INT/CHAR + PTR
				size_t n = sizeoftype(node->rhs->type->ptr_to);
				printf("    imul rax, %lu\n", n);
				printf("    add rax, rdi\n");
			} else if (node->rhs->type->ty == INT || node->rhs->type->ty == CHAR) {
				// OTHERtype + INT/CHAR
				size_t n = sizeoftype(node->lhs->type->ptr_to);
				printf("    imul rdi, %lu\n", n);
				printf("    add rax, rdi\n");
			} else {
				error("足し算の型が不正です");
			}
		}
	    break;
	case ND_SUB:
		if (node->type->ty == INT || node->type->ty == CHAR) {
			printf("    sub rax, rdi\n");
		} else if (node->lhs->type->ty == PTR && node->rhs->type->ty == PTR) {
			printf("    sub rax, rdi\n");
			printf("    cqo\n");
			printf("    mov rdi, %ld\n", sizeoftype(node->type->ptr_to));
			printf("    idiv rdi\n");  // 8 = sizeof(PTR)
		} else if (node->rhs->type->ty == INT || node->rhs->type->ty == CHAR) {
			size_t n = sizeoftype(node->lhs->type->ptr_to);
			printf("    imul rdi, %lu\n", n);
			printf("    sub rax, rdi\n");
		} else {
			error("引き算の型が不正です");
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
