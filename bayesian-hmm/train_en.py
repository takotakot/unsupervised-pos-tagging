# -*- coding: utf-8 -*-
import argparse, sys, os, time, re, codecs
import treetaggerwrapper
import model

class color:
	PURPLE = "\033[95m"
	CYAN = "\033[96m"
	DARKCYAN = "\033[36m"
	BLUE = "\033[94m"
	GREEN = "\033[92m"
	YELLOW = "\033[93m"
	RED = "\033[91m"
	BOLD = "\033[1m"
	UNDERLINE = "\033[4m"
	END = "\033[0m"

def main(args):
	if args.filename is None:
		raise Exception()
	# 訓練データを形態素解析して各品詞ごとにその品詞になりうる単語の総数を求めておく
	# 教師なしと言えるのかは微妙
	print color.BOLD + "品詞辞書を構成しています ..." + color.END
	Wt_count = {}
	# 似たような品詞をまとめる
	# https://courses.washington.edu/hypertxt/csar-v02/penntable.html
	pos_sym = {"SYM", "SENT", ":", ",", "$", "(", ")", "'", "\""}
	pos_nn = {"NN", "NNS"}
	pos_rb = {"RB", "RBR", "RBS"}
	pos_vb = {"VB", "VBD", "VBG", "VBN", "VBZ", "VBP"}
	pos_vd = {"VD", "VDD", "VDG", "VDN", "VDZ", "VDP"}
	pos_vh = {"VH", "VH", "VHG", "VHN", "VHZ", "VHP"}
	pos_vv = {"VV", "VV", "VVG", "VVN", "VVZ", "VVP"}
	with codecs.open(args.filename, "r", "utf-8") as f:
		tagger = treetaggerwrapper.TreeTagger(TAGLANG="en")
		for line in f:
			line = re.sub(ur"\n", "", line)
			result = tagger.tag_text(line)
			if len(result) == 0:
				continue
			for poses in result:
				word, pos, lowercase = poses.split("\t")
				# 品詞をまとめる
				if pos in pos_sym:
					pos = "SYM"
				elif pos in pos_nn:
					pos = "NN"
				elif pos in pos_rb:
					pos = "RB"
				elif pos in pos_vb:
					pos = "VB"
				elif pos in pos_vd:
					pos = "VD"
				elif pos in pos_vh:
					pos = "VH"
				elif pos in pos_vv:
					pos = "VV"
				if pos not in Wt_count:
					Wt_count[pos] = {}
				if lowercase not in Wt_count[pos]:
					Wt_count[pos][lowercase] = 1
				else:
					Wt_count[pos][lowercase] += 1
	# Wtは各タグについて、そのタグになりうる単語の数が入っている
	# タグ0には<bos>と<eos>だけ含まれることにする
	Wt = [2]
	for tag, words in Wt_count.items():
		print tag, ":", len(words)
		if len(words) < 10:
			print words
		Wt.append(len(words))
	print "Wt:", Wt

	try:
		os.mkdir(args.model)
	except:
		pass

	hmm = model.bayesian_hmm()
	hmm.set_num_tags(len(Wt));	# 品詞数を設定

	# テキストファイルの読み込み
	# 複数のファイルを読んでもOK
	hmm.load_textfile(args.filename)

	# 全てのテキストファイルを読み込み終わってから初期化
	hmm.initialize()

	# Wtをセット
	hmm.set_Wt(Wt)

	# 温度の調整が面倒なので1で固定
	# hmm.set_temperature(2)	# 温度の初期設定
	# hmm.set_minimum_temperature(0.08)	# 温度の下限
	for epoch in xrange(1, args.epoch + 1):
		start = time.time()
		hmm.perform_gibbs_sampling()
		elapsed_time = time.time() - start
		sys.stdout.write(" Epoch {} / {} - {:.3f} sec\r".format(epoch, args.epoch, elapsed_time))		
		sys.stdout.flush()
		# hmm.anneal_temperature(0.998)	# 温度を下げる
		if epoch % 10 == 0:
			hmm.show_random_line(20, True);	# ランダムなn個の文と推定結果のタグを表示
			hmm.show_typical_words_for_each_tag(20);	# それぞれのタグにつき上位n個の単語を表示
			hmm.save(args.model);

if __name__ == "__main__":
	parser = argparse.ArgumentParser()
	parser.add_argument("-f", "--filename", type=str, default=None, help="訓練用のテキストファイルのパス.")
	parser.add_argument("-e", "--epoch", type=int, default=20000, help="総epoch.")
	parser.add_argument("-m", "--model", type=str, default="out", help="保存フォルダ名.")
	args = parser.parse_args()
	main(args)