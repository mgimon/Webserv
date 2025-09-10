#include <fstream>
#include <map>
#include <sstream>
#include <vector>

class ConfigParser{
  private:
    std::string filename;
  public:
    ConfigParser(const std::string& filename);
    void  parse(std::vector<ServerConfig>& servers);
};
