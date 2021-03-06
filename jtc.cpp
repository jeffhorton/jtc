#include <iostream>
#include <fstream>
#include <sstream>
#include <deque>
#include <set>
#include <climits>      // LONG_MAX
#include "lib/getoptions.hpp"
#include "lib/Json.hpp"
#include "lib/shell.hpp"
#include "lib/dbg.hpp"

using namespace std;


#define VERSION "1.67"


// option definitions
#define OPT_RDT -
#define OPT_ALL a
#define OPT_CMP c
#define OPT_DBG d
#define OPT_EXE e
#define OPT_FRC f
#define OPT_GDE g
#define OPT_INS i
#define OPT_JAL J
#define OPT_JSN j
#define OPT_LBL l
#define OPT_MDF m
#define OPT_SEQ n
#define OPT_PRG p
#define OPT_QUT q
#define OPT_RAW r
#define OPT_SWP s
#define OPT_TMP T
#define OPT_IND t
#define OPT_UPD u
#define OPT_WLK w
#define OPT_CMN x
#define OPT_PRT y
#define OPT_SZE z

// facilitate option materialization
#define STR(X) XSTR(X)
#define XSTR(X) #X
#define CHR(X) XCHR(X)
#define XCHR(X) *#X

#define INTRP_VAL "{}"                                          // interpolate value
#define CMP_BASE "json_1"
#define CMP_COMP "json_2"
#define SIZE_PFX "size: "
#define PATH_JSN "$PATH"                                        // token for JSON path interp.
#define PATH_STR "$path"                                        // token for stringified path
#define PATH_SPR "_"                                            // stringified path separtor

#include "lib/jtc_guide.hpp"

// various return codes
#define RETURN_CODES \
        RC_OK, \
        RC_WLK_MISS, \
        RC_WP_INV, \
        RC_SC_MISS, \
        RC_CMP_NEQ, \
        RC_END
ENUM(ReturnCodes, RETURN_CODES)

// return code exception offsets
#define OFF_GETOPT RC_END                                       // offset for Getopt exceptions
#define OFF_JSON (OFF_GETOPT + Getopt::end_of_throw)            // offset for Json exceptions
#define OFF_REGEX (OFF_JSON + Jnode::end_of_throw)              // offset for Regex exceptions

// simple macro for to expose class aliases
// usage: REVEAL(jtc, opt, json)
#define __REVEAL_TKN1__(X,Y) X ## Y
#define __REVEAL_TKN2__(X,Y) __REVEAL_TKN1__(X, Y)
#define __REFX__(A) auto & A = __REVEAL_TKN2__(__reveal_class__,__LINE__).A();
#define REVEAL(X, ARGS...) \
        auto & __REVEAL_TKN2__(__reveal_class__, __LINE__) = X; \
        MACRO_TO_ARGS(__REFX__, ARGS)

#define DBG_WIDTH 67                                            // max print len upon parser's dbg
#define KEY first                                               // semantic for map's pair
#define VALUE second                                            // instead of first/second

typedef vector<string> v_string;





class CommonResource {
 // the class facilitates -J option:
 // caters original opt (which is copied to each Jtc instance), input string
 // and json for jsonization of all processed/waled jsons
 public:
    Getopt &            opt(void) { return opt_; };
    size_t              opt_e_found(void) { return opt_e_found_; }  // used for recompile once
    char                opt_ui(void){  return opt_ui_; };           // -i or -u for recompile
    const string &      istr(void){  return istr_; };
    Json &              json(void) { return gj_; };

    void                parse_opt(int argc, char *argv[]);
    const string &      read_inputs(void);
    void                jsonize(Json jout);

    DEBUGGABLE()

 private:
    string              istr_;                                  // user intput string (json)
    Getopt              opt_;
    size_t              opt_e_found_{0};                        // used for recompile once -e found
    char                opt_ui_{'\0'};                          // either -i or -u for recompile
    bool                ji_{false};                             // '-j' imposed?
    Json                gj_{ ARY{} };                           // global json

    bool                is_recompile_required_(int argc, char *argv[]);
    void                recompile_args_(v_string &args, v_string &new_args);
    void                parse_rebuilt_(v_string & new_args);
    void                convert_xyw_(void);
};





class Jtc {
    struct StringOvr {
     // overrides string holder only if argument is non-empty
                            StringOvr(void) = default;
                            StringOvr(string s): str_{s} {}     // copy elision
        StringOvr &         operator=(const string &s)
                             { str_ = s.empty()? str_: s; return *this; }
                            operator string & (void) { return str_; }
     private:
        string              str_;
    };

    typedef vector<Json::iterator> walk_vec;
    typedef deque<Json::iterator> walk_deq;
    typedef void (Jtc::*mptr)(Json::iterator &wi, size_t group);
    typedef map<size_t, Json> map_json;
    typedef map<size_t, Json::iterator> map_jit;
    typedef map<const Json::iterator*, Json::map_jn> map_ns;

    #define WALKREQ \
                exact,  /* walk requirement: number of given -w must match exactly */\
                minimum /* number of given -w must be at least given minimum */
    ENUM(WalkReq, WALKREQ)

    #define MERGEOBJ \
                preserve,   /* clashing labels of merged object not overwritten */\
                overwrite   /* clashing labels of merged objects overwritten */
    ENUM(MergeObj, MERGEOBJ)

 public:

                        Jtc(void) = delete;
                        Jtc(CommonResource & cr):
                         cr_{cr}, opt_{cr.opt()} {
                         ecli_ = opt_[CHR(OPT_EXE)].hits() > 0;     // flag used by -i/-u options
                         merge_ = opt_[CHR(OPT_MDF)].hits() > 0;    // flag used by -i/-u options
                         json_.tab(abs(opt_[CHR(OPT_IND)]))
                              .raw(opt_[CHR(OPT_RAW)])
                              .quote_solidus(opt_[CHR(OPT_QUT)].hits() % 2 == 1);
                        }


    // expose private objects
    auto &              opt(void) { return opt_; }
    auto &              json(void) { return json_; }
    auto &              jout(void) { return jout_; }

    // user methods
    void                parsejson(string::const_iterator & jsp);
    int                 write_json(Json & jsn, bool jsnize = true);
    int                 demux_opt(void);
    ReturnCodes         compare_jsons(void);
    void                compare_by_iterator(Json::iterator &it, size_t group);
    void                insert_json(void);
    void                insert_by_iterator(Json::iterator &it, size_t group);
    void                purge_json(void);
    void                update_json(void);
    void                update_by_iterator(Json::iterator &it, size_t group);
    int                 swap_json(void);
    int                 walk_json(void);
    void                output_by_iterator(Json::iterator &wi, size_t);

 private:
    void                jsonized_output_(Json::iterator &, size_t group, const Json &jref);
    void                jsonized_output_ary_(Json::iterator &, size_t group, const Json &jref);
    void                jsonized_output_obj_(Json::iterator &, size_t group, const Json &jref);
    void                console_output_(Json::iterator &, size_t group, const Json &jref);
    void                check_walk_requirements_(unsigned, WalkReq req=minimum);
    void                crop_out_(void);
    bool                remove_others_(set<const Jnode*> &ws, Jnode &jn);
    walk_vec            collect_walks_(const string &walk_path);
    void                compare_jsons_(const Jnode &, set<const Jnode*> &,
                                       const Jnode &, set<const Jnode*> &);
    void                merge_jsons_(Json::iterator &dst, Json::iterator src);
    void                merge_into_array_(Jnode &dst, const Jnode &src, MergeObj mode);
    void                merge_into_object_(Jnode &dst, const Jnode &src, MergeObj mode);
    void                update_jsons_(Json::iterator &dst, Json::iterator src);
    bool                processed_by_cli_(Json::iterator &it);
    bool                execute_cli_(Json &update, Json::iterator &jit, const Json::map_jn &ns);
    string              reconcile_ui_(Json::iterator &jit, const Json::map_jn &ns);
    void                parse_params_(char option);
    void                extend_itr_(    Json::iterator);
    void                extend_itr_(map_json &, const Json::map_jn &, Json::iterator);
    void                build_path_(Jnode &jpath, Json::iterator &jit);
    Json                interpolate_(StringOvr, const Json::map_jn &, Json::iterator &,
                                     Json::ParseTrailing = Json::relaxed_trailing,
                                     bool parse = true);
    string              interpolate_tmp_(const string &, const Json::map_jn &);
    void                walk_interleaved_(void);
    void                process_walk_iterators_(deque<walk_deq> &walk_iterators);
    void                process_offsets_(deque<walk_deq> &, vector<vector<long>> &,
                                         size_t, vector<size_t> &);
    size_t              build_front_grid_(vector<vector<long>> &, const deque<walk_deq> &);
    void                location_(string::const_iterator & start);

