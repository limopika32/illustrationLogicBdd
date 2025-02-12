#include <tdzdd/DdSpec.hpp>
#include <tdzdd/DdStructure.hpp>
#include <tdzdd/DdEval.hpp>

#include <bits/stdc++.h>

#include <memory>
#include <fstream>
#include <iostream>
#include <string>

// TODO
// //! のコメントは後対応のつもり(検索用)

// プロトタイプ宣言
// C++(C) では上から順に名前解決を行う為 そのままでは後出しが出来ない

int hint_loader(const char*);
void hint_viewer(int, int, int**, int**);
int hasQrPattern(int, int, int);

// 名前空間を指定することで std::cout を cout に省略できる
// python での from... import... の イメージ

using namespace std;

// ヒントに使用する配列を static で宣言 (宣言が先なのでとりあえずダブルポインタを用意)

static int **V;
static int **H;
static int LENGTH;

// QRコード特化の作動フラグ

static bool isQrCheckEnabled = true;
static bool isQrFirstChecked = false;

// --------------------------------------------------

// DdSpec だと pointer の参照渡ししか出来ないので 配列を扱える PodArrayDdSpec を使用する
class illustLogicBdd : public tdzdd::PodArrayDdSpec<illustLogicBdd, char, 2> {

	// ヒント全体の配列 縦vert / 横horz
	// 配列で受け取るのでポインタとして初期化
	// 受け取ったヒントは弄らないので constで定数として扱う
	// const は基本的に触らない 定数 につける
	int** const vert;
	int** const horz;

	// 全体のマス数 total
	// 後で level の設定に使用する
	int const total;

	// 縦または横のマス数 n
	// 配列のサイズ指定 や マス位置の補助定数として使う
	int const n;

	// ヒントの最大個数を格納する
	// 後のforの最大回数でsizeofを使用するのを回避するため
	int const m_length;

	//コンストラクタの代入が n(n) で行われている (javaでいう n = this.n)
public: illustLogicBdd(int pn, int total, int len, int** vertical, int** horizontal) :
	total(total), vert(vertical), horz(horizontal), n(pn), m_length(len) {
		// DdSpec側のstate配列初期化
		setArraySize(2 + n*2);
	}

	// 親ノードの取得
	int getRoot(char* state) const {
		// state の初期化
		// pos_v, st_v, pos_h[n], st_h[n] の順に一次配列として保管
		// ... BDD/ZDD作成時に 縦横の片方が保持できないので 縦/V を配列として保管する

		// pos_v/h	どのヒントまで塗ったか position of vert/horz
		// ... pos_h[j] は j列目のpos_h[j]番目のヒントを塗っている途中であるかを表す
	
		// st_v/h	そのヒントのどこまで塗ったか state of vert/horz
		// ... st_h[j] (= m) は j列目のpos_h番目のヒントの mマス目まで塗ってある状態を表す

		// 全て -1で初期化
		// xs の要素数を取得するときに 割る数を xs[0] とすれば 型が異なっても対応できる
		// → sizeof を動的なものに使用するとコンパイル時にサイズが決まってしまうことから不適切
		// 最初から要素数が分かる物は定数または静的な数を使う
		for (int i = 0; i < 2+n*2 ; i++) state[i] = -1;

		// 塗るマス数 = 初期level を返す
		return total;
	}

