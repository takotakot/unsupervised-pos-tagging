#include <boost/format.hpp>
#include <cassert>
#include "cprintf.h"
#include "sampler.h"
#include "util.h"
#include "hpylm.hpp"
#include "tssb.hpp"
#include "node.hpp"

Node::Node(){
	_identifier = _auto_increment;
	_auto_increment++;
	_parent = NULL;
	init();
}
Node::Node(Node* parent){
	_identifier = _auto_increment;
	_auto_increment++;
	_parent = parent;
	init();
}
Node::Node(Node* parent, int identifier){
	_identifier = identifier;
	_parent = parent;
	init();
}
void Node::init(){
	_depth_v = (_parent != NULL) ? _parent->_depth_v + 1 : 0;
	_depth_h = (_parent != NULL) ? _parent->_children.size() : 0;
	_owner_id_on_structure = (_parent != NULL) ? _parent->_owner_id_on_structure : 0;
	_stick_length = -1;
	_children_stick_length = -1;
	_pass_count_v = 0;
	_stop_count_v = 0;
	_pass_count_h = 0;
	_stop_count_h = 0;
	_ref_count = 0;
	_probability = -1;
	_num_transitions_to_eos = 0;
	_num_transitions_to_other = 0;
	_transition_tssb = NULL;
	_transition_tssb_myself = NULL;
	_parent_transition_tssb_myself = NULL;
	_structure_tssb_myself = NULL;
	_bos_tssb_myself = NULL;
	_owner_on_structure = (_parent != NULL) ? _parent->_owner_on_structure : 0;
	_table_v = new Table();
	_table_h = new Table();
	_hpylm = NULL;
	init_arrays();
	init_horizontal_indices();
	init_pointers_from_root_to_myself();
}
void Node::init_hpylm(){
	_hpylm = new HPYLM(this);
}
void Node::init_arrays(){
	_stop_probability_v_over_parent = new double[_depth_v + 1];
	_stop_ratio_v_over_parent = new double[_depth_v + 1];
	_nodes_from_root_to_myself = new Node*[_depth_v + 1];
	_stop_probability_h_over_parent = new double[_depth_h + 1];
	_stop_ratio_h_over_parent = new double[_depth_h + 1];
	_horizontal_indices_from_root = new int[_depth_v];
}
void Node::init_horizontal_indices(){
	Node* iterator = this;
	for(int i = 0;i < _depth_v;i++){
		_horizontal_indices_from_root[_depth_v - i - 1] = iterator->_depth_h;
		iterator = iterator->_parent;
	}
}
void Node::init_pointers_from_root_to_myself(){
	Node* iterator = this;
	_nodes_from_root_to_myself[_depth_v] = iterator;
	for(int n = 0;n < _depth_v;n++){
		iterator = iterator->_parent;
		assert(iterator != NULL);
		_nodes_from_root_to_myself[_depth_v - n - 1] = iterator;
	}
}
Node::~Node(){
	delete _table_v;
	delete _table_h;
	delete[] _stop_probability_v_over_parent;
	delete[] _stop_ratio_v_over_parent;
	delete[] _nodes_from_root_to_myself;
	delete[] _horizontal_indices_from_root;
	delete[] _stop_probability_h_over_parent;
	delete[] _stop_ratio_h_over_parent;
	if(_hpylm != NULL){
		delete _hpylm;
	}
}
Node* Node::generate_child(){
	Node* child = new Node(this);
	add_child(child);
	return child;
}
void Node::add_child(Node* node){
	assert(node != NULL);
	_children.push_back(node);
}
Node* Node::find_same_node_on_transition_tssb(){
	return _transition_tssb->find_node_by_tracing_horizontal_indices(this);
}
int Node::get_vertical_stop_count(){
	return _stop_count_v;
}
int Node::get_vertical_pass_count(){
	return _pass_count_v;
}
int Node::get_horizontal_stop_count(){
	return _stop_count_h;
}
int Node::get_horizontal_pass_count(){
	return _pass_count_h;
}
Table* Node::get_vertical_table(){
	return _table_v;
}
Table* Node::get_horizontal_table(){
	return _table_h;
}
double Node::compute_transition_probability_to_eos(double tau0, double tau1){
	return (tau0 + _num_transitions_to_eos) / (tau0 + tau1 + _num_transitions_to_eos + _num_transitions_to_other);	
}
bool Node::has_child(){
	return _children.size() != 0;
}
// 遷移確率TSSBに客を追加
void Node::add_customer_to_vertical_crp(double concentration, double g0, bool &new_table_generated){
	Table* table = get_vertical_table();
	Node* parent = _parent;
	assert(table != NULL);
	int total_count = _pass_count_v + _stop_count_v;
	table->add_customer(concentration, g0, total_count, new_table_generated);
	// 停止回数・通過回数を更新
	increment_vertical_stop_count();
	while(parent){
		parent->increment_vertical_pass_count();
		parent = parent->_parent;
	}
}
void Node::increment_vertical_stop_count(){
	_stop_count_v += 1;
}
void Node::decrement_vertical_stop_count(){
	_stop_count_v -= 1;
	assert(_stop_count_v >= 0);
}
void Node::increment_vertical_pass_count(){
	_pass_count_v += 1;
}
void Node::decrement_vertical_pass_count(){
	_pass_count_v -= 1;
	assert(_pass_count_v >= 0);
}
void Node::add_customer_to_horizontal_crp(double concentration, double g0, bool &new_table_generated){
	Table* table = get_horizontal_table();
	assert(table != NULL);
	int total_count = _pass_count_h + _stop_count_h;
	table->add_customer(concentration, g0, total_count, new_table_generated);
	// 停止回数・通過回数を更新
	Node* stopped_child = this;
	Node* parent = _parent;
	while(parent){
		for(int i = 0;i < parent->_children.size();i++){
			Node* child = parent->_children[i];
			if(child == stopped_child){
				child->increment_horizontal_stop_count();
				break;
			}
			child->increment_horizontal_pass_count();
		}
		stopped_child = parent;
		parent = parent->_parent;
	}
	stopped_child->increment_horizontal_stop_count();
}
void Node::increment_horizontal_stop_count(){
	_stop_count_h += 1;
}
void Node::decrement_horizontal_stop_count(){
	_stop_count_h -= 1;
	assert(_stop_count_h >= 0);
}
void Node::increment_horizontal_pass_count(){
	_pass_count_h += 1;
}
void Node::decrement_horizontal_pass_count(){
	_pass_count_h -= 1;
	assert(_pass_count_h >= 0);
}
void Node::increment_transition_count_to_eos(){
	_num_transitions_to_eos += 1;
}
void Node::decrement_transition_count_to_eos(){
	_num_transitions_to_eos -= 1;
	assert(_num_transitions_to_eos >= 0);
}
void Node::increment_transition_count_to_other(){
	_num_transitions_to_other += 1;
}
void Node::decrement_transition_count_to_other(){
	_num_transitions_to_other -= 1;
	assert(_num_transitions_to_other >= 0);
}
void Node::increment_ref_count(){
	_ref_count += 1;
}
void Node::decrement_ref_count(){
	_ref_count -= 1;
	assert(_ref_count >= 0);
}
void Node::increment_word_assignment(id word_id){
	_num_word_assignment[word_id] += 1;
}
void Node::decrement_word_assignment(id word_id){
	auto itr = _num_word_assignment.find(word_id);
	assert(itr != _num_word_assignment.end());
	itr->second -= 1;
	assert(itr->second >= 0);
	if(itr->second == 0){
		_num_word_assignment.erase(itr);
	}
}
// 客を除去
void Node::remove_customer_from_vertical_crp(bool &empty_table_deleted){
	// cout << "remove_customer_from_vertical_crp: " << tssb_identifier << ", " << node->_identifier << endl;
	Table* table = get_vertical_table();
	assert(table != NULL);
	table->remove_customer(empty_table_deleted);
	// 停止回数・通過回数を更新
	decrement_vertical_stop_count();
	Node* parent = _parent;
	while(parent){
		parent->decrement_vertical_pass_count();
		parent = parent->_parent;
	}
}
void Node::remove_customer_from_horizontal_crp(bool &empty_table_deleted){
	// cout << "remove_customer_from_horizontal_crp: " << _identifier << "," << node->_identifier << endl;
	Table* table = get_horizontal_table();
	assert(table != NULL);
	table->remove_customer(empty_table_deleted);
	// 停止回数・通過回数を更新
	Node* stopped_child = this;
	Node* parent = _parent;
	while(parent){
		bool found = false;
		for(int i = parent->_children.size() - 1;i >= 0;i--){	// 逆向きに辿らないと通過ノードが先に消えてしまう
			Node* child = parent->_children[i];
			// cout << "foreach: " << child->_identifier << endl;
			// cout << "foreach: " << child->_identifier << endl;

			if(child == stopped_child){
				found = true;
				child->decrement_horizontal_stop_count();
				continue;
			}
			if(found){
				child->decrement_horizontal_pass_count();
			}
		}
		stopped_child = parent;
		parent = parent->_parent;
	}
	// ルートノードのカウントを減らす
	stopped_child->decrement_horizontal_stop_count();
}
bool Node::delete_node_if_needed(){
	int pass_count_v = get_vertical_pass_count();
	int stop_count_v = get_vertical_stop_count();
	int pass_count_h = get_horizontal_pass_count();
	int stop_count_h = get_horizontal_stop_count();
	if(pass_count_v + stop_count_v + pass_count_h + stop_count_h == 0 && _parent != NULL){
		// cout << pass_count_v << "," << stop_count_v << "," << pass_count_h << "," << stop_count_h << endl;
		// cout << "requesting parent " << _parent->_identifier << ", me = " << _identifier << endl;
		_parent->delete_child_node(_identifier);
		return true;
	}
	return false;
}
Node* Node::delete_child_node(int node_id){
	Node* return_node = NULL;
	for(int i = 0;i < _children.size();i++){
		Node* target = _children[i];
		if(target->_identifier == node_id){
			assert(target->get_vertical_pass_count() == 0);
			assert(target->get_vertical_stop_count() == 0);
			assert(target->get_horizontal_pass_count() == 0);
			assert(target->get_horizontal_stop_count() == 0);
			return_node = target;
			continue;
		}
		if(return_node != NULL){
			target->_depth_h -= 1;
			target->_horizontal_indices_from_root[target->_depth_v - 1] -= 1;
		}
	}
	if(return_node != NULL){
		_children.erase(_children.begin() + return_node->_depth_h);
	}
	return return_node;
}
void Node::dump(){
	cout << _dump() << endl;
}
string Node::_dump_indices(){
	string indices_str = "";
	for(int i = 0;i < _depth_v;i++){
		indices_str += std::to_string(_horizontal_indices_from_root[i]);
		indices_str += ",";
	}
	return indices_str;
}
string Node::_dump(){
	string indices_str = _dump_indices();
	string hpylm_str = "";
	if(_hpylm != NULL){
		hpylm_str = (boost::format("HPY[#c:%d,#t:%d,d:%d]") % _hpylm->_num_customers % _hpylm->_num_tables % _hpylm->_depth).str();
	}
	return (boost::format("$%d [vp:%d,vs:%d,hp:%d,hs:%d,ref:%d][len:%f,self:%f,ch:%f,p:%f,sp:%f][ow:$%d,dv:%d,dh:%d][%s]%s[eos:%d,other:%d]") 
		% _identifier % _pass_count_v % _stop_count_v % _pass_count_h % _stop_count_h % _ref_count % _stick_length 
		% (_stick_length - _children_stick_length) % _children_stick_length % _probability % _sum_probability % _owner_id_on_structure 
		% _depth_v % _depth_h % indices_str.c_str() % hpylm_str.c_str() % _num_transitions_to_eos % _num_transitions_to_other).str();
}

// 特殊なTSSBの識別でもこのIDを使うのでノードのIDの開始を少し大きな値にする
// ithmm.hのTSSB_STRUCTURE_IDとTSSB_BOS_IDよりも大きな値にする
int Node::_auto_increment = 10;