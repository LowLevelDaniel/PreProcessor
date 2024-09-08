#include "PreY"
#include <string.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <unordered_map>

#define SIMPLE_NEXT(file,in,eofhandle) \
  in = fgetc(file); \
  if (in == EOF) { eofhandle }
#define CONFIRM_IN(file,in,ein,handle,eofhandle) \
  in = fgetc(file); \
  if (in == EOF) { eofhandle } \
  else if (in != ein) { handle }
#define PARSING_SETUP(file) size_t __zcom_ftell = ftell(file);
#define PARSING_POSTFIX(file) return false; __zcom_exit_eof: return true;
#define PEBREAK(file,handle) fseek(file,__zcom_ftell,SEEK_SET); handle;

struct Defenition {
  std::string ident;
  std::string value;
};
struct FDefenition {
  std::string ident;
  std::vector<std::string> param;
  std::string value;
};

typedef std::vector<Defenition> defenitions_t;
typedef std::vector<FDefenition> fdefenitions_t;
typedef std::unordered_map<std::string,bool> parsed_files_t;

bool proccess_sub_file(
  FILE *file, FILE *fout, 
  defenitions_t& defenitions, fdefenitions_t& function_defenitions, parsed_files_t& parsed_files,
  const std::vector<char const*>& include_paths
);

#define PARSE_MACRO_IDENT(file,in,ident,eofhandle) \
  bool _function = false; \
  for (in = fgetc(file);;in = fgetc(file)) { \
    if (in == EOF) {eofhandle} \
    else if (in == ' ') { if (!ident.empty()) break; } \
    else if (in == '(') { _function = true; break; } \
    else if (in == '\n') { break; } \
    else { ident.push_back(in); } \
  }
#define PARSE_MACRO_PARAMETER(file,in,param,eofhandle) \
  std::string __zcom_tmp_param; __zcom_tmp_param.reserve(64); \
  for (in = fgetc(file);; in = fgetc(file)) { \
    if (in == EOF) { eofhandle;} \
    else if (in == ',') { \
      param.push_back(__zcom_tmp_param); __zcom_tmp_param.clear(); continue; \
    } else if (in == ')') { \
      param.push_back(__zcom_tmp_param); break; \
    } \
    __zcom_tmp_param.push_back(in); \
  }
#define PARSE_MACRO_VALUE(file,in,value,eofhandle) \
  for (in = fgetc(file);;in = fgetc(file)) { \
    if (in == EOF) {eofhandle} \
    else if (in == '\\') { \
      in = fgetc(file); \
      if (in == '\n') { fputc('\n',fout); continue; } \
      value.push_back('\\'); value.push_back(in); \
    } \
    else if (in == '\n') { break; } \
    else { value.push_back(in); } \
  }
// Macro Dereference
  bool confirm_macro(char in, FILE *fin, const std::string& macro) {
    PARSING_SETUP(fin);
    for (size_t i = 0; i < macro.size(); ++i) {
      if (macro[i] != in) { 
        PEBREAK(fin,return false;);  // not found
      }
      SIMPLE_NEXT(
        fin,in,
        PEBREAK(fin,return false;) // not found (because eof before macro end)
      )
    }
    fseek(fin,-1,SEEK_CUR);
    return true; // found
  }
  void handle_macro(const Defenition& def, FILE *fout) { fwrite(def.value.data(),1,def.value.size(),fout); }
  bool handle_fmacro(FDefenition& def, FILE *fin, FILE *fout) {
    PARSING_SETUP(fin); char in;
    // get user parameters
    CONFIRM_IN(fin,in,'(',PEBREAK(fin,return false;),PEBREAK(fin,return false;))
    std::vector<std::string> uparam; uparam.reserve(def.param.size());
    PARSE_MACRO_PARAMETER(fin,in,uparam,PEBREAK(fin,return false;))
    // parse the macro value and replace parameters
    bool argument_parsed = false;
    FILE *fmem = fmemopen(def.value.data(),def.value.size(),"r");
    if (fmem == NULL) {
      PRINT_ERROR("Internal Memory Error! Fatal Program Exiting!");
      exit(1);
    }
    for (char in = fgetc(fmem); in != EOF; in = fgetc(fmem)) {
      for (size_t j = 0; j < def.param.size(); ++j) {
        if (confirm_macro(in,fmem,def.param[j])) {
          fwrite(uparam[j].data(),1,uparam[j].size(),fout);
          argument_parsed = true;
          break;
        }
      }      
      if (!argument_parsed) fputc(in,fout);
      else argument_parsed = false;
    }
    return true;
  }
  bool check_macro(
    char in, FILE *file, FILE *fout,
    const defenitions_t& defenitions, fdefenitions_t& fdefenitions
  ) {
    for (size_t i = 0; i < fdefenitions.size(); ++i) {
      auto& macro = fdefenitions[i].ident;
      if (confirm_macro(in,file,macro)) { 
        if (handle_fmacro(fdefenitions[i],file,fout)) goto found; // fails if there is no closing brace or if a ( does not come after, in which case continue parsing
      }
    }
    for (size_t i = 0; i < defenitions.size(); ++i) {
      auto& macro = defenitions[i].ident;
      if (confirm_macro(in,file,macro)) { handle_macro(defenitions[i],fout); goto found; }
    }
    err:
      return false;
    found:
      return true;
  }