	// 子ノードの取得
	int getChild(char* state, int level, int value) const {
		//- INITIALIZE PHASE ---------------------------------------------------------------------------------
		// マス位置取得 (total-level で 0～n に修正する)
		int v = (total - level) / n,
			h = (total - level) % n;

		// 今後 state[...]で使用するために宣言しておく
		int pos_v = 0,
			st_v = 1,
			pos_h = 2 + h,
			st_h = 2 + n + h;

		// state 行リセット
		if ( h == 0 ){
			state[pos_v] = -1;
			state[st_v] = -1;
		} 
			
		// ヒント配列の要素数
		// forで -1(ヒント無し) 部分を省いた数を数える
		int len_v = 0,
			len_h = 0;
		for (int i = 0; i < m_length; i++){
			if (vert[v][i] != -1){
				len_v++;
			} else {
				break;
			}
		}
		for (int i = 0; i < m_length; i++){
			if (horz[h][i] != -1){
				len_h++;
			} else {
				break;
			}
		} 

		// その行 または その列にたいして必要なスペースの数を格納する
		// 後のfor分で 1 余計に入るので初期化値は -1
		int req_v = -1,
			req_h = -1;

		// req計算用変数の宣言
		// pos が -1以下 スタートなら 0 に修正 それ以外はその位置から +1(次のヒントから計算する)
		// 塗っている最中は考慮しない ... value==0 の時に矛盾チェックが終わった後で判定する為
		int tmp_minv = state[pos_v] <= -1 ? 0 : state[pos_v]+1,
			tmp_minh = state[pos_h] <= -1 ? 0 : state[pos_h]+1;

		// あとやっぱりVとH逆かも(使用するときに反転で運用している)
		for (int i = tmp_minv; i < len_v ; i++) {
			req_v += vert[v][i];	// ヒント数字
			req_v++;				// 間の1マス分
		}
		for (int i = tmp_minh; i < len_h ; i++) {
			req_h += horz[h][i];	// ヒント数字
			req_h++;				// 間の1マス分
		}

		// for を回せなかった時用に -1 → 0 に修正(len==0など)
		req_v = req_v < 0 ? 0 : req_v;
		req_h = req_h < 0 ? 0 : req_h;


		// 動作確認用
		// cerr << endl;
		// cerr << "[" << level << "-" << value << "] pos V:" << v << " H:" << h << endl;
		// cerr << "| length V:" << len_v << " H:" << len_h << endl;
		// cerr << "| remain V:"<< n-v << " (" << req_v << "), H:" << n-h << " (" << req_h << ")" << endl;

		// for (int i=0 ; i < 2+n*2; i++) cerr << state[i] << ", ";
		// cerr << endl;
	

		//- CHECKING PHASE -----------------------------------------------------------------------------------
		// そもそもの矛盾
		// V側がヒントを塗っている最中 and H側がヒントを塗り終わって空白が必要 (Vst > 0 && Hst[n] == 0)
		// V側がヒントを塗り終わって空白が必要 and H側がヒントを塗っている最中 (Vst == 0 && Hst[n] > 0)
		if ( (state[st_v] == 0 && state[st_h] > 0) || (state[st_v] > 0 && state[st_h] == 0) ) return 0;

		if (value == 1) {
			// そのマスを塗るなら
			
			// 矛盾する場合... (塗ってはいけない)
			// V側がヒントを塗り終わって空白が必要 and H側がいつでも塗れる状態 (Vst == 0 && Hst[n] == -1)
			// V側がいつでも塗れる状態 and H側がヒントを塗り終わって空白が必要 (Vst == -1 && Hst[n] == 0)
			// 両者ともヒントを塗り終わって空白が必要 (Vst == 0 && Hst[n] == 0)
			// ...結局 どちらかが空白が必要なら 0端
			if ( state[st_v] == 0  || state[st_h] == 0 ) return 0;
			// cerr << "cause 11 pass" << endl;

			// QRコード特化仕様
			// 塗らなきゃ行けないのに塗らない判定は0端
			if (hasQrPattern(v, h, n) == 0) return 0;

			// ヒントチェック用の調整変数
			// reqの計算が前にある為、この後の処理によっては重複が発生する
			int adjust_v = 0,
				adjust_h = 0;

			// 塗り始めの処理だけさせる ... st > 0 の時に特にやる事が無い(終盤にまとめてある)
			// V側がいつでも塗れる状態 (Vst == -1)
			if ( state[st_v] == -1 ) {
				// V側に次のヒントが無いときは 0端
				if (state[pos_v] +1 >= len_v) return 0;

				// Vpos += 1 V側ヒント位置を次へ
				state[pos_v]++;
				// Vst = V[pos]  これから塗るマス数を代入
				state[st_v] = vert[v][state[pos_v]];
				adjust_v = state[st_v];
			}
			// cerr << "cause 12 pass" << endl;

			// H側がいつでも塗れる状態 (Hst[n] == -1)
			if ( state[st_h] == -1 ) {
				// H側に次のヒントが無いときは 0端
				if (state[pos_h] +1 >= len_h) return 0;
				
				// Hpos[n] += 1 H側ヒント位置を次へ
				state[pos_h]++;
				// Hst[n] = H[pos]  これから塗るマス数を代入
				state[st_h] = horz[h][state[pos_h]];
				adjust_h = state[st_h];
			}
			// cerr << "cause 13 pass" << endl;

			// 塗るときにもヒントチェックをとりあえず
			if ( n < v + req_h + state[st_h] - adjust_h) return 0;
			// cerr << "cause 14 pass" << endl;
			if ( n < h + req_v + state[st_v] - adjust_v) return 0;
			// cerr << "cause 15 pass" << endl;
			
			// value == 1 の時点で(0端を除いて)どの条件でも塗るので
			// Vst -= 1, Hst[n] -= 1 両者塗るマス-1
			state[st_v]--;
			state[st_h]--;
		
		} else {
			// そのマスを塗らないなら

			// 矛盾する場合... (塗らなきゃいけない)
			// V側がヒントを塗っている最中 and H側がいつでも塗れる状態 (Vst > 0 && Hst[n] == -1)
			// V側がいつでも塗れる状態 and H側がヒントを塗っている最中 (Vst == -1 && Hst[n] > 0)
			// 両者ともヒントを塗っている最中 (Vst > 0 && Hst[n] > 0)
			// ...結局 どちらかが塗ってる最中なら 0端
			if ( state[st_v] > 0 || state[st_h] > 0 ) return 0;
			// cerr << "cause 21 pass" << endl;

			// QRコード特化仕様
			// 塗ったらいけないのに塗る判定は0端
			if (hasQrPattern(v, h, n) == 1) return 0;

			// どちらにせよ塗らないので (かつ矛盾チェックも終わっている)
			// ヒントチェックは全部やっておく
			// V側のヒントの残りが破綻するなら 0端
			if ( n < v + req_h ) return 0;
			// cerr << "cause 22 pass" << endl;
			// H側のヒントの残りが破綻するなら 0端
			if ( n < h + req_v ) return 0;
			// cerr << "cause 23 pass" << endl;

			// state が既に -1 ならほったらかせば良い(特にやることが無い)
			// V側がヒントを塗り終わって空白が必要 (Vst == 0)
			if ( state[st_v] == 0 ) state[st_v] = -1;	// Vst = -1  V側次のヒントを塗れる状態にする
				
			// H側がヒントを塗り終わって空白が必要 (Hst[n] == 0)
			if ( state[st_h] == 0 ) state[st_h] = -1;	// Hst[n] = -1  H側次のヒントを塗れる状態にする		
		}

		// for (int i=0 ; i < 2+n*2; i++) cerr << state[i] << ", ";
		// cerr << endl;

		//- FINALIZE PHASE -----------------------------------------------------------------------------------
		// 最終行 チェック → level-- → level==0 チェックの順で行う

		// level が n未満(最後の列を塗り終えているかチェック)
		if (level < n){
			// H側まだ塗っている最中? (Hst[n]が0以下ならOK)
			if (state[st_h] > 0) return 0;
			// H側ヒントを全て塗りきったか? Hposに+1してlenに到達すればOK
			if (state[pos_h]+1 < len_h) return 0;
		}
		
		
		// 最後の"行"を塗り終えているかチェック
		if ( h == n-1 ){
			// V側まだ塗っている最中? (Vstが0以下ならOK)
			if (state[st_v] > 0) return 0;
			// V側ヒントを全て塗りきったか? Vposに+1してlenに到達すればOK
			if (state[pos_v]+1 < len_v) return 0;
		}

		// level を -1 する
		level--;

		// level == 0 (最終行チェック)
		// 最終的に塗るマス(条件)を満たしているなら -1
		// それ以外は存在しないノードとして 0 を return
		if (level == 0){
			// V側まだ塗っている最中? (Vstが0以下ならOK)
			if (state[st_v] > 0) return 0;
			// V側ヒントを全て塗りきったか? Vposに+1してlenに到達すればOK
			if (state[pos_v]+1 < len_v) return 0;
			
			cout << "found!" << endl;
			//最終チェックOKなら return -1
			return -1;
		}

		// チェックを通過 and まだ塗り最中 なら次のlevelを渡す
		return level;
	}
};