    CommonResource &    cr_;
    Getopt              opt_;
    Json                json_;                                  // jtc input JSON
    map_json            jexc_;                                  // json for -ei or -eu
    Json                jout_;                                  // json output (-j)
    map_json            jsrc_;                                  // JSON in parameters of -i, -u
    map_jit             isrc_;                                  // walk iterators in params -i, -u
    map_ns              wns_;                                   // namespaces for walked (-w) paths
    set<string>         c2a_;                                   // converted to arrays
    bool                is_multi_walk_{false};                  // multiple -w or single iterable?
    bool                convert_req_{false};                    // used in output_by_iterator
    bool                ecli_{false};                           // -e status for insert/update
    bool                merge_{false};                          // -m status for insert/update
    bool                lbl_update_{false};                     // label update operation detected
    size_t              last_group_{0};                         // used in output_by_iterator
    size_t              key_{0};                                // for -i, -u options processing
    mptr                subscriber_;                            // method ptr for output processor
    Shell               sh_;
    ReturnCodes         cmp_{RC_OK};                            // for -c / compare return result

 public:

    DEBUGGABLE(json_, jout_, sh_)
};

#undef WALKREQ
#undef MERGEOBJ


string quote_str(const string &src);





int main(int argc, char *argv[]){

 CommonResource cr;
 REVEAL(cr, opt)

 opt.prolog("\nJSON test console\nVersion " VERSION \
            ", developed by Dmitry Lyssenko (ldn.softdev@gmail.com)\n");
 opt[CHR(OPT_ALL)].desc("process all inputs (by default only one JSON processed; -"
                        STR(OPT_FRC) " ignored)");
 opt[CHR(OPT_DBG)].desc("turn on debugs (multiple calls increase verbosity)");
 opt[CHR(OPT_EXE)].desc("make option parameters for -" STR(OPT_INS) ", -" STR(OPT_UPD)
                        " undergo shell evaluation; see -" STR(OPT_GDE) " for more info");
 opt[CHR(OPT_FRC)].desc("apply changes into the file (instead of printing resulting JSON"
                        " to stdout)");
 opt[CHR(OPT_GDE)].desc("explain walk path syntax, usage notes and examples");
 opt[CHR(OPT_JAL)].desc("wrap all processed JSONs into array (option -" STR(OPT_ALL) " assumed)");
 opt[CHR(OPT_JSN)].desc("wrap walked JSON elements into JSON array (-" STR(OPT_JSN) STR(OPT_JSN)
                        " wrap into JSON object)");
 opt[CHR(OPT_LBL)].desc("print labels (if present) for walked JSON; together with -"
                        STR(OPT_JSN) "  wrap into objects");
 opt[CHR(OPT_MDF)].desc("modifier: toggle merging for options -" STR(OPT_INS) ", -" STR(OPT_UPD)
                        "; see with -" STR(OPT_GDE) " for more info");
 opt[CHR(OPT_SEQ)].desc("do not print/process walks interleaved (i.e. print/process all walks "
                        "sequentially)");
 opt[CHR(OPT_PRG)].desc("purge all walked JSON elements (-" STR(OPT_PRG) STR(OPT_PRG)
                        ": purge all elements except walked)");
 opt[CHR(OPT_QUT)].desc("enforce strict quoted solidus parsing "
                        "(-qq: unquote isolated JSON string)");
 opt[CHR(OPT_RAW)].desc("print JSON in a raw (compact, one-line) format"
                        " (-rr stringify resulting JSON)");
 opt[CHR(OPT_SWP)].desc("swap around two JSON elements pointed by walks (two -" STR(OPT_WLK)
                        " must be given)");
 opt[CHR(OPT_SZE)].desc("print size (number of nodes in JSON) at the end of output (-"
                        STR(OPT_SZE) STR(OPT_SZE) " prints size only)");
 opt[CHR(OPT_CMP)].desc("compare JSONs: display delta between given JSONs").name("f|j|w");
 opt[CHR(OPT_INS)].desc("insert/merge JSON, or from file, or pointed by a walk-path, see with -"
                        STR(OPT_GDE) " for more").name("f|j|w");
 opt[CHR(OPT_TMP)].desc("template to interpolate and apply upon -" STR(OPT_INS) ", -" STR(OPT_UPD)
                        " and JSON walk operations").name("template");
 opt[CHR(OPT_IND)].desc("indent for pretty printing").bind("3").name("indent");
 opt[CHR(OPT_UPD)].desc("update/replace JSON, of from file, or pointed by a walk-path, see with -"
                        STR(OPT_GDE) " for more").name("f|j|w");
 opt[CHR(OPT_WLK)].desc("a standalone walk path (multiple may be given); see with -"
                        STR(OPT_GDE) " for more").name("walkpath");
 opt[CHR(OPT_CMN)].desc("a common part of a path, prepended to every followed -" STR(OPT_PRT)
                        " option").name("common_wp");
 opt[CHR(OPT_PRT)].desc("an individual partial path, prepended by preceding -" STR(OPT_CMN)
                        " option").name("partial_wp");
 opt[0].desc("file to read json from").name("json_file").bind("<stdin>");

 opt.epilog(R"(
this tool provides ability to:
 - parse, validate and display JSON (in a compact and pretty formats)
 - walk about input JSON using various subscripting and search criteria (see with -)" STR(OPT_GDE)
 R"( fore more)
 - manipulate JSON via purge/insert/copy/merge/update/replace/move/swap/iterpolate operations
 - compare JSONs (print diffs)

for walk-path explanation, usage notes and examples run with -)" STR(OPT_GDE) R"(
for a complete user guide visit https://github.com/ldn-softdev/jtc/blob/master/User%20Guide.md
)");


 // parse options
 cr.parse_opt(argc, argv);

 DEBUGGABLE()
 DBG().use_ostream(cerr)
      .level(opt[CHR(OPT_DBG)]);

 // read json
 int main_rc = RC_OK;                                           // main's return code
 string::const_iterator jsp = cr.read_inputs().begin();         // global parse pointer

 // execute as per read options
 try {
  do {
   Jtc jtc(cr);
   jtc.parsejson(jsp);
   int rc = jtc.demux_opt();
   main_rc = rc != RC_OK? rc: main_rc;
  } while(*jsp != '\0' and opt[CHR(OPT_ALL)].hits() > 0);
 }
 catch(Jnode::stdException & e) {
  DBG(1) DOUT() << "exception raised by: " << e.where() << endl;
  cerr << opt.prog_name() << " jnode exception: " << e.what() << endl;
  main_rc = e.code() + OFF_JSON;
 }
 catch(Json::stdException & e) {
  DBG(1) DOUT() << "exception raised by: " << e.where() << endl;
  cerr << opt.prog_name() << " json exception: " << e.what() << endl;
  main_rc = e.code() + OFF_JSON;
 }
 catch(std::regex_error & e) {
  cerr << "regexp exception: " << e.what() << endl;
  main_rc = e.code() + OFF_REGEX;
 }

 if(cr.json().empty()) return main_rc;

 Jtc jtc(cr);
 for(const char *o = STR(OPT_JAL)STR(OPT_JSN)STR(OPT_QUT)STR(OPT_RAW); *o != '\0'; ++o)
  jtc.opt()[*o].reset();                                       // above options to be ignored
 jtc.write_json(cr.json());
 return main_rc;
}



//
// CR methods definitions
//
void CommonResource::parse_opt(int argc, char *argv[]) {
 // parse options, if option -e detected, rebuild -u's arguments and parse with rebuilt args
 v_string args{argv, argv + argc};

 if(is_recompile_required_(argc, argv)) {                       // re-compiling required?
  v_string new_args;                                            // newly rebuilt args go here
  recompile_args_(args, new_args);                              // rebuild -u/i's arguments as one
  parse_rebuilt_(new_args);
 }
 else                                                           // parse normally
  try { opt_.parse(argc, argv); }
  catch(Getopt::stdException &e)
   { opt_.usage(); exit(e.code() + OFF_GETOPT); }

 convert_xyw_();                                                // -w = -x + -y...
 if(opt_[CHR(OPT_GDE)].hits() > 0) exit(print_guide());         // -g, print guide
 if(opt_[CHR(OPT_JAL)].hits() > 0) {                            // -J,
  opt_[CHR(OPT_ALL)].hit();                                     // ensure -a hit unconditionally
  if(opt_[CHR(OPT_JSN)].hits() == 0)                            // if no -j given
   { opt_[CHR(OPT_JSN)].hit(); ji_ = true; }                    // impose one
 }
 if(opt_[CHR(OPT_JSN)].hits() > 0)                              // if -j given,
  if(opt_[CHR(OPT_WLK)].hits() == 0) opt_[CHR(OPT_WLK)] = "";   // ensure -w given too
 if(opt_[CHR(OPT_ALL)].hits() > 0 and opt_[CHR(OPT_FRC)].hits() > 0) {
  DBG(0) DOUT() << "ignoring option -" STR(OPT_FRC) " b/c of multi-input processing" << endl;
  opt_[CHR(OPT_FRC)].reset();                                   // -a (-J), ensure -f ignored
 }
}



