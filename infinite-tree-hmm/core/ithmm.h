#ifndef _ithmm_
#define _ithmm_
#include <boost/format.hpp>
#include <cmath>
#include <vector>
#include "tssb.hpp"
#include "node.hpp"
#include "hpylm.hpp"
#include "sampler.h"
#include "cprintf.h"
#include "hyperparameters.h"

class iTHMM{
public:
	TSSB* _structure_tssb;	// 木構造を表すためだけのTSSB。HTSSBは全てこのノードを基準に成形する
	double _alpha;
	double _gamma;
	double _lambda;
	int _max_depth;
	vector<double> _hpylm_d_m;		// HPYLMのハイパーパラメータ（ディスカウント係数）
	vector<double> _hpylm_theta_m;	// HPYLMのハイパーパラメータ（集中度）
	vector<double> _hpylm_a_m;		// ベータ分布のパラメータ	dの推定用
	vector<double> _hpylm_b_m;		// ベータ分布のパラメータ	dの推定用
	vector<double> _hpylm_alpha_m;	// ガンマ分布のパラメータ	θの推定用
	vector<double> _hpylm_beta_m;	// ガンマ分布のパラメータ	θの推定用
	iTHMM(){
		_alpha = iTHMM_ALPHA;
		_gamma = iTHMM_GAMMA;
		_lambda = iTHMM_LAMBDA;
		_max_depth = 0;

		_structure_tssb = new TSSB(_alpha, _gamma, _lambda);
		Node* root_on_structure = _structure_tssb->_root;
		Node* root_on_htssb = new Node(NULL, root_on_structure->_identifier);
		root_on_htssb->_htssb_owner_id = root_on_structure->_identifier;
		root_on_htssb->_htssb_owner = root_on_structure;
		root_on_htssb->_structure_tssb_myself = root_on_structure;
		root_on_structure->_transition_tssb = new TSSB(root_on_htssb, _alpha, _gamma, _lambda);
		root_on_structure->_transition_tssb->_owner_id = root_on_structure->_identifier;
		root_on_structure->_transition_tssb_myself = root_on_htssb;

		_hpylm_d_m.push_back(HPYLM_D);
		_hpylm_theta_m.push_back(HPYLM_THETA);
		_hpylm_a_m.push_back(HPYLM_A);
		_hpylm_b_m.push_back(HPYLM_B);
		_hpylm_alpha_m.push_back(HPYLM_ALPHA);
		_hpylm_beta_m.push_back(HPYLM_BETA);
	}
	// 木構造で子ノードを生成した際に全てのHTSSBの同じ位置に子ノードを生成する
	Node* generate_and_add_new_child_to(Node* parent){
		assert(parent != NULL);
		// まず木構造上で子ノードを作る
		Node* generated_child_on_structure = NULL;
		if(parent->_htssb_owner_id == 0){	// parentが木構造上のノードの場合
			generated_child_on_structure = parent->generate_child();
		}else{	// parentが別のノードの遷移確率用TSSB上のノードだった場合
			Node* parent_on_cluster = _structure_tssb->find_node_by_tracing_horizontal_indices(parent);
			generated_child_on_structure = parent_on_cluster->generate_child();
		}
		assert(generated_child_on_structure != NULL);
		// 遷移確率用TSSBをセット
		generated_child_on_structure->copy_transition_tssb_from_structure(_structure_tssb);
		Node* myself_on_htssb = generated_child_on_structure->find_same_node_on_transition_tssb();
		assert(myself_on_htssb != NULL);
		generated_child_on_structure->_transition_tssb_myself = myself_on_htssb;

		Node* return_child = generated_child_on_structure;	// 実際に返すノード
		// 木構造上の全ノードのHTSSBにノードを追加
		_generate_and_add_new_child_to_all_htssb(_structure_tssb->_root, parent, generated_child_on_structure, return_child);
		// ポインタを張る
		//// 木構造上とHTSSB上のそれぞれお同じノード間のポインタ
		Node* generated_child_on_htssb = generated_child_on_structure->_transition_tssb_myself;
		assert(generated_child_on_htssb != NULL);
		generated_child_on_htssb->_structure_tssb_myself = generated_child_on_htssb;
		//// 木構造上の親ノードのHTSSBの自分と同じ位置のノードへのポインタ
		Node* iterator_on_structure = generated_child_on_structure;
		Node* parent_on_structure = iterator_on_structure->_parent;
		Node* generated_child_on_parent_htssb = generated_child_on_htssb;
		while(parent_on_structure != NULL){
			assert(iterator_on_structure->_transition_tssb_myself != NULL);
			// 木構造上での親ノードが持つHTSSBにある対応するノードを取る
			generated_child_on_parent_htssb = parent_on_structure->_transition_tssb->find_node_by_tracing_horizontal_indices(generated_child_on_structure);
			assert(generated_child_on_parent_htssb != NULL);
			// ポインタを張る
			generated_child_on_htssb->_parent_transition_tssb_myself = generated_child_on_parent_htssb;
			generated_child_on_parent_htssb->_child_transition_tssb_myself = generated_child_on_htssb;
			generated_child_on_htssb->_structure_tssb_myself = generated_child_on_structure;
			assert(generated_child_on_htssb->_structure_tssb_myself->_identifier == generated_child_on_structure->_identifier);
			// 木構造上で次の親ノードへ
			iterator_on_structure = parent_on_structure;
			parent_on_structure = iterator_on_structure->_parent;
			generated_child_on_htssb = generated_child_on_parent_htssb;
		}
		if(return_child->_depth_v > _max_depth){
			_max_depth = return_child->_depth_v;		
			while(_max_depth >= _hpylm_d_m.size()){
				_hpylm_d_m.push_back(HPYLM_D);
			}			
			while(_max_depth >= _hpylm_theta_m.size()){
				_hpylm_theta_m.push_back(HPYLM_THETA);
			}	
		}
		return return_child;
	}
	void _generate_and_add_new_child_to_all_htssb(Node* iterator_on_structure, Node* parent, Node* generated_child_on_structure, Node* &return_child){
		// iteratorとgenerated_childが同一の場合はすでに追加されているのでスキップ
		if(iterator_on_structure->_identifier != generated_child_on_structure->_identifier){
			assert(iterator_on_structure->_transition_tssb != NULL);
			int owner_id_on_structure_parent_belongs = parent->_htssb_owner_id;
			int child_id_to_generate = generated_child_on_structure->_identifier;
			// 遷移確率用TSSBの同じ位置に子ノードを挿入
			Node* parent_on_htssb = iterator_on_structure->_transition_tssb->find_node_by_tracing_horizontal_indices(parent);
			assert(parent_on_htssb != NULL);
			assert(parent_on_htssb->_identifier == generated_child_on_structure->_parent->_identifier);
			Node* child_on_htssb = new Node(parent_on_htssb, child_id_to_generate);
			// child_on_htssb->_htssb_owner_id = iterator_on_structure->_identifier;
			// child_on_htssb->_htssb_owner = iterator_on_structure;
			if(child_on_htssb->_htssb_owner_id == owner_id_on_structure_parent_belongs){	// 親と同じTSSB上の子ノードを返す
				return_child = child_on_htssb;
			}
			parent_on_htssb->add_child(child_on_htssb);
		}
		for(const auto &child: iterator_on_structure->_children){
			_generate_and_add_new_child_to_all_htssb(child, parent, generated_child_on_structure, return_child);
		}
	}
	Node* sample_node_on_tssb(TSSB* tssb){
		assert(tssb->_owner_id != 0);
		return _sample_node_on_tssb_by_iterating_node(tssb->_root);
	}
	// 止まるノードを決定する
	Node* _sample_node_on_tssb_by_iterating_node(Node* iterator){
		assert(iterator != NULL);
		assert(iterator->_htssb_owner_id != 0);
		double head = compute_expectation_of_vertical_sbr_ratio_on_node(iterator);
		iterator->_children_stick_length = 1 - iterator->_stick_length * head;
		double bernoulli = Sampler::uniform(0, 1);
		if(bernoulli <= head){			// 表が出たらこのノードに降りる
			return iterator;
		}
		// 子ノードがある場合
		for(int i = 0;i < iterator->_children.size();i++){
			Node* child = iterator->_children[i];
			assert(child != NULL);
			double head = compute_expectation_of_horizontal_sbr_ratio_on_node(child);
			double bernoulli = Sampler::uniform(0, 1);
			if(bernoulli <= head){		// 表が出たら次に止まるかどうかを決める
				return _sample_node_on_tssb_by_iterating_node(child);
			}
		}
		// ない場合生成しながらコインを投げる
		while(true){
			Node* child = generate_and_add_new_child_to(iterator);
			double head = compute_expectation_of_horizontal_sbr_ratio_on_node(child);
			double bernoulli = Sampler::uniform(0, 1);
			if(bernoulli <= head){		// 表が出たら次に止まるかどうかを決める
				return _sample_node_on_tssb_by_iterating_node(child);
			}
		}
	}
	// [0, 1)の一様分布からノードをサンプリング
	Node* retrospective_sampling_on_tssb(double uniform, TSSB* tssb, double total_stick_length = 1.0){
		assert(tssb->_owner_id != 0);
		Node* root = tssb->_root;
		double ratio = compute_expectation_of_vertical_sbr_ratio_on_node(root);
		double sum_probability = ratio;
		root->_stick_length = total_stick_length;
		root->_children_stick_length = total_stick_length * (1.0 - ratio);
		return _retrospective_sampling(uniform, sum_probability, root);
	}
	Node* _retrospective_sampling(double uniform, double &sum_probability, Node* node){
		if(uniform <= sum_probability){
			return node;
		}
		// 棒の長さとノードの確率の関係に気をつける
		// [<------------------- 棒の長さ -------------------]
		// [<--親ノードの確率--><---子ノードに割り当てる長さ --->]
		//					  [<---子1の棒---><---子2の棒--->]
		assert(node->_children_stick_length > 0);
		double rest_stick_length = node->_children_stick_length;	// 子ノードに割り当てる棒の長さの総和
		double sum_stick_length_over_children = 0;			// 子ノードを走査する時の走査済みの棒の長さ
		Node* last_node = NULL;
		for(int i = 0;i < node->_children.size();i++){
			Node* child = node->_children[i];
			double alpha = _alpha * pow(_lambda, child->_depth_v);
			double ratio_h = compute_expectation_of_horizontal_sbr_ratio_on_node(child);
			double ratio_v = compute_expectation_of_vertical_sbr_ratio_on_node(child);
			if(uniform <= sum_probability + sum_stick_length_over_children + rest_stick_length * ratio_h){
				// rest_stick_length * ratio_hだけだとこのノードの棒の長さ（つまりこのノード+子ノードに割り当てる棒）なので
				// ratio_vも掛けてこのノードで止まる確率にする必要がある
				sum_probability += sum_stick_length_over_children + rest_stick_length * ratio_h * ratio_v;
				if(uniform <= sum_probability){
					return child;
				}
				if(child->has_child()){
					return _retrospective_sampling(uniform, sum_probability, child);
				}
				// 子ノード領域に当たった場合、uniformを超えるまで棒を折り続ける
				Node* _child = generate_and_add_new_child_to(child);
				double ratio_h = compute_expectation_of_horizontal_sbr_ratio_on_node(_child);
				_child->_stick_length = child->_children_stick_length * ratio_h;
				double alpha = _alpha * pow(_lambda, _child->_depth_v);
				double ratio_v = compute_expectation_of_vertical_sbr_ratio_on_node(_child);
				_child->_probability = _child->_stick_length * ratio_v;
				_child->_children_stick_length = _child->_stick_length * (1.0 - ratio_v);
				assert(child->has_child());
				return _retrospective_sampling(uniform, sum_probability, child);
			}
			sum_stick_length_over_children += child->_stick_length;
			rest_stick_length *= 1.0 - ratio_h;
			last_node = child;
		}
		// 見つからなかったら一番右端のノードを返す
		if(last_node != NULL){
			if(last_node->has_child()){
				return _retrospective_sampling(uniform, sum_probability, last_node);
			}
			return last_node;
		}
		return NULL;
	}
	void add_customer_to(Node* target_on_htssb){
		assert(target_on_htssb != NULL);
		assert(target_on_htssb->_htssb_owner_id != 0);
		double alpha = _alpha * pow(_lambda, target_on_htssb->_depth_v);
		_add_customer_to_vertical_crp(alpha, target_on_htssb);
		_add_customer_to_horizontal_crp(_gamma, target_on_htssb);
	}
	void _add_customer_to_vertical_crp(double alpha, Node* iterator){
		assert(iterator != NULL);
		assert(iterator->_htssb_owner_id != 0);
		bool new_table_generated = false;
		double ratio_v = compute_expectation_of_vertical_sbr_ratio_on_node(iterator);
		iterator->add_customer_to_vertical_crp(alpha, ratio_v, new_table_generated);
		Node* iterator_on_parent_htssb = iterator->_parent_transition_tssb_myself;
		if(new_table_generated && iterator_on_parent_htssb != NULL){
			_add_customer_to_vertical_crp(alpha, iterator_on_parent_htssb);
		}
	}
	void _add_customer_to_horizontal_crp(double gamma, Node* iterator){
		assert(iterator != NULL);
		assert(iterator->_htssb_owner_id != 0);
		bool new_table_generated = false;
		double ratio_h = compute_expectation_of_horizontal_sbr_ratio_on_node(iterator);
		iterator->add_customer_to_horizontal_crp(gamma, ratio_h, new_table_generated);
		Node* iterator_on_parent_htssb = iterator->_parent_transition_tssb_myself;
		if(new_table_generated && iterator_on_parent_htssb != NULL){
			_add_customer_to_horizontal_crp(gamma, iterator_on_parent_htssb);
		}
	}
	void remove_customer_from(Node* target_on_htssb){
		assert(target_on_htssb != NULL);
		assert(target_on_htssb->_htssb_owner_id != 0);
		_remove_customer_from_vertical_crp(target_on_htssb);
		_remove_customer_from_horizontal_crp(target_on_htssb);
	}
	void _remove_customer_from_vertical_crp(Node* iterator){
		assert(iterator != NULL);
		assert(iterator->_htssb_owner_id != 0);
		bool empty_table_deleted = false;
		iterator->remove_customer_from_vertical_crp(empty_table_deleted);
		Node* iterator_on_parent_htssb = iterator->_parent_transition_tssb_myself;
		if(empty_table_deleted && iterator_on_parent_htssb != NULL){
			_remove_customer_from_vertical_crp(iterator_on_parent_htssb);
		}
	}
	void _remove_customer_from_horizontal_crp(Node* iterator){
		assert(iterator != NULL);
		bool empty_table_deleted = false;
		iterator->remove_customer_from_horizontal_crp(empty_table_deleted);
		Node* iterator_on_parent_htssb = iterator->_parent_transition_tssb_myself;
		if(empty_table_deleted && iterator_on_parent_htssb != NULL){
			_remove_customer_from_horizontal_crp(iterator_on_parent_htssb);
		}
	}
	// クラスタリング用TSSBから客が消える場合、木構造が変化する可能性があるので専用メソッドを用意
	void remove_clustering_customer_from_node(Node* node_on_structure){
		assert(node_on_structure->_htssb_owner_id == 0);
		_remove_clustering_customer_from_vertical_crp_on_node(node_on_structure);
		_remove_clustering_customer_from_horizontal_crp_on_node(node_on_structure);
	}
	// 客を除去
	void _remove_clustering_customer_from_vertical_crp_on_node(Node* target_on_structure){
		// cout << "remove_customer_from_vertical_crp: " << tssb_identifier << ", " << node->_identifier << endl;
		Table* table = target_on_structure->get_vertical_table();
		assert(table != NULL);
		bool empty_table_deleted = false;
		table->remove_customer(empty_table_deleted);
		target_on_structure->decrement_vertical_stop_count();
		_decrement_clustering_vertical_pass_counts_on_node(target_on_structure->_parent);
	}
	void _decrement_clustering_vertical_pass_counts_on_node(Node* parent_on_cluster){
		if(parent_on_cluster == NULL){
			return;
		}
		parent_on_cluster->decrement_vertical_pass_count();
		delete_invalid_children(parent_on_cluster);
		_decrement_clustering_vertical_pass_counts_on_node(parent_on_cluster->_parent);
	}
	void _remove_clustering_customer_from_horizontal_crp_on_node(Node* target_on_structure){
		// cout << "remove_customer_from_horizontal_crp: " << tssb_identifier << ", " << node->_identifier << endl;
		Table* table = target_on_structure->get_horizontal_table();
		assert(table != NULL);
		bool empty_table_deleted = false;
		table->remove_customer(empty_table_deleted);
		// 通過回数・停止回数を減らす
		Node* stopped_child = target_on_structure;
		Node* parent = target_on_structure->_parent;
		while(parent){
			bool found = false;
			for(int i = parent->_children.size() - 1;i >= 0;i--){	// 逆向きに辿らないと通過ノードが先に消えてしまう
				Node* child = parent->_children[i];
				if(child == stopped_child){
					found = true;
					child->decrement_horizontal_stop_count();
					continue;
				}
				if(found){
					child->decrement_horizontal_pass_count();
				}
			}
			delete_invalid_children(parent);
			stopped_child = parent;
			parent = parent->_parent;
		}
		// ルートノードのカウントを減らす
		stopped_child->decrement_horizontal_stop_count();
	}
	// 縦の棒折り過程における、棒を折る比率を計算。親のTSSBから階層的に生成
	double compute_expectation_of_vertical_sbr_ratio_on_node(Node* target_on_htssb){
		assert(target_on_htssb->_htssb_owner_id != 0);	// 木構造上のノードだった場合は計算できない
		Node* owner_on_structure = target_on_htssb->_htssb_owner;
		assert(owner_on_structure != NULL);
		// cout << "target" << endl << "	";
		// target_on_htssb->dump();
		double sbr_ratio = 0;
		int depth_v = owner_on_structure->_depth_v;
		int num_parents = depth_v;

		// ルートまで遡る
		Node* target_on_parent_htssb = target_on_htssb;
		while(target_on_parent_htssb->_parent_transition_tssb_myself != NULL){
			target_on_parent_htssb = target_on_parent_htssb->_parent_transition_tssb_myself;
		}

		// 木構造上での基準となるノードを選ぶ
		assert(owner_on_structure != NULL);
		Node** iterators_on_structure_from_root_to_myself = owner_on_structure->_nodes_from_root_to_myself;
		// Node* iterator_on_structure = owner_on_structure;
		// iterators_on_structure_from_root_to_myself[depth_v] = iterator_on_structure;
		// for(int n = 0;n < num_parents;n++){
		// 	iterator_on_structure = iterator_on_structure->_parent;
		// 	assert(iterator_on_structure != NULL);
		// 	iterators_on_structure_from_root_to_myself[depth_v - n - 1] = iterator_on_structure;
		// }
		// 階層TSSBなので親TSSBのSBPを全て睿珊しないと次ノードのSBPを計算できない
		// ノードの深さをdとすると(d+1)^2回計算する必要がある
		// ルートノードの深さは0
		double* stop_ratio_over_parent = owner_on_structure->_stop_ratio_v_over_parent;
		double* stop_probability_over_parent = owner_on_structure->_stop_probability_v_over_parent;
		for(int n = 0;n < num_parents + 1;n++){
			// cout << "n = " << n << endl;
			// 木構造上での基準となるノードを選ぶ
			// nが増えるごとに木構造を下に降りていく
			Node* iterator_on_structure = iterators_on_structure_from_root_to_myself[n];
			assert(iterator_on_structure != NULL);
			assert(iterator_on_structure->_transition_tssb_myself != NULL);
			// iterator_on_structure->_transition_tssb_myself->dump();
			// トップレベルのノードから順に停止確率を計算
			double sum_parent_stop_probability = 0;
			// Node* iterator_on_htssb = iterator_on_structure->_transition_tssb->_root;	// 遷移確率用TSSBのルートから始める
			Node* iterator_on_htssb = target_on_parent_htssb->_nodes_from_root_to_myself[0];
			assert(iterator_on_htssb != NULL);
			for(int m = 0;m < num_parents + 1;m++){
				assert(iterator_on_htssb != NULL);
				// cout << "m = " << m << endl;
				// iterator_on_htssb->dump();
				// cout << "iterator_on_htssb = " << iterator_on_htssb->_identifier << endl;
				if(iterator_on_structure->_depth_v == 0){	// クラスタリング用TSSBの親ノードの場合
					int pass_count = iterator_on_htssb->_pass_count_v;
					int stop_count = iterator_on_htssb->_stop_count_v;
					// cout << "pass_count = " << pass_count << ", stop_count = " << stop_count << endl;
					double ratio_v = (1.0 + stop_count) / (1.0 + _alpha + stop_count + iterator_on_htssb->_pass_count_v);
					// cout << "ratio_v = " << ratio_v << endl;
					stop_ratio_over_parent[m] = ratio_v;
				}else{	// 親の遷移確率用HTSSBから生成
					int pass_count = iterator_on_htssb->_pass_count_v;
					int stop_count = iterator_on_htssb->_stop_count_v;
					// cout << "pass_count = " << pass_count << ", stop_count = " << stop_count << endl;
					double parent_stop_probability = stop_probability_over_parent[m];
					// cout << "parent_stop_probability = " << parent_stop_probability << endl;
					// cout << "sum_parent_stop_probability = " << sum_parent_stop_probability << endl;
					double alpha = _alpha * pow(_lambda, iterator_on_htssb->_depth_v);
					// cout << "alpha = " << alpha << endl;
					double ratio_v = (alpha * parent_stop_probability + stop_count) / (alpha * (1.0 - sum_parent_stop_probability) + stop_count + pass_count);
					// cout << "ratio_v = " << ratio_v << endl;
					stop_ratio_over_parent[m] = ratio_v;
					sum_parent_stop_probability += parent_stop_probability;
					sbr_ratio = ratio_v;
				}
				// 親から子へ降りていく
				if(m < num_parents){
					iterator_on_htssb = target_on_parent_htssb->_nodes_from_root_to_myself[m + 1];
				}
			}
			// 計算した棒を折る比率から確率を計算
			if(n < num_parents){
				double rest_stick_length = 1;
				for(int m = 0;m < num_parents + 1;m++){
					// cout << "m = " << m << endl;
					double ratio_v = stop_ratio_over_parent[m];
					double stop_probability = rest_stick_length * ratio_v;
					// cout << "stop_probability = " << stop_probability << endl;
					rest_stick_length *= 1.0 - ratio_v;
					// cout << "rest_stick_length = " << rest_stick_length << endl;
					stop_probability_over_parent[m] = stop_probability;
				}
				target_on_parent_htssb = target_on_parent_htssb->_child_transition_tssb_myself;
				assert(target_on_parent_htssb);
				// cout << "target switched!" << endl;
				// target_on_parent_htssb->dump();
			}
		}
		return sbr_ratio;
	}
	// 横の棒折り過程における、棒を折る比率を計算。親のTSSBから階層的に生成
	double compute_expectation_of_horizontal_sbr_ratio_on_node(Node* target_on_htssb){
		assert(target_on_htssb != NULL);
		Node* owner_on_structure = target_on_htssb->_htssb_owner;
		assert(owner_on_structure != NULL);
		int depth_v = owner_on_structure->_depth_v;
		int depth_h = target_on_htssb->_depth_h;
		if(depth_v == 0){	// ルートノードなら必ず止まる
			return 1;
		}
		double sbr_ratio = 0;
		int num_parents = depth_v;

		// ルートまで遡る
		Node* iterator_on_htssb = target_on_htssb;
		while(iterator_on_htssb->_parent_transition_tssb_myself != NULL){
			iterator_on_htssb = iterator_on_htssb->_parent_transition_tssb_myself;
		}

		Node* _ = target_on_htssb->_structure_tssb_myself;
		assert(_ != NULL);
		double* stop_ratio_over_parent = _->_stop_ratio_h_over_parent;
		double* stop_probability_over_parent = _->_stop_probability_h_over_parent;
		for(int n = 0;n < num_parents + 1;n++){
			// cout << "n = " << n << endl;
			// トップレベルのノードから順に停止確率を計算
			double sum_parent_stop_probability = 0;
			Node* parent_contains_target_on_htssb = iterator_on_htssb->_parent;
			assert(parent_contains_target_on_htssb);
			// parent_contains_target_on_htssb->dump();
			// cout << "parent contains" << endl << "	";
			// parent_contains_target_on_htssb->dump();
			for(int m = 0;m < depth_h + 1;m++){		// 自分自身も含めるので+1
				Node* child_on_htssb = parent_contains_target_on_htssb->_children[m];
				assert(child_on_htssb);
				// cout << "m = " << m << endl << "	";
				// child_on_htssb->dump();
				if(iterator_on_htssb->_htssb_owner_id == 1){	// 親ノードの場合
					int pass_count = child_on_htssb->_pass_count_h;
					int stop_count = child_on_htssb->_stop_count_h;
					// cout << "pass_count = " << pass_count << ", stop_count = " << stop_count << endl;
					double ratio_h = (1.0 + stop_count) / (1.0 + _gamma + stop_count + pass_count);
					// cout << "ratio_h = " << ratio_h << endl;
					stop_ratio_over_parent[m] = ratio_h;
				}else{
					int pass_count = child_on_htssb->_pass_count_h;
					int stop_count = child_on_htssb->_stop_count_h;
					// cout << "pass_count = " << pass_count << ", stop_count = " << stop_count << endl;
					double parent_stop_probability = stop_probability_over_parent[m];
					// cout << "parent_stop_probability = " << parent_stop_probability << endl;
					// cout << "sum_parent_stop_probability = " << sum_parent_stop_probability << endl;
					double ratio_h = (_gamma * parent_stop_probability + stop_count) / (_gamma * (1.0 - sum_parent_stop_probability) + stop_count + pass_count);
					// cout << "ratio_h = " << ratio_h << endl;
					stop_ratio_over_parent[m] = ratio_h;
					sum_parent_stop_probability += parent_stop_probability;
					// cout << "sum_parent_stop_probability = " << sum_parent_stop_probability << endl;
					sbr_ratio = ratio_h;
				}

			}
			if(n < num_parents){
				double rest_stick_length = 1;
				for(int m = 0;m < depth_h + 1;m++){
					double ratio_h = stop_ratio_over_parent[m];
					double stop_probability = rest_stick_length * ratio_h;
					// cout << "stop_probability = " << stop_probability << endl;
					rest_stick_length *= 1.0 - ratio_h;
					// cout << "rest_stick_length = " << rest_stick_length << endl;
					stop_probability_over_parent[m] = stop_probability;
				}
				// 親から子へ降りていく
				iterator_on_htssb = iterator_on_htssb->_child_transition_tssb_myself;
			}
		}
		return sbr_ratio;
	}
	void update_stick_length_of_tssb(TSSB* tssb){
		assert(tssb->_owner_id != 0);
		Node* root = tssb->_root;
		double ratio_v = compute_expectation_of_vertical_sbr_ratio_on_node(root);
		double sum_probability = ratio_v;
		root->_stick_length = 1;
		root->_children_stick_length = 1.0 - ratio_v;
		root->_probability = ratio_v;
		_update_stick_length(sum_probability, root);
	}
	void _update_stick_length(double &sum_probability, Node* node){
		assert(node->_children_stick_length > 0);
		double rest_stick_length = node->_children_stick_length;
		for(int i = 0;i < node->_children.size();i++){
			Node* child = node->_children[i];
			double ratio_h = compute_expectation_of_horizontal_sbr_ratio_on_node(child);
			child->_stick_length = rest_stick_length * ratio_h;
			double ratio_v = compute_expectation_of_vertical_sbr_ratio_on_node(child);
			child->_probability = child->_stick_length * ratio_v;
			sum_probability += child->_probability;
			rest_stick_length *= 1.0 - ratio_h;
			double alpha = _alpha * pow(_lambda, child->_depth_v);
			child->_children_stick_length = child->_stick_length * (1.0 - ratio_v);
			if(child->has_child()){
				_update_stick_length(sum_probability, child);
			}
		}
	}
	void delete_invalid_children(Node* parent){
		vector<Node*> &children = parent->_children;
		for(int i = children.size() - 1;i >= 0;i--){
			Node* child = children[i];
			bool success = delete_node_if_needed(child);
			if(success == false){	// 失敗したらそれ以上は消さない
				break;
			}
		}
	}
	bool delete_node_if_needed(Node* target){
		Node* target_on_structure = NULL;
		if(target->_htssb_owner_id == 0){	// targetが木構造上のノードの場合
			target_on_structure = target;
		}else{								// targetがHTSSB上のノードの場合
			target_on_structure = target->_structure_tssb_myself;
		}
		assert(target_on_structure != NULL);
		if(target_on_structure->_depth_v == 0){
			return false;
		}
		assert(target_on_structure->_parent != NULL);
		int delete_id = target_on_structure->_identifier;
		int pass_count_v = target_on_structure->get_vertical_pass_count();
		int stop_count_v = target_on_structure->get_vertical_stop_count();
		int pass_count_h = target_on_structure->get_horizontal_pass_count();
		int stop_count_h = target_on_structure->get_horizontal_stop_count();
		if(pass_count_v != 0){
			return false;
		}
		if(stop_count_v != 0){
			return false;
		}
		if(pass_count_h != 0){
			return false;
		}
		if(stop_count_h != 0){
			return false;
		}
		Node* parent_on_structure = target_on_structure->_parent;
		Node* delete_node = target_on_structure->_parent->delete_child_node(delete_id);
		if(delete_node != NULL){
			TSSB* delete_tssb = delete_node->_transition_tssb;
			delete delete_node;
			delete delete_tssb;
		}
		_delete_node_on_all_htssb(delete_id, _structure_tssb->_root, parent_on_structure);
		return true;
	}
	void _delete_node_on_all_htssb(int delete_id, Node* iterator_on_structure, Node* target_parent_on_structure){
		assert(target_parent_on_structure != NULL);
		assert(iterator_on_structure->_transition_tssb != NULL);
		// 遷移確率用TSSBでの同じ位置の子ノードを削除
		Node* parent_on_htssb = iterator_on_structure->_transition_tssb->find_node_by_tracing_horizontal_indices(target_parent_on_structure);
		assert(parent_on_htssb != NULL);
		assert(parent_on_htssb->_htssb_owner_id != 0);
		Node* delete_node = parent_on_htssb->delete_child_node(delete_id);
		if(delete_node != NULL){
			TSSB* delete_tssb = delete_node->_transition_tssb;
			delete delete_node;
			delete delete_tssb;
		}
		for(const auto &child: iterator_on_structure->_children){
			_delete_node_on_all_htssb(delete_id, child, target_parent_on_structure);
		}
	}
	// "A Bayesian Interpretation of Interpolated Kneser-Ney" Appendix C参照
	// http://www.gatsby.ucl.ac.uk/~ywteh/research/compling/hpylm.pdf
	void sum_auxiliary_variables_recursively_for_hpylm(Node* parent, vector<double> &sum_log_x_u_m, vector<double> &sum_y_ui_m, vector<double> &sum_1_y_ui_m, vector<double> &sum_1_z_uwkj_m){
		for(const auto &child: parent->_children){
			int depth = child->_depth_v;
			assert(depth < _hpylm_d_m.size());
			assert(depth < _hpylm_theta_m.size());

			double d = _hpylm_d_m[depth];
			double theta = _hpylm_theta_m[depth];
			sum_log_x_u_m[depth] += child->_hpylm->auxiliary_log_x_u(theta);	// log(x_u)
			sum_y_ui_m[depth] += child->_hpylm->auxiliary_y_ui(d, theta);		// y_ui
			sum_1_y_ui_m[depth] += child->_hpylm->auxiliary_1_y_ui(d, theta);	// 1 - y_ui
			sum_1_z_uwkj_m[depth] += child->_hpylm->auxiliary_1_z_uwkj(d);		// 1 - z_uwkj

			sum_auxiliary_variables_recursively_for_hpylm(child, sum_log_x_u_m, sum_y_ui_m, sum_1_y_ui_m, sum_1_z_uwkj_m);
		}
	}
	// dとθの推定
	void sample_hpylm_hyperparameters(){
		assert(_max_depth < _hpylm_d_m.size());
		assert(_max_depth < _hpylm_theta_m.size());

		// 親ノードの深さが0であることに注意
		vector<double> sum_log_x_u_m(_max_depth + 1, 0.0);
		vector<double> sum_y_ui_m(_max_depth + 1, 0.0);
		vector<double> sum_1_y_ui_m(_max_depth + 1, 0.0);
		vector<double> sum_1_z_uwkj_m(_max_depth + 1, 0.0);

		// _root
		HPYLM* root = _structure_tssb->_root->_hpylm;
		sum_log_x_u_m[0] = root->auxiliary_log_x_u(_hpylm_theta_m[0]);			// log(x_u)
		sum_y_ui_m[0] = root->auxiliary_y_ui(_hpylm_d_m[0], _hpylm_theta_m[0]);			// y_ui
		sum_1_y_ui_m[0] = root->auxiliary_1_y_ui(_hpylm_d_m[0], _hpylm_theta_m[0]);		// 1 - y_ui
		sum_1_z_uwkj_m[0] = root->auxiliary_1_z_uwkj(_hpylm_d_m[0]);				// 1 - z_uwkj

		// それ以外
		sum_auxiliary_variables_recursively_for_hpylm(_structure_tssb->_root, sum_log_x_u_m, sum_y_ui_m, sum_1_y_ui_m, sum_1_z_uwkj_m);

		for(int u = 0;u <= _max_depth;u++){
			_hpylm_d_m[u] = Sampler::beta(_hpylm_a_m[u] + sum_1_y_ui_m[u], _hpylm_b_m[u] + sum_1_z_uwkj_m[u]);
			_hpylm_theta_m[u] = Sampler::gamma(_hpylm_alpha_m[u] + sum_y_ui_m[u], _hpylm_beta_m[u] - sum_log_x_u_m[u]);
		}
		// 不要な深さのハイパーパラメータを削除
		int num_remove = _hpylm_d_m.size() - _max_depth - 1;
		for(int n = 0;n < num_remove;n++){
			_hpylm_d_m.pop_back();
			_hpylm_theta_m.pop_back();
			_hpylm_a_m.pop_back();
			_hpylm_b_m.pop_back();
			_hpylm_alpha_m.pop_back();
			_hpylm_beta_m.pop_back();
		}
	}
};

#endif