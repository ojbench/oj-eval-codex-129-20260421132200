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

// Each scope maps name -> Value
using Scope = unordered_map<string, Value>;

// Find variable by name from inner to outer; return pointer and index of scope
static pair<Value*, int> resolve(vector<Scope>& scopes, const string& name) {
  for (int i = (int)scopes.size() - 1; i >= 0; --i) {
    auto it = scopes[i].find(name);
    if (it != scopes[i].end()) return { &it->second, i };
  }
  return { nullptr, -1 };
}

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

  vector<Scope> scopes;
  scopes.emplace_back(); // global scope

  auto invalid = [](){ cout << "Invalid operation\n"; };

  string line;
  for (int li = 0; li < n; ++li) {
    if (!std::getline(cin, line)) break;
    size_t p = 0;
    string op;
    if (!read_token(line, p, op)) { invalid(); continue; }

    if (op == "Indent") {
      scopes.emplace_back();
      continue;
    } else if (op == "Dedent") {
      if (scopes.size() <= 1) { invalid(); } else { scopes.pop_back(); }
      continue;
    } else if (op == "Declare") {
      string type, name;
      if (!read_token(line, p, type) || !read_token(line, p, name)) { invalid(); continue; }
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
      scopes.back()[name] = std::move(val);
      continue;
    } else if (op == "Add") {
      string r, a, b;
      if (!read_token(line, p, r) || !read_token(line, p, a) || !read_token(line, p, b)) { invalid(); continue; }
      auto [pr, ir] = resolve(scopes, r);
      auto [p1, i1] = resolve(scopes, a);
      auto [p2, i2] = resolve(scopes, b);
      if (!pr || !p1 || !p2) { invalid(); continue; }
      if (pr->type != p1->type || pr->type != p2->type) { invalid(); continue; }
      if (pr->type == 0) pr->i = p1->i + p2->i; else pr->s = p1->s + p2->s;
      continue;
    } else if (op == "SelfAdd") {
      string name;
      if (!read_token(line, p, name)) { invalid(); continue; }
      auto [pv, idx] = resolve(scopes, name);
      if (!pv) { invalid(); continue; }
      if (pv->type == 0) {
        string tok;
        if (!read_token(line, p, tok)) { invalid(); continue; }
        long long x; if (!is_int_literal(tok, x)) { invalid(); continue; }
        pv->i += x;
      } else {
        string raw;
        if (!read_quoted(line, p, raw)) { invalid(); continue; }
        string ss; if (!is_string_literal(raw, ss)) { invalid(); continue; }
        pv->s += ss;
      }
      continue;
    } else if (op == "Print") {
      string name;
      if (!read_token(line, p, name)) { invalid(); continue; }
      auto [pv, idx] = resolve(scopes, name);
      if (!pv) { invalid(); continue; }
      cout << name << ':';
      if (pv->type == 0) cout << pv->i; else cout << pv->s;
      cout << '\n';
      continue;
    } else {
      invalid();
    }
  }

  return 0;
}
