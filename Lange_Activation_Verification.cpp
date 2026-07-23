#include <bits/stdc++.h>
using namespace std;

struct Stats { uint64_t total=0, self=0; };

int main(int argc,char**argv){
    uint32_t N = 20000000;
    if(argc>1) N = stoul(argv[1]);
    vector<uint8_t> seen((size_t)N+1,0), isself((size_t)N+1,0);
    vector<uint64_t> checkpoints = {10,100,1000,10000,100000,250000,500000,750000,1000000,2000000,5000000,10000000,20000000};
    size_t cp=0; uint64_t selfcnt=0;
    vector<pair<uint32_t,uint64_t>> cpout;
    vector<uint64_t> path; path.reserve(1024);
    for(uint32_t n=1;n<=N;n++){
        bool self = !seen[n];
        if(self){ isself[n]=1; selfcnt++; }
        uint64_t x=n;
        while(true){
            if(x<=N){
                if(seen[(size_t)x]) break;
                seen[(size_t)x]=1;
            }
            if(x==1) break;
            if(x&1ULL){
                if(x > (numeric_limits<uint64_t>::max()-1)/3){ cerr<<"overflow\n"; return 2; }
                x=3*x+1;
            } else x>>=1;
        }
        while(cp<checkpoints.size() && checkpoints[cp]==n){ cpout.push_back({n,selfcnt}); cp++; }
    }
    cout << "N "<<N<<" self "<<selfcnt<<" density "<<setprecision(12)<<(long double)selfcnt/N<<"\n";
    cout << "CHECKPOINTS\n";
    for(auto [n,c]:cpout) cout<<n<<" "<<c<<" "<<setprecision(12)<<(long double)c/n<<"\n";

    vector<int> mods={3,6,12,18,24,54,162,486,1458,4374};
    for(int m:mods){
        vector<Stats> s(m);
        for(uint32_t n=1;n<=N;n++){ auto &z=s[n%m]; z.total++; z.self += isself[n]; }
        cout<<"MOD "<<m<<"\n";
        for(int r=0;r<m;r++) if(s[r].total){
            if(m<=24 || (r%6==1))
                cout<<r<<" "<<s[r].total<<" "<<s[r].self<<" "<<setprecision(10)<<(long double)s[r].self/s[r].total<<"\n";
        }
    }
    return 0;
}