// Handle # Statements
  FILE *checkfile(const std::string& filepath, const std::vector<char const*>& include_paths) {
    FILE *file;
    if ((file = fopen(filepath.c_str(), "r")) != NULL) return file;
    if ((file = fopen(("/usr/include/" + filepath).c_str(), "r")) != NULL) return file;
    if ((file = fopen(("/usr/local/include/" + filepath).c_str(), "r")) != NULL) return file;
    for (const char* ipath : include_paths) {
      if ((file = fopen((ipath + filepath).c_str(), "r")) != NULL) return file;
    }
    return file;
  }
  bool remove_macro(
    char in, FILE *file,
    defenitions_t& defenitions, fdefenitions_t& fdefenitions
  ) {
    for (size_t i = 0; i < fdefenitions.size(); ++i) {
      auto& macro = fdefenitions[i].ident;
      if (confirm_macro(in,file,macro)) {
        fdefenitions.erase(fdefenitions.begin()+i);
        goto found;
      }
    }
    for (size_t i = 0; i < defenitions.size(); ++i) {
      auto& macro = defenitions[i].ident;
      if (confirm_macro(in,file,macro)) { 
        defenitions.erase(defenitions.begin()+i);
        goto found;
      }
    }
    return false;
    found: return true;
  }
  bool handle_macro_stat(
    FILE *file, FILE *fout, 
    defenitions_t& defenitions, fdefenitions_t& function_defenitions, parsed_files_t& parsed_files,
    const std::vector<char const*>& include_paths
  ) {
    PARSING_SETUP(file);
    char in;
    SIMPLE_NEXT(file,in,return false;)
    switch (in) {
      case 'd': { // define
        CONFIRM_IN(file,in,'e',goto err;,goto err;)
        CONFIRM_IN(file,in,'f',goto err;,goto err;)
        CONFIRM_IN(file,in,'i',goto err;,goto err;)
        CONFIRM_IN(file,in,'n',goto err;,goto err;)
        CONFIRM_IN(file,in,'e',goto err;,goto err;)
        CONFIRM_IN(file,in,' ',goto err;,goto err;)
        // get identifier
        std::string ident; ident.reserve(32);
        PARSE_MACRO_IDENT(file,in,ident,goto err;)
        if (_function) {
          FDefenition def; def.ident = ident;
          PARSE_MACRO_PARAMETER(file,in,def.param,goto err;)
          PARSE_MACRO_VALUE(file,in,def.value,goto err;)
          function_defenitions.push_back(def);
        } else {
          Defenition def; def.ident = ident;
          PARSE_MACRO_VALUE(file,in,def.value,goto err;)
          defenitions.push_back(def);
        }        
        goto success;
      } case 'u': { // undef
        CONFIRM_IN(file,in,'n',goto err;,goto err;)
        CONFIRM_IN(file,in,'d',goto err;,goto err;)
        CONFIRM_IN(file,in,'e',goto err;,goto err;)
        CONFIRM_IN(file,in,'f',goto err;,goto err;)
        CONFIRM_IN(file,in,' ',goto err;,goto err;)
        std::string ident; ident.reserve(32);
        PARSE_MACRO_IDENT(file,in,ident,goto err;)
        PRINT_DEBUG("Removing Macro %s", ident.c_str());
        FILE *fident = fmemopen(ident.data(),ident.size(),"r");
        if (fident == NULL) {
          PRINT_DEBUG("Internal Memory Error (FATAL EXITING)"); 
          exit(1);
        }
        if (remove_macro(fgetc(fident),fident,defenitions,function_defenitions)) {
          PRINT_DEBUG("Macro Removed");
        } else {
          PRINT_DEBUG("Macro Not Removed");
        }
        goto success;      
      } case 'i': // include, if, ifdef
        SIMPLE_NEXT(file,in,goto err;)
        if (in == 'n') {
          CONFIRM_IN(file,in,'c',goto err;,goto err;)
          CONFIRM_IN(file,in,'l',goto err;,goto err;)
          CONFIRM_IN(file,in,'u',goto err;,goto err;)
          CONFIRM_IN(file,in,'d',goto err;,goto err;)
          CONFIRM_IN(file,in,'e',goto err;,goto err;)
          SIMPLE_NEXT(file,in,goto err;)
          std::string file_name; file_name.reserve(16);
          while (in == ' ') {
            SIMPLE_NEXT(file,in,goto err;)
            if (in == '<') {
              goto parse_angler;
            }
          } 
          if (in == '<') {
            parse_angler:
            for (char in = fgetc(file); in != '>'; in = fgetc(file)) {
              if (in == EOF) { PRINT_ERROR("Couldn't parse end anglar bracker '>' in #include < statement"); exit(1); }
              file_name.push_back(in);
            }
          }
          FILE *subfile = checkfile(file_name,include_paths);
          if (subfile == NULL) {
            PRINT_ERROR("Couldn't Open File - %s (FATAL EXITING)",file_name.c_str());
            exit(1);
          }
          fputc('#',fout);
          fwrite(file_name.data(),1,file_name.size(),fout);
          fputc('\n',fout);
          proccess_sub_file(subfile,fout,defenitions,function_defenitions,parsed_files,include_paths);
          fwrite("\n#@MAIN",1,7,fout);
          goto __FindAWayToGetRidOfThis;
        } else if (in == 'f') {
          PRINT_ERROR("#if statements not implemented yet");
          exit(1);
        }
        goto err;
      case 'p': { // pragma
        PRINT_ERROR("#pragma statements not implemented yet");
        exit(1);
        goto err;
      }
    }
    err:
      PEBREAK(file,);
      return false;
    success:
      fputc('\n',fout);
    __FindAWayToGetRidOfThis:
      return true;
  }
