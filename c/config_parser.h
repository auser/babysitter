/**
 * Config Parser
 * Handles dealing with configuration files
 * 
 * Syntax
 * [app name].[action_name]: [action]
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
 * name before the action, such as:
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

class ConfigDefinition {
private:
  std::string           m_name; // The name of the type of app
  std::string           m_action;    // The action (i.e. bundle/start/stop, etc)
  std::string           m_before;    // The before hook
  std::string           m_after;     // The after hook
  std::string           m_command;   // The command
  
public:
  ConfigDefinition() : m_name(""),m_action(""),m_before(""),m_after(""),m_command("") {}
  ~ConfigDefinition() {}
  
  std::string name()      {return m_name;}
  std::string action()    {return m_action;}
  std::string before()    {return m_before;}
  std::string after()     {return m_after;}
  std::string command()   {return m_command;}
  
  void set_name(std::string n) {m_name = n;}
  void set_action(std::string n) {m_action = n;}
  void set_before(std::string n) {m_before = n;}
  void set_after(std::string n) {m_after = n;}
  void set_command(std::string n) {m_command = n;}  
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
  int parse_line(const char *line, int len, int linenum);
  ConfigDefinition parse_config_line(const char *line, int len, int linenum);
};