// ヒントを読み込む
// テキストファイルのpathを受取
int hint_loader(const char* path){
	int num_v, num_h, max_len;
	ifstream ifs(path);
    string body, header, buf;

	// 指定したファイルが読めない
    if (ifs.fail()){
		cerr << "Failed to open file." << endl;
		return -1;
	}

	// ヘッダーの読み取り
	// 行数、列数、総塗マス数を取得
	getline(ifs, header);
	istringstream line_buf(header);
		getline(line_buf, buf, ',');
		num_v = stoi(buf);
		getline(line_buf, buf, ',');
		num_h = stoi(buf);
		getline(line_buf, buf, ',');
		max_len = stoi(buf);
	
	LENGTH = max_len;

	// 二次配列の一次部分を初期化
	// ファイルを読み取って 動的に配列確保
	V = new int*[num_v];
	H = new int*[num_h];
	// new を使用しているので後で delete[] が必要? → プログラム終了まで使うので特に必要なしと判断
	// unique_ptrを使用すると良いかも? → state等に持つのはメモリを余計に食うことになるので使うとしてもヒントの配列へ代入ぐらい...

	// 列側 - 配列の初期化と代入
    for (int i = 0; i < num_v; i++){
		// 二次配列の二次部分
		V[i] = new int[max_len];
	
		getline(ifs, body);
		istringstream line_buf(body);
		
		// 出来ればforで回したいが
		// その行/列のヒントがない場合は全部0で帰ってくる事を確認済み
		int j = 0;
		while (getline(line_buf, buf, ',')){
			V[i][j] = stoi(buf);
			j++;
		}
		// どうしても二次部分が余分発生するので -1 で応急処置
		for (; j < max_len; j++) V[i][j] = -1;
	}

	// 行側 - 配列の初期化と代入 (列側と同等処理)
	for (int i = 0; i < num_h; i++){
		H[i] = new int[max_len];
	
		getline(ifs, body);
		istringstream line_buf(body);
		
		int j = 0;
		while (getline(line_buf, buf, ',')){
			H[i][j] = stoi(buf);
			j++;
		}

		for (; j < max_len; j++) H[i][j] = -1;  
	}

	return num_v > num_h ? num_v : num_h;
};

