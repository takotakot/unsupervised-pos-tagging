#pragma once
#include <boost/serialization/serialization.hpp>
#include <vector>
#include <map>
#include "common.h"

namespace ithmm {
	class Node;
	class HPYLM {
	private:
		friend class boost::serialization::access;
		template <class Archive>
		void serialize(Archive &ar, unsigned int version);
		bool _add_customer_to_table(int token_id, int table_k, double parent_Pw, std::vector<double> &d_m, std::vector<double> &theta_m);
		bool _add_customer_to_new_table(int token_id, double parent_Pw, std::vector<double> &d_m, std::vector<double> &theta_m);
		bool _remove_customer_from_table(int token_id, int table_k, std::vector<int> &num_customers_at_table);
	public:
		std::map<int, std::vector<int>> _arrangement;	// 客の配置 std::vector<int>のk番目の要素がテーブルkの客数を表す
		int _num_tables;					// 総テーブル数
		int _num_customers;					// 客の総数
		int _depth;
		HPYLM* _parent;
		Node* _state_node;
		HPYLM();
		HPYLM(Node* node);
		bool child_exists(int token_id);
		int get_num_tables_serving_word(int token_id);
		int get_num_customers_eating_word(int token_id);
		HPYLM* find_child_node(int token_id, bool generate_if_not_exist = false);
		bool add_customer(int token_id, double g0, std::vector<double> &d_m, std::vector<double> &theta_m);
		bool remove_customer(int token_id);
		double compute_p_w(int token_id, double g0, std::vector<double> &d_m, std::vector<double> &theta_m);
		bool remove_from_parent();
		void delete_child_node(int token_id);
		int sample_token(double g0, std::vector<double> &d_m, std::vector<double> &theta_m);
		int get_max_depth(int base);
		int get_num_tables();
		int get_num_customers();
		// dとθの推定用
		// "A Bayesian Interpretation of Interpolated Kneser-Ney" Appendix C参照
		// http://www.gatsby.ucl.ac.uk/~ywteh/research/compling/hpylm.pdf
		double auxiliary_log_x_u(double theta_u);
		double auxiliary_y_ui(double d_u, double theta_u);
		double auxiliary_1_y_ui(double d_u, double theta_u);
		double auxiliary_1_z_uwkj(double d_u);
		void dump();
	};
}
