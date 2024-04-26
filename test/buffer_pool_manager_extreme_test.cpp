#include<iostream>
#include"./MemoryRiver.hpp"
#include"./MemoryRiverStd.hpp"
#include<cstdlib>

using namespace std;

struct T{
    int a[1000];
    int sum,num;
    int f(){
        int res=0;
        for (size_t i = 0; i < 1000; i++)
        {
            res+=a[i];
        }
        if (res!=sum) return -1;
        return res;
    }

    T():sum(0),num(0){
        for (size_t i = 0; i < 1000; i++)
        {
            a[i]=rand()*rand()>>10;
            sum+=a[i];
        }
    }

    void g(){
        for (size_t i = 0; i < 1000; i++)
        {
            a[i]*=2;
        }
        sum*=2;
    }

    bool operator!=(const T & other){
        if (sum!=other.sum) return true;
        if (num!=other.num) return true;
        for (size_t i = 0; i < 1000; i++)
        {
            if (a[i] != other.a[i]) return true;
        }
        return false;
    }
};

const int n=30000;
sol::MemoryRiver<T,4> STD("STD.file");
MemoryRiver<T,4> mr("save.file");
int off[n],offSTD[n];
T tmp,tmpSTD;

int main(){
    srand(2333);
    mr.initialise();
    STD.initialise();
    for (size_t i = 0; i < n; i++)
    {
        tmp.num=i;
        off[i] = mr.write(tmp);
        offSTD[i] = STD.write(tmp);
    }
    int r=0,rSTD=0;
    while (r<n)
    {
        r=0;
        rSTD = -1;
        mr.get_info(r,3);
        STD.get_info(rSTD,3);
        if (r != rSTD) {
            cout<<0;
            return 0;
        }
        mr.read(tmp,off[r]);
        STD.read(tmpSTD,offSTD[rSTD]);
        if (tmp.f() != tmpSTD.f()) {
            cout<<0;
            return 0;
        }
        r+=2;
        rSTD+=2;
        mr.write_info(r,3);
        STD.write_info(rSTD,3);
    }
    for (size_t i = 0; i < n; i++)
    {
        mr.read(tmp,off[i]);
        STD.read(tmpSTD,offSTD[i]);
        if (tmp!=tmpSTD) {
            cout<<0;
            return 0;
        }
    }
    cout<<1;
    return 0;
}