// デバッグ用 取得したヒントをcerrに出力する、log の確認用に
// n 縦横の長さ, len_max ヒントの最大の長さ, hint_V/hint_H ヒントの二次元配列
void hint_viewer(int n, int len_max, int** hint_V, int** hint_H){
	cerr << "vertical hints:" << endl;
	for (int i = 0; i < n ; i++){
		cerr << i << ": ";
		for (int j=0; j < len_max; j++) {
			cerr << " " << hint_V[i][j];
		}
		cerr << endl;
	}
	cerr << endl;
	
	cerr << "horizontal hints:" << endl;
	for (int i = 0; i < n ; i++){
		cerr << i << ": ";
		for (int j=0; j < len_max; j++) {
			cerr << " " << hint_H[i][j];
		}
		cerr << endl;
	}
	cerr << endl;
};


//!QR関連はクラスにしても良いかも

// セル数がQRの仕様に合致しているかチェックする
// Lv.1 21*21 ~ Lv.40 177*177
// Lv が 1 上がる毎に セル数は 4 増える (21 % 4 == 1 == 177 % 4)
bool hasQrSpecification(int n){
    return (21 <= n and n <= 177 and n % 4 == 1);
}

// qrコードの固定部分をチェックする
// v,h に座標  n に縦横の長さ
// 塗るときは 1, 塗らないときは 0 を返す 不明(変動部分)の場合は -1
int hasQrPattern(int v, int h, int n){
	if (!isQrCheckEnabled) return -2;
    if (!isQrFirstChecked) {
        isQrCheckEnabled = hasQrSpecification(n);
        isQrFirstChecked = true;
    };

		// 固定部分である ファインダパターン(3隅のパターン) と タイミングパターン を確定させるために使う
		if (
			// 塗るケース
			// ファインダパターン(上部)
			(v == 0 and (h <= 5 or n - 7 <= h))
			or
			((v == 1 or v == 5) and (h == 0 or h == n - 7 or h == n - 1))
			or
			((2 <= v and v <= 4) and ((h <= 4 and h != 1) or (n-7 <= h and h != n - 6 and h != n - 2)))
			or
			// ファインダパターン(下部)
			((v == n - 7 or v == n - 1) and h <= 5)
			or
			((v == n - 6 or v == n - 2) and h == 0)
			or
			((n - 5 <= v and v <= n - 3) and (h <= 4 and h != 1))
			or
			// タイミングパターン
			(v == 6 and (h <= 6 or h % 2 == 0 or h >= n - 7))
			or
			(h == 6 and (v <= 6 or v % 2 == 0 or v >= n - 7))
		){
			return 1;
		}else if(
			// 塗らないケース(塗るケースで塗らなかったところ)
			// ファインダパターン
			(v <= 7 and (h <= 7 or n - 8 <= h))
			or
			(n - 8 <= v and h <= 7)
			or
			// タイミングパターン
			(v == 6 and (h <= 6 or h % 2 == 1 or h >= n - 7))
			or
			(h == 6 and (v <= 6 or v % 2 == 1 or v >= n - 7))
		){
			return 0;
	}else{
	return -1;
    }
}


