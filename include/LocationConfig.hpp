#ifndef LOCATIONCONFIG_HPP
#define LOCATIONCONFIG_HPP

#include <string>
#include <vector>
#include <iostream>
#include <map>

class LocationConfig {
	private:
		std::pair<int, std::string> redirect_; // vacio si no hay redirect
		std::string path_;                     // Ej: "/images/"
		std::vector<std::string> methods_;     // Ej: {"GET", "POST"}
		bool autoindex_;                       // true = on, false = off
		std::string root_override_;            // Si un location tiene su propio root
		std::vector<std::string>	index_files_;
		std::string exec_cgi_;

	public:
		LocationConfig();
		LocationConfig(const std::string& path,
						const std::vector<std::string>& methods,
						bool autoindex,
						const std::string& root_override = "");
		LocationConfig(const LocationConfig& other);
		LocationConfig& operator=(const LocationConfig& other);
		~LocationConfig();

		std::pair<int, std::string> getRedirect() const;
		std::string getPath() const;
		std::vector<std::string> getMethods() const;
		bool getAutoIndex() const;
		std::string getRootOverride() const;
		std::vector<std::string> getIndexs() const;
		const std::map<std::string, std::string>& getCgiMap() const;
		std::string getCgiForExtension(const std::string &ext) const;

		void setRedirect(const std::pair<int, std::string> &redirect);
		void setPath(const std::string& path);
		void setMethods(const std::vector<std::string>& methods);
		void setAutoIndex(bool autoindex);
		void setRootOverride(const std::string& root_override);
		void setCgi(const std::string &ext, const std::string &executable);

		void printLocation() const;
		const std::vector<std::string>& getLocationIndexFiles() const;
		void setLocationIndexFiles(const std::vector<std::string>&index_files);
		void setPythonCGIExecutable(const std::string& executable);
		std::string getPythonCGIExecutable() const;
};

#endif
