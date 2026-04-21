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

int main() {
  ios::sync_with_stdio(false);
  cin.tie(nullptr);

  int n;
  if (!(cin >> n)) return 0;
  vector<Scope> scopes;
  scopes.emplace_back(); // global scope

  string op;
  string type, name, v1, v2;

  auto invalid = [](){ cout << "Invalid operation\n"; };

  for (int line = 0; line < n; ++line) {
    if (!(cin >> op)) break;

    if (op == "Indent") {
      scopes.emplace_back();
      continue;
    }
    if (op == "Dedent") {
      if (scopes.size() <= 1) {
        invalid();
      } else {
        scopes.pop_back();
      }
      continue;
    }
    if (op == "Declare") {
      if (!(cin >> type >> name)) { invalid(); continue; }
      string valtok;
      if (!(cin >> valtok)) { invalid(); continue; }

      Value val;
      if (type == "int") {
        long long x;
        if (!is_int_literal(valtok, x)) { invalid(); continue; }
        val.type = 0; val.i = x;
      } else if (type == "string") {
        string ss;
        if (!is_string_literal(valtok, ss)) { invalid(); continue; }
        val.type = 1; val.s = std::move(ss);
      } else {
        invalid();
        continue;
      }

      // Declare always binds in current scope; shadow allowed
      scopes.back()[name] = std::move(val);
      continue;
    }
    if (op == "Add") {
      if (!(cin >> name >> v1 >> v2)) { invalid(); continue; }
      auto [pr, ir] = resolve(scopes, name);
      auto [p1, i1] = resolve(scopes, v1);
      auto [p2, i2] = resolve(scopes, v2);
      if (!pr || !p1 || !p2) { invalid(); continue; }
      if (pr->type != p1->type || pr->type != p2->type) { invalid(); continue; }
      if (pr->type == 0) {
        pr->i = p1->i + p2->i;
      } else {
        pr->s = p1->s + p2->s;
      }
      continue;
    }
    if (op == "SelfAdd") {
      if (!(cin >> name >> v1)) { invalid(); continue; }
      auto [p, idx] = resolve(scopes, name);
      if (!p) { invalid(); continue; }
      if (p->type == 0) {
        long long x;
        if (!is_int_literal(v1, x)) { invalid(); continue; }
        p->i += x;
      } else {
        string ss;
        if (!is_string_literal(v1, ss)) { invalid(); continue; }
        p->s += ss;
      }
      continue;
    }
    if (op == "Print") {
      if (!(cin >> name)) { invalid(); continue; }
      auto [p, idx] = resolve(scopes, name);
      if (!p) { invalid(); continue; }
      cout << name << ':';
      if (p->type == 0) cout << p->i;
      else cout << p->s;
      cout << '\n';
      continue;
    }

    // Unknown op (should not occur by problem promise)
    invalid();
  }

  return 0;
}

