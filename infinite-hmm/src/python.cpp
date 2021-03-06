#include "python/model.h"
#include "python/corpus.h"
#include "python/dataset.h"
#include "python/dictionary.h"
#include "python/trainer.h"

using namespace ihmm;

BOOST_PYTHON_MODULE(ihmm){
	boost::python::class_<Dictionary>("dictionary")
	.def("string_to_word_id", &Dictionary::string_to_word_id)
	.def("is_id_unk", &Dictionary::is_id_unk)
	.def("is_string_unk", &Dictionary::is_string_unk)
	.def("save", &Dictionary::save)
	.def("load", &Dictionary::load);

	boost::python::class_<Corpus>("corpus")
	.def("add_words", &Corpus::python_add_words);

	boost::python::class_<Dataset>("dataset", boost::python::init<Corpus*, double, int, int>())
	.def("get_num_words", &Dataset::get_num_words)
	.def("get_dict", &Dataset::get_dict_obj, boost::python::return_internal_reference<>());

	boost::python::class_<Trainer>("trainer", boost::python::init<Dataset*, Model*>())
	.def("compute_log_p_dataset_train", &Trainer::compute_log_p_dataset_train)
	.def("compute_log_p_dataset_dev", &Trainer::compute_log_p_dataset_dev)
	.def("gibbs", &Trainer::gibbs);

	boost::python::class_<Model>("model", boost::python::init<int, Dataset*>())
	.def(boost::python::init<std::string>())
	.def("get_tags", &Model::python_get_valid_tags)
	.def("set_initial_alpha", &Model::set_initial_alpha)
	.def("set_initial_beta", &Model::set_initial_beta)
	.def("set_initial_gamma", &Model::set_initial_gamma)
	.def("set_initial_gamma_emission", &Model::set_initial_gamma_emission)
	.def("set_initial_beta_emission", &Model::set_initial_beta_emission)
	.def("viterbi_decode", &Model::python_viterbi_decode)
	.def("print_typical_words_assigned_to_each_tag", &Model::print_typical_words_assigned_to_each_tag)
	.def("save", &Model::save)
	.def("load", &Model::load);
}