const string & CommonResource::read_inputs(void) {
 bool read_from_cin{opt_[0].hits()==0 or opt_[CHR(OPT_RDT)].hits()>0};// no file, or forced via '-'
 DBG(0)
  DOUT() << "reading json from " << (read_from_cin? "<stdin>": opt_[0].c_str()) << endl;

 istr_ = string{istream_iterator<char>(read_from_cin?
                                        cin >> noskipws:
                                        ifstream{opt_[0].c_str(), ifstream::in} >> noskipws),
                istream_iterator<char>{}};
 return istr_;
}



bool CommonResource::is_recompile_required_(int argc, char *argv[]) {
 // check if option -e is present in the arguments (then re-parsing is required)
 opt_.suppress_opterr(true);
 try { opt_.parse(argc, argv); } catch(Getopt::stdException &e) { }
 bool rr = opt_[CHR(OPT_EXE)];
 opt_.reset().suppress_opterr(false);
 return rr;
}



void CommonResource::jsonize(Json jout) {
 // put all walked and non-walked json results into a global json
 if(ji_ and jout.is_iterable()) {                               // -j was imposed
  for(auto &jn: jout) gj_.push_back(move(jn));                  // therefore push one by one
  return;
 }
                                                                // no -j were imposed:
 gj_.push_back(move(jout));                                     // move jout as it is
}



void CommonResource::recompile_args_(v_string & args, v_string &new_args) {
 // recompile argv minding -u/i's arguments, put re-parsed args into new_args
 bool semicolon_found = false;

 for(auto &arg: args) {                                         // go through all args
  if(semicolon_found)                                           // -i/u already found and processed
   { new_args.push_back(arg); continue; }                       // push arg w/o any processing

  if(opt_e_found_ > 0) {                                        // facing -i/u; ';' not found yet,
   if(arg.back() == ';')                                        // ';' found
    { semicolon_found = true; arg.pop_back(); }                 // trim trailing ';'
   if(not arg.empty()) {
    if(opt_e_found_++ == 1) new_args.back() += arg;             // first argument
    else new_args.push_back(string("-") + opt_ui_ + arg);       // any subsequent arg
   }
   continue;
  }

  if(arg.front() == '-')                                        // opt, see if opt -i/u is present
   for(const char &chr: arg) {
    if(&chr == &arg[0]) continue;                               // skip first char '-'
    if(not opt_.defined(chr)) break;                            // undefined option, process arg
    if(chr == CHR(OPT_UPD) or chr == CHR(OPT_INS)) {            // opt -i/u found, indicate&record
     opt_e_found_ = 1;
     opt_ui_ = chr;
     if(arg.back() == ';')                                      // ';' found
      { semicolon_found = true; arg.pop_back(); ++opt_e_found_; }
     else
      if(&arg.back() != &chr)                                   // arg is attached to the option
       ++opt_e_found_;
     break;
    }
   }
  new_args.push_back(arg);
 }

 if(opt_e_found_ > 0 and not semicolon_found) {
  cerr << "fail: don't see parameter termination of '-" << opt_ui_ << "' - `\\;'" << endl;
  exit(RC_SC_MISS);
 }
}



void CommonResource::parse_rebuilt_(v_string & new_args) {
 // parse rebuilt arguments
 char *nargv[new_args.size()];                                  // here, build a new argv
 for(size_t i = 0; i < new_args.size(); ++i) {
  nargv[i] = new char[new_args[i].size()+1];
  stpcpy(nargv[i], new_args[i].c_str());
 }

 try { opt_.reset().parse(new_args.size(), nargv); }            // re-parse newly rebuilt args
 catch(Getopt::stdException & e)
  { opt_.usage(); exit(e.code() + OFF_GETOPT); }

 for(size_t i = 0; i < new_args.size(); ++i)                    // clean up nargv now
  delete [] nargv[i];
}



void CommonResource::convert_xyw_(void) {
 // convert -x, -y options into -w
 string last_x, last_y;
 v_string new_w;                                                // record new -w options here

 for(auto &option: opt_.order()) {                              // go by options order
  if(option.id() == CHR(OPT_CMN)) {                             // option -x, process it
   if(not last_x.empty() and last_y.empty())                    // it's like: -x... -x...
    new_w.push_back(move(last_x));                              // standalone -x is converted to -w
   last_x = move(option.str());
   last_y.clear();
   continue;
  }
  if(option.id() == CHR(OPT_PRT)) {                             // option -y
   last_y = move(option.str());
   new_w.push_back(last_x + last_y);
  }
 }
 if(not last_x.empty() and last_y.empty())                      // option -x... is given alone
  new_w.push_back(move(last_x));

 for(auto &opt_w: new_w)                                        // move all new '-w' to opt
  opt_[CHR(OPT_WLK)] = opt_w;
}



//
// Jtc methods definitions
//
void Jtc::parsejson(string::const_iterator & jsp) {
 // parse read json text via string iterator
 string::const_iterator jbegin = jsp;                           // for debug / location_ only
 try { json_.parse(jsp); }
 catch(Json::stdException & e) {
  if(e.code() >= Jnode::start_of_json_parsing_exceptions and
     e.code() <= Jnode::end_of_json_parsing_exceptions)
   DBG(0) location_(jbegin);
  throw e;
 }

 if(json_.is_atomic() and opt_[CHR(OPT_WLK)].hits() > 0) {
  DBG(1) DOUT() << "read json is an atomic value, ignoring all possible walk-paths" << endl;
  opt_[CHR(OPT_WLK)].reset();
 }
}



int Jtc::write_json(Json & json, bool jsonize) {
 // write whole json to output (demultiplexing file and stdout), featuring:
 // inquoting/unquoting json string, putting array into json (-j), printing size to stdout
 if(opt_[CHR(OPT_SZE)].hits() > 1)                              // -zz
  { cout << json.size() << endl; return RC_OK; }

 bool write_to_file{opt_[0].hits() > 0 and opt_[CHR(OPT_FRC)].hits() > 0};  // [0] and -f given
 bool unquote{opt_[CHR(OPT_QUT)].hits() >= 2};                  // -qq given, unquote
 bool inquote{opt_[CHR(OPT_RAW)].hits() >= 2};                  // -rr given, inquote

 if(jsonize and opt_[CHR(OPT_JSN)].hits() == 1)                 // -j given, force jsonizing
  json = ARY{ move(json) };

 if(not unquote and inquote)
  json.root() = json.inquote_str(json.to_string(Jnode::Raw));
 DBG(0)
  DOUT() << "outputting json to " << (write_to_file?
                                       opt_[0].c_str():
                                       opt_[CHR(OPT_JSN)]? "<JSON>": "<stdout>") << endl;
 if(opt_[CHR(OPT_JAL)].hits() > 0)                              // -J, jsonize to global
  { cr_.jsonize(move(json)); return RC_OK; }

 ofstream fout{write_to_file? opt_[0].c_str(): nullptr};
 ostream & xout = write_to_file? fout: cout;

 if(unquote and json.is_string()) xout << json.unquote_str(json.str()) << endl;
 else xout << json << endl;

 if(opt_[CHR(OPT_SZE)])
  cout << SIZE_PFX << json.size() << endl;

 return RC_OK;
}



int Jtc::demux_opt(void) {
 // demultiplex functional options, execute once
 for(char opt: STR(OPT_CMP)STR(OPT_INS)STR(OPT_UPD)STR(OPT_SWP)STR(OPT_PRG)STR(OPT_WLK)) {
  if(opt == '\0' or opt_[opt].hits() == 0) continue;
  DBG(1) DOUT() << "option: '-" << opt << "', hits: " << opt_[opt].hits() << endl;

  switch(opt) {
   case CHR(OPT_CMP): return compare_jsons();                   // will print result
   case CHR(OPT_WLK): return walk_json();                       // will print result
   case CHR(OPT_INS): insert_json(); break;
   case CHR(OPT_UPD): update_json(); break;
   case CHR(OPT_PRG): purge_json(); break;
   case CHR(OPT_SWP): { auto rc = swap_json(); if(rc != RC_OK) return rc; break; }
   default: continue;
  }
  break;
 }

 // this write will be used only by -i, -u, -p, -s
 return write_json(json_);
}



ReturnCodes Jtc::compare_jsons() {
 // plug-in compare_by_iterator and let process walks
 bool is_json_atomic = json_.is_atomic();
 if(opt_[CHR(OPT_WLK)].hits() == 0)                             // no -w?
  opt_[CHR(OPT_WLK)] = is_json_atomic? "[0]": "[^0]";           // then walk from root
 parse_params_(CHR(OPT_CMP));                                   // collect all sources

 if(is_json_atomic) json_ = ARY{ move(json_) };
 subscriber_ = &Jtc::compare_by_iterator;
 walk_interleaved_();

 if(opt_[CHR(OPT_JSN)].hits() > 0)
  write_json(jout_);
 return cmp_;
}



