# coding: utf-8
import argparse, sys, re, pylab, codecs
import treetaggerwrapper
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
import model
from train_en import collapse_pos, posset, stdout, parse_tagger_result_str

# フォントをセット
# UbuntuならTakaoGothicなどが標準で入っている
sns.set(font=["MS Gothic"], font_scale=3)

def main(args):
	ithmm = model.ithmm()
	if ithmm.load(args.model) == False:
		raise Exception("モデルが見つかりません.")

	# 訓練データを形態素解析してモデルに追加
	print stdout.BOLD + "データを集計しています ..." + stdout.END
	true_series_array = []
	all_types_of_pos = set()
	all_types_of_pos.add("EOS")
	num_occurrence_of_pos_for_tag = {}
	tagger = treetaggerwrapper.TreeTagger(TAGLANG="en")
	with codecs.open(args.filename, "r", "utf-8") as f:
		for i, line in enumerate(f):
			if i % 10 == 0:
				sys.stdout.write("\r{}行目を処理中です ...".format(i))
				sys.stdout.flush()
			line = re.sub(ur"\n", "", line)	# 改行を消す
			results = tagger.tag_text(line)	# 形態素解析
			segmentation = ""
			true_series = []
			for result_str in results:
				pos, word = parse_tagger_result_str(result_str)
				all_types_of_pos.add(pos)
				segmentation += word + " "
				true_series.append((word, pos))
			true_series.append(("<eos>", "EOS"))	# <eos>
			segmentation = re.sub(r" +$", "",  segmentation)	# 行末の空白を除去
			ithmm.add_test_data(segmentation)
			true_series_array.append(true_series)

	viterbi_series_array = ithmm.viterbi_decode_test()
	for n, viterbi_series in enumerate(viterbi_series_array):
		true_series = true_series_array[n]
		assert len(viterbi_series) == len(true_series)
		for i, point_viterbi in enumerate(viterbi_series):
			point_true = true_series[i]
			pos_true = point_true[1]
			tag_viterbi = point_viterbi[1]
			if tag_viterbi not in num_occurrence_of_pos_for_tag:
				num_occurrence_of_pos_for_tag[tag_viterbi] = {}
			if pos_true not in num_occurrence_of_pos_for_tag[tag_viterbi]:
				num_occurrence_of_pos_for_tag[tag_viterbi][pos_true] = 0
			num_occurrence_of_pos_for_tag[tag_viterbi][pos_true] += 1

	tags = ithmm.get_all_tags()
	# 存在しない部分を0埋め
	for tag, occurrence in num_occurrence_of_pos_for_tag.items():
		for pos in all_types_of_pos:
			if pos not in occurrence:
				occurrence[pos] = 0
	
	for tag in tags:
		if tag not in num_occurrence_of_pos_for_tag:
			num_occurrence_of_pos_for_tag[tag] = {}
			for pos in all_types_of_pos:
				num_occurrence_of_pos_for_tag[tag][pos] = 0

	# 正解品詞ごとに正規化
	for pos in all_types_of_pos:
		z = 0
		for tag, occurrence in num_occurrence_of_pos_for_tag.items():
			z += occurrence[pos]
		if z > 0:
			for tag, occurrence in num_occurrence_of_pos_for_tag.items():
				occurrence[pos] = float(occurrence[pos]) / float(z)

	fig = pylab.gcf()
	fig.set_size_inches(len(tags) + 3, len(all_types_of_pos))
	pylab.clf()
	dataframe = pd.DataFrame(num_occurrence_of_pos_for_tag)
	ax = sns.heatmap(dataframe, annot=False, fmt="f", linewidths=0)
	ax.tick_params(labelsize=20) 
	plt.yticks(rotation=0)
	plt.xticks(rotation=90)
	plt.xlabel(u"予測タグ")
	plt.ylabel(u"正解品詞")
	heatmap = ax.get_figure()
	heatmap.savefig("{}/pos.png".format(args.model))

if __name__ == "__main__":
	parser = argparse.ArgumentParser()
	parser.add_argument("-f", "--filename", type=str, default=None, help="学習に使ったテキストファイルのパス.")
	parser.add_argument("-m", "--model", type=str, default="out", help="モデルファイル.")
	args = parser.parse_args()
	main(args)