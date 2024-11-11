#include<iostream>
using namespace std;
void merge_Combine(int*A, int n);
void merge_Sort(int A[],int n)
{
    if (n == 1) return;
    if (n > 2){
        merge_Sort(A,n/2);
        merge_Sort(A+n/2+1,n - n/2);
        merge_Combine(A,n);
    }
}
void merge_Combine(int*A, int n)
{
    int B[n];
    for (int i = 0,j = 0; i+j < n;)
    {
        if (A[i] <= A[n/2+j]){
            B[i+j] = A[i];
            i++;
            if (i == n/2){
                for (;j <= n-n/2-1;j++){
                    B[i+j] = A[n/2+j];
                }
            }
        }
        if (A[i] > A[n/2+j]){
            B[i+j] = A[n/2+j];
            j++;
            if (j == n - n/2){
                for (;i <= n/2;i++){
                    B[i+j] = A[i];
                }
            }
        }
    }
    for (int i = 0; i < n; i++){
        A[i] = B[i];
    }
}
int main()
{
    int n,num[100];
    cin>>n;
    for (int i = 0; i < n; i++){
        cin >> num[i];
    }
    // 你的代码
    merge_Sort(num,n);
    for (int i = 0; i < n; i++){
        if (i) cout <<' ';
        cout << num[i];
    }
    return 0;
}


/*
in:
3 3 2 -1
out:
-1 2 3
 */