void Jtc::compare_by_iterator(Json::iterator &it, size_t group) {
 // compare two JSONs: one pointed by iterator (it), another taken from params
 if(key_ >= jsrc_.size() + isrc_.size()) return;                // nothing left to process

 vector<Json> jv{2};                                            // 2 JSONs to compare
 for(auto &j: jv) DBG().severity(j);

 jv.front()[CMP_BASE] = *it;                                    // 1st comes form walk_interleaved
 if(jsrc_.empty()) jv.back()[CMP_COMP] = *isrc_[key_];          // 2nd comes from -c <walk-path>
 else jv.back()[CMP_COMP] = jsrc_[key_];                        // or from -c <JSON>

 vector<set<const Jnode*>> node_set{2};                         // preserved different node ptrs
 compare_jsons_(jv.front()[CMP_BASE], node_set.front(),
                jv.back()[CMP_COMP], node_set.back());

 v_string lbl{ CMP_BASE, CMP_COMP };
 for(size_t i = 0; i < node_set.size(); ++i)
  DBG(1) DOUT() << "found diffs (" << lbl[i]
                << ", instance " << key_ << "): " << node_set[i].size() << endl;

 for(size_t i = 0; i < jv.size(); ++i)
  if(jv[i][ lbl[i] ].is_iterable())                             // if root is iterable
   remove_others_(node_set[i], jv[i][ lbl[i] ]);                // remove then all matching nodes
  else                                                          // root is atomic
   if(node_set[i].empty()) jv[i][ lbl[i] ] = OBJ{};             // set is as an empty set {}

 for(auto &json: jv) {                                          // output compared jsons
  auto jit = json.walk("[0]");
  if(not jit->empty()) cmp_ = RC_CMP_NEQ;
  output_by_iterator(jit, 0);
 }
 if(jv.front().front().type() != jv.back().front().type())
  cmp_ = RC_CMP_NEQ;
 ++key_;
}



// insert handles 4 parameter types:
//  1. static insert ( -i <json> )
//  2. insert from file ( -i <file> ) - file must contain a valid JSON
//  3. insert from cli ( -e -i <cli> \; ) - cli must return a valid JSON
//  4. insert by walk-path (-i <src walk-path>) effectively performing copy operation
// Destination walks (-w <dst walk-path>) define mode of insertion:
//  - if insertion point is single (only one walk given and it's non-iterable), then
//    all source jsons/walks will be attempted to be inserted into a single location
//  - if multiple insertions points given (ether multiple -w, or single -w with
//    an iterable walk-path), then sources are inserted in a circular manner//
//
// insert matrix (src -> dst):
//                                    insert w/o -m
//   to  \ from  |     [3,4]     |    {"a":3,"c":4}    |      "a":3,"c":4      |     "c"
//---------------+---------------+---------------------+-----------------------+---------------+	
//      [1,2]    |   [1,2,[3,4]  | [1,2,{"a":3,"c":4}] | [1,2,{"a":3},{"c":4}] |  [1,2,"c"]
// {"a":1,"b":2} | {"a":1,"b":2} | {"a":1,"b":2,"c":4} |  {"a":1,"b":2,"c":4}  | {"a":1,"b":2}
//     "a":1     |     "a":1     |        "a":1        |         "a":1         |     "a":1
//
//                                       insert with -m
//  to  \ from |        [3,4]        |     {"a":3,"c":4}     |      "a":3,"c":4      |     "c"
//-------------+---------------------+-----------------------+-----------------------+------------+
//    [1,2]    |      [1,2,3,4]      |       [1,2,3,4]       |       [1,2,3,4]       |  [1,2,"c"]
//{"a":1,"b":2}|{"a":[1,3],"b":[2,4]}|{"a":[1,3],"b":2,"c":4}|{"a":[1,3],"b":2,"c":4}|{"a":1,"b":2}
//    "a":1    |     "a":[1,3,4]     |      "a":[1,3,4]      |      "a":[1,3,4]      |  "a":[1,"c"]
//
// Basic principles:
//  o w/o '-m' src param (-i src) is considered as a whole, with '-m' as an mergeable iterable
//  o insert operation never rewrites the dst data (-w dst), while '-m' may extend it


void Jtc::insert_json() {
 // plug-in insert_by_iterator and let process walks
 if(opt_[CHR(OPT_WLK)].hits() == 0)                             // no -w?
  opt_[CHR(OPT_WLK)] = json_.is_atomic()? "[0]": "[^0]";        // then walk from root
 parse_params_(CHR(OPT_INS));                                   // collect all sources

 bool is_json_atomic = json_.is_atomic();
 if(is_json_atomic) json_ = ARY{ move(json_) };
 subscriber_ = &Jtc::insert_by_iterator;
 walk_interleaved_();
 if(is_json_atomic) json_ = move(json_[0]);

 if(not isrc_.empty() and opt_[CHR(OPT_PRG)].hits() > 0)        // only work when walk-path is src
  purge_json();
}



void Jtc::insert_by_iterator(Json::iterator &it, size_t group) {
 // insert each/all -i processed jsons
 if(processed_by_cli_(it)) return;

 size_t max_key = not jsrc_.empty()? jsrc_.size(): isrc_.size();
 while(key_ < max_key) {
  DBG(1) DOUT() << "trying to insert instance " << key_ << " out of " << max_key << endl;

  bool is_cli_success{true};
  if(jsrc_.empty() and not isrc_[key_].is_valid())              // key is in isrc_ and invalid
   cerr << "error: walk instance " << key_
        << " became invalid due to prior operations, skipping" << endl;
  else {
   if(ecli_) {
    is_cli_success = execute_cli_(jexc_[0], isrc_[key_], wns_[&isrc_[key_]]);
    if(is_cli_success) isrc_[key_] = jexc_[0].walk();
   }
   if(is_cli_success)
    merge_jsons_(it, jsrc_.empty()? isrc_[key_]: jsrc_[key_].walk());
  }
  ++key_;
  if(is_multi_walk_) {
   if(key_ >= max_key) key_ = 0;
   break;
  }
 }
}



void Jtc::purge_json(void) {
 // remove all json nodes pointed by iterator(s), or do reverse
 if(opt_[CHR(OPT_WLK)].hits() == 0)                             // no -w?
  opt_[CHR(OPT_WLK)] = "[^0]";                                  // then walk from root

 if(opt_[CHR(OPT_PRG)].hits() > 1)
  return crop_out_();
 for(auto &wp: opt_[CHR(OPT_WLK)]) {                           // process all walks
  walk_vec ji = collect_walks_(wp);                            // collect all purging points

  for(size_t i = 0; i < ji.size(); ++i) {                      // purge all walked instances
   auto & rec = *ji[i];
   if(ji[i].is_valid()) {                                      // i.e. hasn't been deleted already
    DBG(1) DOUT() << "purging walk instance " << i << endl;
    if(rec.is_root()) rec.clear();                             // if root, erase everything
    else rec[-1].erase(ji[i]);
   }
  }
 }
}



// update handles 4 cases:
//  1. static update ( -u <json> )
//  2. update from file ( -u <file> ) - file must contain a valid JSON
//  3. update from cli ( -e -u <cli> \; ) - cli must return a valid JSON
//  4. update by walk-path (-u <src walk-path>) effectively performing overwrite/move operation
//
// update matrix (src -> dst):
//                               update w/o -m
//   to  \ from  |   [3,4]   |   {"a":3,"c":4}   | "a":3 |  "c"
//---------------+-----------+-------------------+-------+--------+
//      [1,2]    |   [3,4]   |   {"a":3,"c":4}   |   3   |   "c"
// {"a":1,"b":2} |   [3,4]   |   {"a":3,"c":4}   |   3   |   "c"
//     "a":1     | "a":[3,4] | "a":{"a":3,"c":4} | "a":3 | "a":"c"
//
//                                     update with -m
//   to  \ from  |     [3,4]     |    {"a":3,"c":4}    |     "a":3     |     "c"
//---------------+---------------+---------------------+---------------+----------------+
//      [1,2]    |     [3,4]     |        [3,4]        |     [3,2]     |   ["c",2]
// {"a":1,"b":2} | {"a":3,"b":4} | {"a":3,"b":2,"c":4} | {"a":3,"b":2} | {"a":"c","b":2}
//     "a":1     |   "a":[3,4]   |  "a":{"a":3,"c":4}  |  "a":{"a":3}  |   "a":"c"
//
// Basic principles:
//  o w/o '-m' dst param (-u dst) is considered as a whole, with '-m' as an mergeable iterable
//  o update operation rewrites the dst data (-w dst)

void Jtc::update_json() {
 // plug-in update_by_iterator and let process walks
 if(opt_[CHR(OPT_WLK)].hits() == 0)                             // no -w?
  opt_[CHR(OPT_WLK)] = json_.is_atomic()? "[0]": "[^0]";        // walk from root
 parse_params_(CHR(OPT_UPD));                                   // collect all sources

 bool is_json_atomic = json_.is_atomic();
 if(is_json_atomic) json_ = ARY{ move(json_) };
 subscriber_ = &Jtc::update_by_iterator;
 walk_interleaved_();
 if(is_json_atomic) json_ = move(json_[0]);

 if(not isrc_.empty() and opt_[CHR(OPT_PRG)].hits() > 0)        // only work when walk-path is src
  purge_json();
}



