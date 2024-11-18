#include <iostream>
#include <cmath>
using namespace std;

int main()
{
    int r;
    cin >> r;
    const int R=r;
    int a[1000][1000]={0};
    for(int i=0;i<R;i++){
        for(int j=1;j<i+1;j++) cin>>a[i][j];
    }
    //  (0,1)
    //  (1,1) (1,2)
    //  (2,1) (2,2) (2,3)
    //
    for(int i=1;i<R;i++){
        for(int j=1;j<i+1;j++){
            a[i][j]+=max(a[i-1][j-1],a[i-1][j]);
	    // a[1][1] += max(a[0][0], a[0][1])
	    // a[2][1] += max(a[1][0], a[1][1])
        }
    }
    int ans=0;
    for(int j=1;j<=R+1;j++){
        if(a[R-1][j]>ans) ans=a[R-1][j];
    }
    cout<<ans<<endl;
    return 0;
}

/*

in:
5 7 3 8 8 1 1 2 7 4 4 4 5 2 6 5
out:
30
illustration:
        7
       3 8
      8 1 1
     2 7 4 4
    4 5 2 6 5
 */