// Root Level Processing
bool proccess_sub_file(
  FILE *file, FILE *fout, 
  defenitions_t& defenitions, fdefenitions_t& function_defenitions, parsed_files_t& parsed_files,
  const std::vector<char const*>& include_paths
) {
  for (char in = fgetc(file); in != EOF; in = fgetc(file)) {
    if (in == '/') { PARSING_SETUP(file); if (parse_comment(file)) { continue; } PEBREAK(file,goto insert;) }
    else if (in == '#') { if (handle_macro_stat(file,fout,defenitions,function_defenitions,parsed_files,include_paths)) { continue; } goto insert; }
    else if (check_macro(in,file,fout,defenitions,function_defenitions)) { continue; } // macro found and written
    else { insert: fputc(in,fout); }
  }
  return false;
}

FILE *proccess_file(FILE *file, const char *ident, const std::vector<char const*>& include_paths) {
  FILE *fout = fopen(ident, "w");
  if (fout == NULL) { PRINT_ERROR("Couldn't Write to file, %s",ident); return NULL; }
  defenitions_t defenitions;
  fdefenitions_t function_defenitions;
  parsed_files_t parsed_files; // true if pragma once
  proccess_sub_file(file,fout,defenitions,function_defenitions,parsed_files,include_paths);
  for (auto def : defenitions) {
    PRINT_DEBUG(" defenition - %s with value %s",def.ident.c_str(),def.value.c_str());
  }
  for (auto def : function_defenitions) {
    std::string parameters;
    for (auto param : def.param) {
      parameters += param + ", ";
    }
    PRINT_DEBUG(" function defenition - %s(%s) with value %s",def.ident.c_str(),parameters.c_str(),def.value.c_str());
  }
  for (auto file : parsed_files) {
    PRINT_DEBUG(" parsed file - %s", file.first.c_str());
  }
  fclose(fout);
  FILE *fin = fopen(ident,"r");
  if (fin == NULL) { PRINT_ERROR("Couldn't Open Temporary File, %s",ident); return NULL; }
  return fin;
}

// Other
  bool parse_comment(FILE *file) {
    char in = fgetc(file);
    if (in == '/') {
      for (; in != '\n'; in = fgetc(file)) {
        if (in == EOF) return false;
      }
      return true;
    } else if (in == '*') {
      bool last_character_star = false; 
      for (char in = fgetc(file); !G_exit; in = fgetc(file)) {
        switch (in) {
          case EOF: return false;
          case '*': last_character_star = true; break;
          case '/': if (last_character_star) return true;
          default: last_character_star = false; // to ensure that */ is conncurrent
        } 
      }
    } else {
      return false; // not a comment
    }
    return true;
  }
// END