void Jtc::update_by_iterator(Json::iterator &it, size_t group) {
 // update each/all -u processed jsons
 if(lbl_update_ == false)                                       // not faced label update yet
  lbl_update_ = not it.walks().empty() and it.walks().back().jsearch == Json::key_of_value;
 else                                                           // lbl update occurred, then
  if(not it.is_valid())                                         // verify sanity of dst walks
   { cerr << "error: destination walk became invalid, skipping update" << endl; return; }
 if(processed_by_cli_(it)) return;                              // -e w/o trailing -u processed

 size_t max_key = not jsrc_.empty()? jsrc_.size(): isrc_.size();
 while(key_ < max_key) {
  DBG(1) DOUT() << "trying to update instance " << key_ << " out of " << max_key << endl;

  bool is_cli_success{true};
  if(jsrc_.empty() and not isrc_[key_].is_valid())              // it's isrc source and key invalid
   cerr << "error: walk instance " << key_
        << " became invalid due to prior operations, skipping" << endl;
  else {                                                        // isrc_ is valid, or jsrc[key_]
   if(ecli_) {                                                  // -e with trailing -u
    is_cli_success = execute_cli_(jexc_[0], isrc_[key_], wns_[&isrc_[key_]]);
    if(is_cli_success) isrc_[key_] = jexc_[0].walk();           // put root's itr for the next line
   }
   if(is_cli_success)
    update_jsons_(it, jsrc_.empty()? isrc_[key_]: jsrc_[key_].walk());
  }

  ++key_;
  if(is_multi_walk_) {
   if(key_ >= max_key) key_ = 0;
   break;
  }
 }
}



int Jtc::swap_json(void) {
 // swap around nodes pointed by 2 walk paths
 check_walk_requirements_(2, exact);

 vector<walk_vec> swaps{2};                                     // collect all walks in here
 swaps[0] = collect_walks_(opt_[CHR(OPT_WLK)].str(1));
 swaps[1] = collect_walks_(opt_[CHR(OPT_WLK)].str(2));

 size_t max_i = min(swaps[0].size(), swaps[1].size());
 for(size_t i = 0; i < max_i; ++i) {                            // swap only paired walks
  if(not swaps[0][i].is_valid() or not swaps[1][i].is_valid()) {
   cerr << "fail: walk instance " << i
        << " became invalid due to prior operations, aborting" << endl;
   return RC_WP_INV;
  }
  swap(*swaps[0][i], *swaps[1][i]);
 }
 return RC_OK;
}



int Jtc::walk_json(void) {
 // walk all -w paths, output via subscriber
 Json jdb;                                                      // integrity check in debugs only
 DBG(0)
  { jdb = json_; DOUT() << "copying input json for integrity check (debug only)" << endl; }

 if(opt_[CHR(OPT_JSN)].hits() == 1) jout_ = ARY{};
 subscriber_ = &Jtc::output_by_iterator;
 walk_interleaved_();

 // after walking all paths
 if(opt_[CHR(OPT_JSN)].hits() > 0)                              // -j, jout_ contains the output
   write_json(jout_, false);
                                                                // otherwise it's already stdout'ed
 // check json integrity...
 DBG(0)
  DOUT() << "source json integrity check: "
         << (jdb == json_? "Good. (...phew!)": "BROKEN! (aaaa! panic! bug found!)" ) << endl;
 return RC_OK;
}



void Jtc::output_by_iterator(Json::iterator &wi, size_t group) {
 // prints json element from given iterator
 // in case of -j option: collect into provided json container rather than print
 Json tmp;
 tmp.type() = Jnode::Neither;
 DBG().severity(tmp);

 if(opt_[CHR(OPT_TMP)].hits() >= 1) {                           // -T given
  tmp = interpolate_(opt_[CHR(OPT_TMP)].str(key_ + 1), wns_[&wi], wi, Json::strict_trailing);
  if(++key_  >= opt_[CHR(OPT_TMP)].size() - 1) key_ = 0;        // -1: adjust b/c of opt_'s default
 }

 typedef void (Jtc::*jd_ptr)(Json::iterator &, size_t, const Json &);
 static jd_ptr demux_out[2] = {&Jtc::console_output_, &Jtc::jsonized_output_};
 (this->*demux_out[opt_[CHR(OPT_JSN)].hits() > 0])(wi, group, tmp);
 last_group_ = group;
}



void Jtc::jsonized_output_(Json::iterator &wi, size_t group, const Json &jtmp_ref) {
 // demux output based on jout_ state: either to ARY or OBJ
 typedef void (Jtc::*jo_ptr)(Json::iterator &, size_t, const Json &);
 static jo_ptr jsonize[2] = {&Jtc::jsonized_output_obj_, &Jtc::jsonized_output_ary_};

 (this->*jsonize[jout_.is_array()])(wi, group, jtmp_ref);
}



void Jtc::jsonized_output_ary_(Json::iterator &wi, size_t group, const Json &jtmp_ref) {
 // if -j option given, output into jout_ as Array
 auto create_obj = [&]{ return opt_[CHR(OPT_SEQ)].hits() > 0?
                               group >= last_group_: group > last_group_; };
 auto &sr = jtmp_ref.type() == Jnode::Neither? *wi: jtmp_ref.root();
 if(not opt_[CHR(OPT_LBL)])                                     // -l not given, make simple array
  { jout_.push_back(sr); return; }
                                                                // -l given (combine relevant grp)
 if(not sr.has_label())                                         // sr has no label, push to back
  { jout_.push_back(sr); return; }
                                                                // sr has label, merge
 if(create_obj() or jout_.empty())                              // time to create a new object
  { jout_.push_back( OBJ{} ); convert_req_ = false; }
 if(not jout_.back().is_object())
  jout_.push_back( OBJ{} );

 if(jout_.back().count(sr.label()) == 0) {                      // no label recorded (first time)
  jout_.back()[sr.label()] = sr;                                // copy supernode
  if(sr.is_array()) convert_req_ = true;                        // if subsequent update, will conv.
  return;
 }
                                                                // label already exist
 if(convert_req_ or not jout_.back()[sr.label()].is_array()) {  // convert to array then
  Json tmp{ move(jout_.back()[sr.label()]) };
  (jout_.back()[sr.label()] = ARY{}).push_back( move(tmp) );
  convert_req_ = false;
 }
 jout_.back()[sr.label()].push_back( sr );                      // & push back into converted array
}



void Jtc::jsonized_output_obj_(Json::iterator &wi, size_t group, const Json &jtmp_ref) {
 // if -jj option given, output into jout_ as Object (items w/o label are ignored)
 auto &sr = jtmp_ref.type() == Jnode::Neither? *wi: jtmp_ref.root();

 if(not sr.has_label()) return;                                 // sr has no label, ignore

 auto found = jout_.find(sr.label());
 if(found == jout_.end())                                       // it's a new label
  { jout_[sr.label()] = sr; return; }                           // copy supernode
                                                                // label already exist
 if(c2a_.count(sr.label()) == 0) {                              // and not converted to array yet
  jout_[sr.label()] = ARY{ move(jout_[sr.label()]) };
  c2a_.emplace(sr.label());
 }
 jout_[sr.label()].push_back(sr);
}



void Jtc::console_output_(Json::iterator &wi, size_t group, const Json &jtmp_ref) {
 // no -j given, print out element pointed by iter wi
 if(opt_[CHR(OPT_SZE)].hits() > 1)
  { cout << wi->size() << endl; return; }

 auto &sr = jtmp_ref.type() == Jnode::Neither? *wi: jtmp_ref.root();
 bool unquote{opt_[CHR(OPT_QUT)].hits() >= 2};                  // -qq given
 bool inquote{opt_[CHR(OPT_RAW)].hits() >= 2};                  // -rr given, inquote

 if(opt_[CHR(OPT_LBL)] and sr.has_label())                      // -l given
  { cout << '"' << sr.label() << "\": ";  unquote = false; }    // then print label (if present)
 if(unquote and sr.is_string())
  cout << json_.unquote_str(sr.str()) << endl;                  // don't try collapsing it into
 else {
  if(inquote) cout << '"' << json_.inquote_str(sr.to_string(Jnode::Raw)) << '"' << endl;
  else cout << sr << endl;                                      // a single operation!
 }

 if(opt_[CHR(OPT_SZE)])                                         // -z given
  cout << SIZE_PFX << sr.size() << endl;
}



//
// private methods
//
void Jtc::check_walk_requirements_(unsigned x, WalkReq req) {
 // check if actual number of walks fits min requirements
 if(req == exact)
  { if(opt_[CHR(OPT_WLK)].hits() == x) return; }
 else
  { if(opt_[CHR(OPT_WLK)].hits() >= x) return; }

 cerr << "fail: " << (req==exact? "exactly ":"at least ") << x << " '-" STR(OPT_WLK)
         "' must be given" << endl;
 exit(RC_WLK_MISS);
}



void Jtc::crop_out_(void) {
 // output walked elements preserving source json structure
 set<const Jnode*> preserve;

 for(auto &wp: opt_[CHR(OPT_WLK)])                              // process all walks
  for(auto it = json_.walk(wp, Json::keep_cache); it != json_.end(); ++it)
   preserve.insert(&it->value());                              // collect all walked elements

 DBG(0) DOUT() << "preserved instances: " << preserve.size() << endl;
 remove_others_(preserve, json_.root());
}


