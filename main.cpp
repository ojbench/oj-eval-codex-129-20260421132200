#include <bits/stdc++.h>
using namespace std;

/*
 Simulator for scope-based variable handling.
 Supported commands:
 - Indent: enter new scope
 - Dedent: leave current scope
 - Declare [type] [name] [value]
 - Add [res] [v1] [v2]
 - SelfAdd [name] [value]
 - Print [name]

 Rules:
 - Name resolution searches from innermost scope outward.
 - Declare defines in current (innermost) scope only.
 - If a command is invalid (type mismatch, undeclared, bad literal, etc.),
   print "Invalid operation" and do not change any state.
 - Input size up to 1e6 lines; Indent levels < 100. Use fast IO.
*/

struct Value {
  // 0 = int, 1 = string
  int type; 
  long long i; // we will keep in range but store in long long
  string s;
};

// Variable stacks by name for O(1) current lookup
using VarStacks = unordered_map<string, vector<Value>>;

// Parsing helpers
static bool is_int_literal(const string& t, long long& out) {
  if (t.empty()) return false;
  int i = 0; bool neg = false;
  if (t[0] == '-' || t[0] == '+') { neg = (t[0]=='-'); i = 1; if (i==(int)t.size()) return false; }
  long long val = 0;
  for (; i<(int)t.size(); ++i) {
    if (!isdigit((unsigned char)t[i])) return false;
    val = val*10 + (t[i]-'0');
  }
  out = neg ? -val : val;
  return true;
}

static bool is_string_literal(const string& t, string& out) {
  if (t.size() >= 2 && t.front()=='"' && t.back()=='"') {
    out = t.substr(1, t.size()-2);
    return true;
  }
  return false;
}

// Read a possibly space-containing quoted string token from stream.
// Returns true and sets raw (including quotes) if successful.
static bool read_quoted_token(istream& in, string& raw) {
  string tok;
  if (!(in >> tok)) return false;
  if (tok.empty() || tok.front() != '"') return false;
  string acc = std::move(tok);
  while (acc.back() != '"') {
    string next;
    if (!(in >> next)) return false;
    acc.push_back(' ');
    acc += next;
  }
  raw = std::move(acc);
  return true;
}

// Helpers for line parsing
static inline void skip_spaces(const string& s, size_t& p) {
  while (p < s.size() && isspace((unsigned char)s[p])) ++p;
}
static inline bool read_token(const string& s, size_t& p, string& out) {
  skip_spaces(s, p);
  if (p >= s.size()) return false;
  size_t st = p;
  while (p < s.size() && !isspace((unsigned char)s[p])) ++p;
  out.assign(s.data()+st, p-st);
  return true;
}
static inline bool read_quoted(const string& s, size_t& p, string& out_raw) {
  skip_spaces(s, p);
  if (p >= s.size() || s[p] != '"') return false;
  size_t start = p;
  ++p; // skip opening quote
  // find closing quote
  size_t q = s.find('"', p);
  if (q == string::npos) return false;
  out_raw.assign(s.data()+start, q - start + 1);
  p = q + 1;
  return true;
}

int main() {
  ios::sync_with_stdio(false);
  cin.tie(nullptr);

  int n;
  if (!(cin >> n)) return 0;
  string dummy;
  getline(cin, dummy); // consume rest of line

  // var stacks per name
  VarStacks vars;
  vars.reserve(1 << 15);

  // per-scope declared names (for pop on dedent) and set for duplicate check
  vector<vector<string>> declared_names;
  vector<unordered_set<string>> declared_sets;
  declared_names.emplace_back();
  declared_sets.emplace_back();

  auto invalid = [](){ cout << "Invalid operation\n"; };

  string line;
  for (int li = 0; li < n; ++li) {
    if (!std::getline(cin, line)) break;
    size_t p = 0;
    string op;
    if (!read_token(line, p, op)) { invalid(); continue; }

    if (op == "Indent") {
      declared_names.emplace_back();
      declared_sets.emplace_back();
      continue;
    } else if (op == "Dedent") {
      if (declared_names.size() <= 1) { invalid(); }
      else {
        // pop all declarations in this scope
        for (const string& nm : declared_names.back()) {
          auto it = vars.find(nm);
          if (it != vars.end()) {
            if (!it->second.empty()) it->second.pop_back();
            if (it->second.empty()) vars.erase(it);
          }
        }
        declared_names.pop_back();
        declared_sets.pop_back();
      }
      continue;
    } else if (op == "Declare") {
      string type, name;
      if (!read_token(line, p, type) || !read_token(line, p, name)) { invalid(); continue; }
      // duplicate in same scope is invalid
      if (declared_sets.back().count(name)) { invalid(); continue; }
      Value val;
      if (type == "int") {
        string tok;
        if (!read_token(line, p, tok)) { invalid(); continue; }
        long long x;
        if (!is_int_literal(tok, x)) { invalid(); continue; }
        val.type = 0; val.i = x;
      } else if (type == "string") {
        string raw;
        if (!read_quoted(line, p, raw)) { invalid(); continue; }
        string ss;
        if (!is_string_literal(raw, ss)) { invalid(); continue; }
        val.type = 1; val.s = std::move(ss);
      } else { invalid(); continue; }
      vars[name].push_back(val);
      declared_sets.back().insert(name);
      declared_names.back().push_back(name);
      continue;
    } else if (op == "Add") {
      string r, a, b;
      if (!read_token(line, p, r) || !read_token(line, p, a) || !read_token(line, p, b)) { invalid(); continue; }
      auto ir = vars.find(r); auto i1 = vars.find(a); auto i2 = vars.find(b);
      if (ir==vars.end() || i1==vars.end() || i2==vars.end()) { invalid(); continue; }
      if (ir->second.empty() || i1->second.empty() || i2->second.empty()) { invalid(); continue; }
      Value &vr = ir->second.back();
      Value &v1 = i1->second.back();
      Value &v2 = i2->second.back();
      if (vr.type != v1.type || vr.type != v2.type) { invalid(); continue; }
      if (vr.type == 0) vr.i = v1.i + v2.i; else vr.s = v1.s + v2.s;
      continue;
    } else if (op == "SelfAdd") {
      string name;
      if (!read_token(line, p, name)) { invalid(); continue; }
      auto it = vars.find(name);
      if (it==vars.end() || it->second.empty()) { invalid(); continue; }
      Value &pv = it->second.back();
      if (pv.type == 0) {
        string tok;
        if (!read_token(line, p, tok)) { invalid(); continue; }
        long long x; if (!is_int_literal(tok, x)) { invalid(); continue; }
        pv.i += x;
      } else {
        string raw;
        if (!read_quoted(line, p, raw)) { invalid(); continue; }
        string ss; if (!is_string_literal(raw, ss)) { invalid(); continue; }
        pv.s += ss;
      }
      continue;
    } else if (op == "Print") {
      string name;
      if (!read_token(line, p, name)) { invalid(); continue; }
      auto it = vars.find(name);
      if (it==vars.end() || it->second.empty()) { invalid(); continue; }
      Value &pv = it->second.back();
      cout << name << ':';
      if (pv.type == 0) cout << pv.i; else cout << pv.s;
      cout << '\n';
      continue;
    } else {
      invalid();
    }
  }

  return 0;
}
