#include "../include/LocationConfig.hpp"

/** CANONICAL **/
LocationConfig::LocationConfig() : path_("/"), autoindex_(false), root_override_("") { methods_.push_back("GET"); }
LocationConfig::LocationConfig(const std::string& path, const std::vector<std::string>& methods, bool autoindex, const std::string& root_override) : path_(path), methods_(methods), autoindex_(autoindex), root_override_(root_override) {}
LocationConfig::LocationConfig(const LocationConfig& other) : path_(other.path_), methods_(other.methods_), autoindex_(other.autoindex_), root_override_(other.root_override_) {}
LocationConfig& LocationConfig::operator=(const LocationConfig& other) {
	if (this != &other) {
		path_ = other.path_;
		methods_ = other.methods_;
		autoindex_ = other.autoindex_;
		root_override_ = other.root_override_;
	}
	return *this;
}
LocationConfig::~LocationConfig() {}

/** FUNCTIONS **/
std::string LocationConfig::getPath() const { return path_; }
std::vector<std::string> LocationConfig::getMethods() const { return methods_; }
bool LocationConfig::getAutoIndex() const { return autoindex_; }
std::string LocationConfig::getRootOverride() const { return root_override_; }

void LocationConfig::setPath(const std::string& path) { path_ = path; }
void LocationConfig::setMethods(const std::vector<std::string>& methods) { methods_ = methods; }
void LocationConfig::setAutoIndex(bool autoindex) { autoindex_ = autoindex; }
void LocationConfig::setRootOverride(const std::string& root_override) { root_override_ = root_override; }