bool Jtc::remove_others_(set<const Jnode*> &ws, Jnode &jn) {
 // return true if node has to be removed. remove all but those in ws
 bool remove = true;
 if(ws.count(&jn.value())) return false;                        // preserve root itself if found

 for(auto it = jn.begin(); it != jn.end();) {
  if(it->is_atomic()) {
   if(ws.count(&it->value()) == 0)                              // atomic entry is not among walked
    { jn.erase(it); continue; }                                 // erase the entry then
   else remove = false;                                         // don't erase this iterable
  }
  else {                                                        // it's nested iterable
   if(ws.count(&it->value()) == 1)                              // if itself a walked entry
    remove = false;                                             // then preserve it entirely
   else {                                                       // otherwise figure if to removed
    if(remove_others_(ws, *it) == true)                         // no walked entries inside
     { jn.erase(it); continue; }
    else remove = false;
   }
  }
  ++it;
 }
 return remove;
}



Jtc::walk_vec Jtc::collect_walks_(const string &walk_path) {
 // collect all walk iterations from given walk path (used by insert/update/swap/purge)
 walk_vec walk_itr;
 if(isrc_.empty()) {                                            // called for a standalone key
  for(auto it = json_.walk(walk_path, Json::keep_cache); it != json_.end(); ++it)
   walk_itr.push_back(it);
  DBG(0) DOUT() << "walk path: '" << walk_path << "', instances: " << walk_itr.size() << endl;
 }
 else {                                                         // called from either insert/update
  for(auto &it: isrc_)
   walk_itr.push_back(it.VALUE);
  DBG(0) DOUT() << "collected iteration instances: " << walk_itr.size() << endl;
 }
 return walk_itr;
}



void Jtc::compare_jsons_(const Jnode &j1, set<const Jnode*> &s1,
                         const Jnode &j2, set<const Jnode*> &s2) {
 // compare j1 <> j2, put into s1 and j2 nodes which are different (require preserving)
 if(j1.type() != j2.type()) {
  s1.insert(&j1);
  s2.insert(&j2);
  return;
 }

 if(j1.is_object()) {
  set<string> keys;
  for(auto &r: j1) keys.insert(r.label());
  for(auto &r: j2) keys.insert(r.label());
  for(auto &l: keys) {
   if(j1.count(l) == j2.count(l))
    { compare_jsons_(j1[l], s1, j2[l], s2); continue; }
   if(j1.count(l) == 0) s2.insert(&j2[l]);
   else s1.insert(&j1[l]);
  }
  return;
 }

 if(j1.is_array()) {
  auto i1 = j1.begin(), i2 = j2.begin();
  while(i1 != j1.end() or i2 != j2.end()) {
   if(i1 == j1.end()) { s2.insert(&i2->value()); ++i2; continue; }
   if(i2 == j2.end()) { s1.insert(&i1->value()); ++i1; continue; }
   compare_jsons_(i1->value(), s1, i2->value(), s2);
   ++i1; ++i2;
  }
  return;
 }

 if(j1 == j2) return;
 s1.insert(&j1);
 s2.insert(&j2);
}



void Jtc::merge_jsons_(Json::iterator &it_dst, Json::iterator it_src) {
 // merge 2 jsons. convert to array non-array dst jsons (predicated by -m)
 if(it_dst.walks().back().jsearch == Json::key_of_value)        // '<>k' facing
  { cerr << "error: insert into label not applicable, use update" << endl; return; }

 if(it_dst->is_object()) {                                      // dst is object
  if(it_src->has_label())                                       // it's coming from -i walk-path
   merge_into_object_(*it_dst, OBJ{ LBL{it_src->label(), *it_src} }, preserve); //
  else
   if(it_src->is_iterable())                                    // from either json/walk-path
    merge_into_object_(*it_dst, *it_src, preserve);
   else
    cerr << "error: only an iterable could be insert-merged with an object" << endl;
  return;
 }

 if(merge_) {                                                   // -m given
  DBG(1) DOUT() << "merging into array" << endl;
  merge_into_array_(*it_dst, *it_src, preserve);                // merge any jsons
  return;
 }

 if(it_dst->is_array()) {                                       // merge only 1 json object
  it_dst->push_back(it_src->has_label()? OBJ{ LBL{it_src->label(), *it_src} }: *it_src);
  return;
 }
 cerr << "error: walk-path must point to an iterable, or use merging" << endl;
}



void Jtc::merge_into_array_(Jnode &dst, const Jnode &src, MergeObj mode) {
 // merge 2 jsons into array.
 // convert dst to array if non-array. convert src to array if non-iterable
 const Jnode *src_ptr = &src;
 Jnode src_array;

 if(not dst.is_array()) dst = ARY{ move(dst) };                 // convert to arrays if not yet
 if(not src.is_iterable())
  { src_array = ARY{ move(src) }; src_ptr = &src_array; }

 if(mode == preserve) {
  DBG(2) DOUT() << "pushing into destination" << endl;
  size_t safity_cnt = src_ptr->children();                      // to avoid endless loop when src
  for(auto &src_child: *src_ptr) {
   dst.push_back(src_child);
   if(--safity_cnt == 0) break;                                 // and dst is the same array
  }
  return;
 }
                                                                // mode == overwrite
 DBG(2) DOUT() << "overwriting destination" << endl;
 if(dst.empty()) return;
 auto di = dst.begin();
 for(auto &src_child: *src_ptr) {
  *di = src_child;
  if(++di == dst.end()) break;
 }
}



void Jtc::merge_into_object_(Jnode &dst, const Jnode &src, MergeObj mode) {
 // merge into object, recursively. assert(dst.is_object() and src.is_iterable())
 if(not src.is_object()) {                                      // merge arr->obj, node by node
  DBG(2) DOUT() << "merge array into object" << endl;
  if(not merge_) return;                                        // only if -m given
  auto di = dst.begin();
  for(auto &src_child: src.is_array()? src: ARY{ move(src) }) { // go by every element in the src
   if(di == dst.end()) break;
   if(mode == overwrite)
    *di = src_child;                                            // overwrite upon instruction only
   else
    merge_into_array_(*di, src_child, mode);
   ++di;
  }
  return;
 }
                                                                // merge object->object recursively
 DBG(2) DOUT() << "merge objects" << endl;
 for(auto &src_child: src) {                                    // go by every element in the src
  auto di = dst.find(src_child.label());                        // dst iterator
  if(di == dst.end())                                           // labels not clashing
   { dst[src_child.label()] = src_child; continue; }            // merge non-clashing labels

  if(di->is_object() and src_child.is_object())                 // both clashing elements are OBJ
   { merge_into_object_(*di, src_child, mode); continue; }      // merge OBJ elements recursively

  if(merge_) {                                                  // if -m given
   if(mode == overwrite)
    *di = src_child;                                            // overwrite upon instruction only
   else
    merge_into_array_(*di, src_child, mode);                    // merge clashing non-obj into arr
  }
 }
}



void Jtc::update_jsons_(Json::iterator &it_dst, Json::iterator it_src) {
 // update dst with src, merge jsons with overwrite if -m is given
 if(it_dst.walks().back().jsearch == Json::key_of_value) {      // facilitate '<>k'
  DBG(2) DOUT() << "label being updated" << endl;
  if(merge_)
   { cerr << "error: merge not applicable in label update, ignoring" << endl; }
  if(not it_src->is_string())
   { cerr << "error: only labels could be updated with valid JSON strings" << endl; return; }
  auto & parent = (*it_dst)[-1];
  if(not parent.is_object())
   { cerr << "error: labels could be updated in objects only" << endl; return; }
  if(*it_src == *it_dst) return;                                // do not move then
  parent[it_src->str()] = move(parent[it_dst->str()]);
  parent.erase(it_dst->str());
  return;
 }

 if(not merge_) {
  DBG(2) DOUT() << "destination being overwritten" << endl;
  *it_dst = *it_src;
  return;
 }

 DBG(2) DOUT() << "destination being merged" << endl;
 if(it_dst->is_object())
  merge_into_object_(*it_dst, it_src->has_label()?
                              OBJ{ LBL{ it_src->label(), *it_src } }: *it_src, overwrite);
 else
  if(it_dst->is_array())
   merge_into_array_(*it_dst, *it_src, overwrite);
  else
   *it_dst = it_src->has_label()? OBJ{ LBL{ it_src->label(), *it_src } }: *it_src;
}



bool Jtc::processed_by_cli_(Json::iterator &it) {
 // if -e given alone -  just execute cli, return true if -e given alone and executed,
 // return false otherwise: -e not given, or given but not alone; don't execute if so
 if(ecli_ == false or not isrc_.empty()) return false;          // no -e, or isrc has trailing opt.

 if(execute_cli_(jexc_[0], it, wns_[&it]) == true) {            // cli resulted in a valid json
  typedef void (Jtc::*op_ptr)(Json::iterator &, Json::iterator);
  static op_ptr op_json[2] = {&Jtc::update_jsons_, &Jtc::merge_jsons_};
  (this->*op_json[subscriber_ == &Jtc::insert_by_iterator])(it, jexc_[0].walk());
 }
 return true;
}