int main(int argc, char** argv) {
	int n;
	const char DEFAULT_HINT[10] = "hint6.txt";

	// argv によるファイルパスが渡されている(argcが2以上)なら そのヒントを読み込む
	// 無ければ DEFAULT_HINT をデフォルトで読み込む
	n = hint_loader( argc > 1 ? argv[1] : DEFAULT_HINT );

	// ヒントが読めなかった
	if (n < 0) return n;

	// logに読み込んだヒントの出力
	//hint_viewer(n, LENGTH, V, H);

	// class 呼び出し
	illustLogicBdd illustLogic(n, n*n, LENGTH, V, H);
	
	// tdzdd へ処理を回す OpenMPを使った並列化をするなら第2引数をtrueに (logを取るときはfalse)
	tdzdd::DdStructure<2> dd(illustLogic, true);

	// BDD の 最適化
	dd.bddReduce();

	// dot形式に書き出し(coutに)
	//dd.dumpDot();

	// 全ての解をpbmファイルで出力する
	int fileNo = 0;
	for (auto& edge_set : dd){
		string path = "output_"+to_string(fileNo)+".pbm";
		ofstream outputfile( path );
    	// header of Potable Bitmap Map (.pbm)
		outputfile << "P1" << endl << n << " " << n << endl;
		for (int i=0; i<n; i++){
			for (int j=0; j < n; j++){
				int level = n*n - (n*i+j);
				outputfile << edge_set.count(level);
			}
			outputfile << endl;
		}
		outputfile.close();
		fileNo++;
	}
	cout << "Done! count:" << fileNo << endl;

	return 0;
};
