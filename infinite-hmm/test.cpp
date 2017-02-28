#include <boost/format.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <string>
#include <set>
#include <unordered_map>
#include <functional>
#include <fstream>
#include <cassert>
#include "core/ihmm.h"
#include "core/util.h"
#include "model.cpp"
using namespace std;
using namespace boost;

void test1(){
	PyInfiniteHMM* model = new PyInfiniteHMM(2);
	model->load_textfile("../alice.txt");
	model->mark_low_frequency_words_as_unknown(1);
	model->compile();
	for(int i = 0;i < 10;i++){
		model->perform_gibbs_sampling();
		// model->_hmm->dump_oracle_tags();
		model->show_typical_words_for_each_tag(20);
		// model->_hmm->dump_oracle_words();
		// model->_hmm->check_oracle_tag_count();
		// model->_hmm->check_oracle_word_count();
		// model->_hmm->check_sum_bigram_destination();
		// model->_hmm->check_sum_tag_customers();
		// model->_hmm->check_sum_word_customers();
		// model->_hmm->check_tag_count();
		// model->_hmm->sample_alpha_and_beta();
		// model->_hmm->sample_gamma();
		// model->_hmm->sample_gamma_emission();
		// model->_hmm->dump_hyperparameters();
		model->show_log_Pdata();
		if(i % 10 == 0){
			model->_hmm->save();
		}
	}
	delete model;
}

void test2(){
	PyInfiniteHMM* model = new PyInfiniteHMM(20);
	model->load_textfile("../test.txt");
	model->mark_low_frequency_words_as_unknown(1);
	model->compile();
	for(int i = 0;i < 10000;i++){
		model->perform_gibbs_sampling();
		model->show_typical_words_for_each_tag(20);
	}
	delete model;
}

int main(){
	// for(int i = 0;i < 5;i++){
	// 	test1();
	// }
	test2();
	return 0;
}