bool Jtc::execute_cli_(Json &json, Json::iterator &jit, const Json::map_jn &ns) {
 // execute cli in -i/u option (interpolating jit if required) and parse the result into json
 sh_.system( reconcile_ui_(jit, ns) );
 if(sh_.rc() != 0)
  { cerr << "error: shell returned error (" << sh_.rc() << ")" << endl; return false; }
 if(sh_.stdout().empty())
  { DBG(1) DOUT() << "shell returned empty result, not updating" << endl; return false; }

 DBG().severity(json);
 try { json.parse(sh_.stdout()); }
 catch(Json::stdException & e) {                                // promote output to JSON string
  string out = json.inquote_str(sh_.stdout());                  // inquote all occurrences of ["\]
  out = regex_replace(out, regex{R"(\r)"}, R"(\r)");
  out = regex_replace(out, regex{R"(\n)"}, R"(\n)");
  if(out.compare(out.size()-2, 2, R"(\n)") == 0)                // erase possibly trailing \n
   out.erase(out.size()-2);
  json.parse("\"" + out + "\"");
 }
 return true;
}



string Jtc::reconcile_ui_(Json::iterator &jit, const Json::map_jn &ns) {
 // reconcile here options -i, or -u. or interpolate a static json string
 stringstream is;                                               // is: interpolation stream
 size_t opt_idx = 0;
 string dlm;
 for(const auto & opt_param: opt_[cr_.opt_ui()]) {              // reconcile here all -u/-i options
  if(++opt_idx >= cr_.opt_e_found()) break;
  is << dlm << opt_param;
  dlm = ' ';
 }

 Json ij = interpolate_(is.str(), ns, jit, Json::relaxed_trailing, false);
 DBG(1) DOUT() << "interpolated & quoted string: '" << quote_str(ij.str()) << "'" << endl;
 return quote_str(ij.str());
}



void Jtc::parse_params_(char option) {
 // attempt to parse parameter (of either -c, -i or -u), first assume it's a file,
 // then assume it's a JSON, finally it's a walk-path
 // also, parse a mix or arguments, i.e. -i<static> -i<walk-path>,
 // use jexc_[0] as a storage for <static> json and fill out isrc from walking the former.
 // if there's a template provided, apply interpolation
 map_json tmp;                                                  // interpolated templates (if any)
 if(ecli_)                                                      // -e detected
  for(size_t i = cr_.opt_e_found(); cr_.opt_ui() and i <= opt_[cr_.opt_ui()].hits(); ++i)
   extend_itr_(json_.walk(opt_[cr_.opt_ui()].str(i), Json::keep_cache));
   // the loop conditions make this call only if trailing options (-u/-i) are present
 else
  for(auto & arg: opt_[option])
   try {
    string jstr{istream_iterator<char>(ifstream{arg, ifstream::in} >> noskipws),
               istream_iterator<char>{}};
    DBG().severity(jsrc_[jsrc_.size()]);
    DBG(1) DOUT() << "attempting to parse parameter (" << arg << ") as json" << endl;
    jsrc_[jsrc_.size()-1].parse(jstr.empty()? arg: jstr,
                                jstr.empty()? Json::strict_no_trail: Json::strict_trailing);
    if(not jexc_.empty()) { jsrc_.erase(jsrc_.size()-1); continue; }
   }
   catch(Json::stdException & e) {                              // not a static json - a walk-path
    DBG(1) DOUT() << "attempting to parse parameter as a walk-path" << endl;
    jsrc_.erase(jsrc_.size()-1);
    if(not jsrc_.empty())                                       // mix of args: <file> <walk-path>
     { jexc_[0] = move(jsrc_.begin()->VALUE); jsrc_.clear(); }  // use only 1st static json
    extend_itr_(tmp, (jexc_.empty()? json_: jexc_[0]).ns(),
                     (jexc_.empty()? json_: jexc_[0]).walk(arg, Json::keep_cache));
   }

 if(not tmp.empty()) jsrc_ = move(tmp);
 DBG(0)
  DOUT() << "option '-" << option << "': total jsons: " << jsrc_.size()
         << ", total iterators: " << isrc_.size() << endl;
 opt_[CHR(OPT_TMP)].reset();
}



void Jtc::extend_itr_(Json::iterator jit) {
 // extend jit till end, and interpolate all templates (if any) NS put into and lock wns_
 while( jit != jit.end() ) {                                // extend all iterators until end
  isrc_[isrc_.size()] = jit;
  auto & jit_ref = isrc_.rbegin()->VALUE;
  wns_[&jit_ref] = json_.ns();
  ++jit;
 }
 DBG(2) DOUT() << "walk's name-space locked exclusively for trailing options" << endl;
 wns_[nullptr];                                              // lock wns exclusively for trailing
}



void Jtc::extend_itr_(map_json &tmp, const Json::map_jn &ns, Json::iterator jit) {
 // extend jit till end, and interpolate all templates (if any)
 while( jit != jit.end() ) {                                // extend all iterators until end
  isrc_[isrc_.size()] = jit;

  for(auto & o: opt_[CHR(OPT_TMP)]) {                       // try interpolationg all -T options
   Json ijsn = interpolate_(o, ns, jit, Json::strict_trailing);
   tmp[tmp.size()] = ijsn.type() != Jnode::Neither? move(ijsn): Json{*jit};
  }

  ++jit;
 }
}



void Jtc::build_path_(Jnode &jpath, Json::iterator &jit) {
 // build json path (Json ARRAY) for given Json::iteratof
 Jnode *node = &jit.json().root();

 for(auto &itr: jit.path())
  if(node->is_array()) {
   long idx = strtol(itr.lbl.c_str(), nullptr, 16);
   jpath.push_back(idx);
   node = &(*node)[idx];
  }
  else {
   jpath.push_back(itr.lbl);
   node = &(*node)[itr.lbl];
  }
}



Json Jtc::interpolate_(StringOvr tmp, const Json::map_jn &ns,
                        Json::iterator &jit, Json::ParseTrailing pt, bool parse) {
 // wrapper for interpolate_tmp_, including the empty case
 // designed behavior:
 //  o Template (tmp) may or may not contain interpolations e.g.: `true` or  `{{..}}`
 //  o returned result must indicate either a successful interpolation of failure
 //  o a successful interpolation results in a valid JSON
 //  o thus: returned result is JSON (invalid interpolation indicated by returning Json with
 //    Jtype::Neither)
 //  o user might not desire the interpolated value to undergo JSON parsing, then
 //    the indication of that is passed via parameter and result returned as a JSON string
 Jnode jpath{ARY{}};

 if(not interpolate_tmp_(tmp, {pair<string, Jnode>(PATH_STR, {})}).empty()) {
  build_path_(jpath, jit);
  string spath;
  for(auto &itm: jpath) spath += itm.val() + PATH_SPR;          // stringify json path
  spath.pop_back();                                             // remove trailing PATH_SPR
  tmp = interpolate_tmp_(tmp, {pair<string, Jnode>(PATH_STR, spath)});
 }
 if(not interpolate_tmp_(tmp, {pair<string, Jnode>(PATH_JSN, {})}).empty()) {
  if(jpath.empty())
   build_path_(jpath, jit);
  tmp = interpolate_tmp_(tmp, {pair<string, Jnode>(PATH_JSN, jpath)});
 }

 tmp = interpolate_tmp_(tmp, ns);
 tmp = interpolate_tmp_(tmp, {pair<string, Jnode>("", *jit)});  // crafted ns for: {{}}

 Json rjv;                                                       // return json value
 DBG().severity(rjv);
 if(parse)
  try { rjv.parse(tmp, pt); }
  catch(Json::stdException & e) { rjv.root().type() = Jnode::Neither; }
 else
  rjv = move(STR{tmp});
 return rjv;
}



string Jtc::interpolate_tmp_(const string &tmp, const Json::map_jn &ns) {
 // interpolate template (tmp_) from the namespace (ns)
 // return an empty string if no interpolation occurs (tmp not altered)
 string out{tmp};
 vector<string> head{{INTRP_VAL[0]}, string{INTRP_VAL[0]} + INTRP_VAL[0]};  // [ "{", "{{" ]
 vector<string> tail{{INTRP_VAL[1]}, string{INTRP_VAL[1]} + INTRP_VAL[1]};  // [ "}", "}}" ]

 while(not head.empty()) {
  for(const auto & ins: ns) {
   string sk = head.back() + ins.KEY + tail.back();         // sk: search key
   string sv = ins.VALUE.to_string(Jnode::Raw);             // sv: json string (literal) value
   if(head.back().size() == 1)                              // i.e. {} interpolation
    if(sv.front() == '"') sv = sv.substr(1, sv.size()-2);   // drop quotes in string
   for(size_t interpolate_pos = out.find(sk);
              interpolate_pos != string::npos;
              interpolate_pos = out.find(sk, interpolate_pos + 1))
    out.replace(interpolate_pos, sk.size(), sv);
  }
  head.pop_back();
  tail.pop_back();
 }
 if(out == tmp) out.clear();                                // interpolation fails entirely
 return out;
}



