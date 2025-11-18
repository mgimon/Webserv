#include "../include/LocationConfig.hpp"

/** CANONICAL **/
LocationConfig::LocationConfig() : redirect_(0, ""), path_("/"), autoindex_(false), root_override_("") {}
LocationConfig::LocationConfig(const std::string& path, const std::vector<std::string>& methods, bool autoindex, const std::string& root_override) : redirect_(0, ""), path_(path), methods_(methods), autoindex_(autoindex), root_override_(root_override) {}
LocationConfig::LocationConfig(const LocationConfig& other) : redirect_(other.redirect_), path_(other.path_), methods_(other.methods_), autoindex_(other.autoindex_), root_override_(other.root_override_), index_files_(other.index_files_) {}
LocationConfig& LocationConfig::operator=(const LocationConfig& other) {
	if (this != &other) {
		redirect_ = other.redirect_;
		path_ = other.path_;
		methods_ = other.methods_;
		autoindex_ = other.autoindex_;
		root_override_ = other.root_override_;
		index_files_ = other.index_files_;
	}
	return *this;
}
LocationConfig::~LocationConfig() {}

/** FUNCTIONS **/
std::pair<int, std::string> LocationConfig::getRedirect() const { return redirect_; }
std::string LocationConfig::getPath() const { return path_; }
std::vector<std::string> LocationConfig::getMethods() const { return methods_; }
bool LocationConfig::getAutoIndex() const { return autoindex_; }
std::string LocationConfig::getRootOverride() const { return root_override_; }

void LocationConfig::setRedirect(const std::pair<int, std::string> &redirect) { redirect_ = redirect; }
void LocationConfig::setPath(const std::string& path) { path_ = path; }
void LocationConfig::setMethods(const std::vector<std::string>& methods) { methods_ = methods; }
void LocationConfig::setAutoIndex(bool autoindex) { autoindex_ = autoindex; }
void LocationConfig::setRootOverride(const std::string& root_override) { root_override_ = root_override; }

void LocationConfig::printLocation() const 
{
	    std::cout << "redirect: " << redirect_.first << " " << redirect_.second << std::endl;
        std::cout << "path: " << path_ << std::endl;
		std::cout << "index files: ";
		for (std::size_t i = 0; i < index_files_.size(); ++i)
		{
            if (i) std::cout << ",";
            std::cout << index_files_[i];
        }
		std::cout << std::endl;
        std::cout << "methods: ";
        for (std::size_t i = 0; i < methods_.size(); ++i)
		{
            if (i) std::cout << ",";
            std::cout << methods_[i];
        }
        std::cout << std::endl;
        std::cout << "autoindex: " << (autoindex_ ? "on" : "off") << std::endl;
        std::cout << "root_override: " << root_override_ << std::endl;
}

const std::vector<std::string>& LocationConfig::getLocationIndexFiles() const { 
	return index_files_; 
}
void  LocationConfig::setLocationIndexFiles(const std::vector<std::string>&index_files) {
	index_files_ = index_files; 
}

void LocationConfig::setPythonCGIExecutable(const std::string& executable) {
	exec_cgi_ = executable;
}
std::string LocationConfig::getPythonCGIExecutable() const {
	return exec_cgi_;
}