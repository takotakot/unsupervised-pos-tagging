#pragma once
#include <boost/python.hpp>
#include <string>
#include "../bhmm/hmm.h"

namespace bhmm {
	class Model{
	public:
		HMM* _hmm;
		Model();
		~Model();
		bool load(std::string filename);
		bool save(std::string filename);
		void set_alpha(double alpha);
		int get_num_tags();
		double get_temperature();
		void set_temperature(double temperature);
		void set_minimum_temperature(double temperature);
		void anneal_temperature(double temperature);
		void viterbi_decode(std::vector<Word*> &data, std::vector<int> &sampled_state_sequence, double** forward_table, double** decode_table);
	};
}