// interleaved walks engaged when multiple walk paths are given (in absence of -n)
// processing of interleaved walks is somewhat complex:
// 1. collect all iterators produced by each walk path into a vector (wpi), e.g
//    [ [w0_i0, w0_i1, w0_iend],
//      [w1_i0, w1_i1, w1_i2, w1_iend] ]
// 2. for all instances build a matrix from front iterators offsets (fom) only for
//    entire walk state:
//    - iterator offset for a built walk state returns -1 if walk step is non-iterable,
//      otherwise walk position's current offset is returned, e.g.:
//      w0_i0 and w1_i0, result in the probable FOM:
//          [ [w0_i0_p0 (0), w0_i0_p1 (0), w0_i0_p2 (-1),
//            [w1_i0_p0 (0), w1_i0_p1 (-1)] ]
// 3. then build a vector referring actual instances: actual is the instance that is
//    non-empty (e.g.: 0, 1)
// 4. among actual iterators, select first in the relevant group, give it to the subscriber
//    and remove it from the wpi
// 5. repeat until entire wpi is empty

void Jtc::walk_interleaved_(void) {
 // collect all walks (-w/-i/-u) and feed one by one to the subscriber
 json_.clear_ns();
 if(wns_.count(nullptr) == 0)                                   // if not locked for trailed opt
  wns_.clear();
 deque<walk_deq> wpi;

 for(const auto &walk_str: opt_[CHR(OPT_WLK)]) {                // process all -w arguments
  wpi.push_back( {json_.walk(walk_str.find_first_not_of(" ") == string::npos?
                              "[^0]": walk_str, Json::keep_cache)} );
  if(opt_[CHR(OPT_SEQ)] and wpi.size() > 1) {                   // -n and multiple -w given
   wpi.front().push_back( move(wpi.back().front()) );           // move iterator to front wpi
   wpi.pop_back();                                              // drop last instance
  }

  auto & wi = wpi.back();                                       // wi: deque<Json::iterators>
  if(wi.back() != wi.back().end())
   while(true) {                                                // extend all iterators until end
    if(wns_.count(nullptr) == 0)                                // if wns_ is not locked,
     wns_[&wi.back()] = json_.ns();                             // preserve walked namespace
    wi.push_back(wi.back());                                    // make new copy (next instance)
    ++wi.back();                                                // and iterate
    if(wi.back() == wi.back().end()) break;
   }
  wi.pop_back();                                                // remove last (->end()) iterator
 }

 is_multi_walk_ = opt_[CHR(OPT_WLK)].hits() > 1 or              // or hits == 1
                  not(wpi.size() == 1 and not wpi.front().front().is_walkable());
 DBG(1) {
  DOUT() << "multi-walk: " << (is_multi_walk_? "true": "false") << endl;
  DOUT() << DBG_PROMPT(1) << "walk-path instances: " << wpi.size() << ":" << endl;
  for(size_t i = 0; i < wpi.size(); ++i)
   DOUT() << DBG_PROMPT(1) << "instance: " << i << ", iterators: " << wpi[i].size() << endl;
 }

 while( any_of(wpi.begin(), wpi.end(), [](walk_deq &wi){ return not wi.empty(); }) )
  process_walk_iterators_(wpi);
}



void Jtc::process_walk_iterators_(deque<walk_deq> &wpi) {
 // build front iterators offset matrix: wpi may contain empty deque
 vector<vector<long>> fom(wpi.size());                          // front offset matrix

 size_t longest_walk = build_front_grid_(fom, wpi);
 vector<size_t> actual_instances;                               // ai: those which not ended yet
 for(size_t idx = 0; idx < fom.size(); ++idx)                   // init non-empty fom indices
  if(not fom[idx].empty()) actual_instances.push_back(idx);

 process_offsets_(wpi, fom, longest_walk, actual_instances);
}



void Jtc::process_offsets_(deque<walk_deq> &wpi, vector<vector<long>> &fom,
                      size_t longest_walk, vector<size_t> &actuals) {
 // scans each offset's row (in wpi) and prints actual (non-empty) and relevant elements
 DBG(2) DOUT() << "walking offsets (" << longest_walk << ")";

 int grouping = 0;                                              // group size
 for(size_t offset = 0; offset < longest_walk; ++offset) {      // go across all offsets
  map<size_t, size_t> pos_ai, neg_ai;                           // build new actuals in here
  long lowest_offset = LONG_MAX, cnt = 0;                       // helpers to build new actuals
  if(DBG()(2)) DOUT() << ", [" << offset << "]:";

  for(auto ai: actuals) {                                       // a.inst. are with lowest offset
   if(DBG()(2)) DOUT() << ' ' << ai;
   if(offset >= fom[ai].size() or fom[ai][offset] < 0)          // negative or short actuals offset
    { neg_ai[cnt++] = ai; continue; }                           // collected separately
   if(fom[ai][offset] < lowest_offset)                          // found a lower counter
    { lowest_offset = fom[ai][offset]; pos_ai.clear(); }        // clear prior found actuals
   if(fom[ai][offset] == lowest_offset)                         // update new actuals
    pos_ai[cnt++] = ai;
  }

  actuals.clear();
  for(long idx = 0; idx < cnt; ++idx)
   if(pos_ai.count(idx)) actuals.push_back(pos_ai[idx]);
   else if(neg_ai.count(idx)) actuals.push_back(neg_ai[idx]);
  grouping = actuals.size();                                    // update groping
  if(grouping == 1) break;                                      // performance optimization
 }

 if(DBG()(2)) DOUT() << endl;                                   // close debug line
 if(actuals.empty())                                            // should never be the case
  { wpi.clear(); return; }                                      // in case, avoiding endless loop
 DBG(2) DOUT() << "output instance: " << actuals.front() << ", group size: " << grouping <<endl;
 (this->*subscriber_)(wpi[actuals.front()].front(), grouping);
 wpi[actuals.front()].pop_front();
}



size_t Jtc::build_front_grid_(vector<vector<long>> &fom, const deque<walk_deq> &wpi) {
 // build matrix from front iterator of each walk: using iterator's counter() method
 // that way it'll be possible to group relevant walk-paths during output
 size_t longest = 0;
 for(size_t idx = 0; idx < wpi.size(); ++idx) {                 // go over all given walk paths
  auto & wi = wpi[idx];
  if(wi.empty()) continue;                                      // no more iterators in this walk

  DBG(2) DOUT() << "instance " << idx;
  for(size_t position = 0; position < wi.front().walk_size(); ++position) {
   fom[idx].push_back( wi.front().counter(position) );
   if(DBG()(2)) DOUT() << (position == 0? " front offsets matrix: ": ", ") << fom[idx].back();
  }

  if(DBG()(2)) DOUT() << endl;
  longest = max(longest, fom[idx].size());
 }

 return longest;
}



void Jtc::location_(string::const_iterator &jbegin) {
 // show location of the exception, unicode UTF-8 supported
 string jsrc{&*jbegin};
 const char * pfx = "exception locus: ";
 for(auto &chr: jsrc)
  chr = chr AMONG('\r', '\n')? '|': static_cast<unsigned char>(chr) < ' '? ' ': chr;

 size_t from_start = Json::utf8_adjusted(0, jsrc, json_.exception_point() - jbegin);
 size_t to_end = Json::utf8_adjusted(json_.exception_point() - jbegin, jsrc);
 size_t ptr = from_start;

 if(from_start + to_end > DBG_WIDTH) {
  if(from_start > DBG_WIDTH/2) {
   jsrc = "..." + jsrc.substr(Json::byte_offset(jsrc, from_start - DBG_WIDTH/2 + 3));
   ptr = DBG_WIDTH/2;
  }
  if(to_end > DBG_WIDTH/2)
   jsrc = jsrc.substr(0, Json::byte_offset(jsrc, DBG_WIDTH - 3)) + "...";
 }

 DOUT() << pfx << jsrc << endl << DBG_PROMPT(0) << "exception spot: "
        << string(ptr, '-') << ">| (offset: " << from_start << ")" << endl;
}



string quote_str(const string &src) {
 // quote src: quote chars (which due to be quoted) outside of literals only
 string quoted;
 bool outside_literal = true;
 short backslash_seen = 0;
 for(auto chr: src) {
  if(not backslash_seen)
   if(chr == '\\') backslash_seen = 1;

  if(chr AMONG('\'', '"'))
   { if(not backslash_seen) outside_literal ^= true; }
  else                                                          // not ' or "
   if(outside_literal) {
    if(not isalnum(chr)) quoted += '\\';
    if(strchr("~`#$&*()\\|{};<>?!. \t", chr) != nullptr)        // don't quote those bash specials
     quoted.pop_back();
   }
  quoted += chr;

  if(backslash_seen)
    { if(++backslash_seen > 2) backslash_seen = 0; }
 }
 return quoted;
}



















