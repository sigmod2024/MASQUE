#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <random>
#include <fstream>
#include <map>
using namespace std;


vector<vector<uint32_t>> raw, share0, share1;

int main() {
	freopen("q9.txt", "r", stdin);

	std::ios::sync_with_stdio(false);

	string str;
    char usa[20] = "UNITED STATES";
    map<string, int> country;
    int cid = 0;
	while (getline(cin, str)) {
		char *token = strtok(str.data(), "|");
		vector<uint32_t> temp = {};
		while (token != NULL) {
			if (token[0] >= 'A' && token[0] <= 'Z') {
                string cn = token;
                if (country.find(cn) == country.end()) {
                    country[cn] = ++cid;
                }
				temp.push_back(country[cn]);
			} else {
				temp.push_back(atoi(token));
			}
			token = strtok(NULL, "|");
		}
		raw.push_back(temp);
	}
	cerr << raw.size() << endl;

	share0.resize(raw.size());
	share1.resize(raw.size());

	std::random_device rd;
	std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> uniform(0U, (uint32_t)-1);

	for (auto i=0; i<raw.size(); ++i) {
		share0[i].resize(raw[i].size());
		share1[i].resize(raw[i].size());
		for (auto j=0; j<raw[i].size(); ++j) {
			share0[i][j] = uniform(gen);
			share1[i][j] = raw[i][j] ^ share0[i][j];
		}
	}

	ofstream fout0("q90.shrdat");
	ofstream fout1("q91.shrdat");
	for (auto i=0; i<raw.size(); ++i) {
		for (auto j=0; j<raw[i].size(); ++j) {
			fout0 << share0[i][j] << ' ';
			fout1 << share1[i][j] << ' ';
		}
		fout0 << endl;
		fout1 << endl;
	}

	return 0;
}
