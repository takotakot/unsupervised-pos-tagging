#pragma once
#include <boost/python.hpp>
#include <unordered_map>
#include <vector>
#include <iostream>

namespace {
	template<class T>
	boost::python::list list_from_vector(std::vector<T> &vec){  
		 boost::python::list list;
		 typename std::vector<T>::const_iterator it;
		 for(it = vec.begin(); it != vec.end(); ++it){
			  list.append(*it);
		 }
		 return list;
	}

	template<class T1,class T2>
	boost::python::dict dict_from_map(std::unordered_map<T1,T2> &map_){  
		 boost::python::dict py_dict;
		 typename std::unordered_map<T1,T2>::const_iterator it;
		 for(it = map_.begin(); it != map_.end(); ++it){
			  py_dict[it->first]=it->second;        
		 }
		 return py_dict;  
	}
	double factorial(double n) {
		if (n == 0){
			return 1;
		}
		return n * factorial(n - 1);
	}
	void split_word_by(const std::wstring &str, wchar_t delim, std::vector<std::wstring> &elems){
		elems.clear();
	    std::wstring item;
	    for(wchar_t ch: str){
	        if (ch == delim){
	            if (!item.empty()){
	                elems.push_back(item);
	            }
	            item.clear();
	        }
	        else{
	            item += ch;
	        }
	    }
	    if (!item.empty()){
	        elems.push_back(item);
	    }
	}
	void show_progress(int step, int total){
		using std::cout;
		using std::endl;
		double progress = step / (double)(total - 1);
		int barWidth = 30;

		cout << "　" << step << " / " << total << " [";
		int pos = barWidth * progress;
		for(int i = 0; i < barWidth; ++i){
			if (i < pos) cout << "=";
			else if (i == pos) cout << ">";
			else cout << " ";
		}
		cout << "] " << int(progress * 100.0) << " %\r";
		cout.flush();
		if(step == total){
			cout << endl;
		}
	}
	double compute_sbr_probability_given_params(std::vector<double> &params){
		double probability = 1;
		for(int i = 0;i < params.size() - 1;i++){
			probability *= 1 - params[i];
		}
		return probability * params.back();
	}
	double compute_sbr_probability_given_params_reverse(std::vector<double> &params_reverse){
		double probability = 1;
		for(int i = params_reverse.size() - 1;i > 0;i--){
			probability *= 1 - params_reverse[i];
		}
		return probability * params_reverse.front();
	}
	void compute_sbr_probability_and_sum_given_params(std::vector<double> &params, double &probability, double &sum){
		double stick = 1;
		probability = 1;
		sum = 0;
		for(int i = 0;i < params.size() - 1;i++){
			probability *= 1 - params[i];
			sum += stick * params[i];
			stick = stick * (1 - params[i]);
		}
		probability *= params.back();
		sum += stick * params.back();
	}
}