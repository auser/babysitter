/**
 * Config Parser
 * Handles dealing with configuration files
 * 
 * Syntax
 * [app namespace].[action_name]: [action]
 * For instance:
 * # Comment
 * # The defaults are run internal to babysitter, 
 * # but can be overridden with shell scripts
 * # 
 * # The available actions are:
 * # default.bundle: "TODO"
 * # default.mount: 
 * # default.start: 
 * # default.stop:
 * # default.umount:
 * # default.cleanup:
 *
 * To override these values, provide a custom
 * namespace before the action, such as:
 * rack.bundle: "Some executable action"
 * rack.mount:
 * 
**/
/** includes **/
#include <stdio.h>
#include <string.h>
#include <string>
#include <stdlib.h>
#include <set>
#include <regex.h>
#include <map>

// Define
#define SPLIT_CHAR ':'
#define BUFFER 1024

typedef std::map<std::string, std::string> string_dict;

class ConfigDefinition {
private:
  std::string                         m_name; // Name of the configuration definition. i.e. rack
  std::map<std::string, std::string>  m_dict; // The map of the configuration definition. i.e. bundle : "action"
  
public:
  ConfigDefinition() : m_name("") {}
  ~ConfigDefinition() {}
  
  std::string name() {return m_name;}
  string_dict dict() {return m_dict;}
  
  void set_name(std::string n) {m_name = n;}
};

typedef ConfigDefinition config_definition;

class ConfigParser {
private:
  std::map<std::string, config_definition> m_definitions;
  
public:
  ConfigParser() {}
  ~ConfigParser() {}
  
  std::map<std::string, config_definition> definitions() {return m_definitions;}
  
  int parse_file(std::string filename);
  
private:
  int parse_line(const char *line, int len);
  ConfigDefinition parse_config_line(const char *line, int len);
};