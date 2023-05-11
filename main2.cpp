#include "hw2_output.h"
#include<iostream>
#include<vector>
#include <pthread.h>
#include<string>
#include <stdlib.h>
#include <cstring>
#include <sstream>
#include <cmath>
#include <semaphore.h>
#include <algorithm>

using namespace std;


int n,m,k;
int **A, **B, **C, **D, **J, **L, **R;
vector<int> signalled_columns;

int finished_rows_JL=0;


pthread_mutex_t finished_jl_row;
pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;


sem_t *ready_J; // for each row
sem_t *ready_L; //for each element
sem_t all_done;

int grb2 = sem_init(&all_done,0,0);

void multiplyMatrix(int cur_row, int cur_col)
{

       
        int val = 0;
        for (int k = 0; k < m; k++)
        {
            val += J[cur_row][k] * L[k][cur_col];
            
        }
        R[cur_row][cur_col] = val;

        hw2_write_output(2,cur_row+1,cur_col+1,val);
        

    
    
}
void* addRowAB(void* args)
{

    int cur_row = *((int*) args);
    
    for(int i=0;i<m;i++)
    {
        int val = A[cur_row][i] + B[cur_row][i];
        J[cur_row][i] = val;
        hw2_write_output(0,cur_row+1,i+1,val);
    }
    sem_post(&ready_J[cur_row]);
    pthread_exit(0);
}


void checkTheColumnOfL(int i)
{
   
    for(int j=0;j<m;j++)
    {
        if(L[j][i] == -1890)
        {
            
            return;
        }
    }
    
        sem_post(&ready_L[i]);
    
}

void* addRowCD(void* args)
{

    int cur_row = *((int*) args);
    
    for(int i=0;i<k;i++)
    {
        int val = C[cur_row][i] + D[cur_row][i];
        hw2_write_output(1,cur_row+1,i+1,val);
        L[cur_row][i] = val;
        
        
        
        
        checkTheColumnOfL(i);

        
    }
    
    pthread_exit(0);
}


void* multiplyJL(void* args)
{
    int cur_row = *((int*) args);
    
    sem_wait(&ready_J[cur_row]);


    for(int i=0;i<k;i++)
    {
        
            sem_wait(&ready_L[i]);           //think what happens if context switch between these two
            multiplyMatrix(cur_row, i); // i changed the order 
            sem_post(&ready_L[i]);
            

        
        
    }



    

    pthread_mutex_lock(&finished_jl_row);
    finished_rows_JL++;
    if(finished_rows_JL == n)
    {
        sem_post(&all_done);
    }
    pthread_mutex_unlock(&finished_jl_row);
    pthread_exit(0);
}
void getInitialMatrix(int**& M, int i, int n,int nn, vector<vector<int>>& lines)
{
    
    int s;
    int check;
    if(i<2)
    {

        s=(i)*(n+1) +1;

        check=n;
        

    }
    else
    {
        s = (2)*(n+1) +(i-2)*(nn+1)+1;
        check=nn;
    }

    


    for(int flag = 0; flag < check;s++,flag++)
    {
       for(int t = 0 ; t<lines[s].size();t++)
       {
            M[flag][t] = lines[s][t];
       }
    }
}

void initializeMatrix(int**& M, int r, int c)
{
    for(int i=0;i<r;i++)
    {
        
        for(int j=0;j<c;j++)
        {
            M[i][j] = -1890;
        }
    }
}

int main()
{
    hw2_init_output();
    
    string l;
    vector<vector<int>> lines;
    int line_no = 0;
    while (getline(cin, l))
    {
        if (l.empty())
        {
            break;
        }
        lines.push_back(vector<int>());

        istringstream ss(l);

        string word;
        while (ss >> word)
        {

            lines[line_no].push_back(stoi(word));
        }
        line_no++;
    }
    /*
    for (int i = 0; i < line_no ; i ++)
    {
        for(int j=0;j<lines[i].size();j++)
        {
            cout<<lines[i][j]<<" ";
        }
        cout<<endl;
    }
    */

   
    n = lines[0][0];
    m = lines[0][1];
    k = lines[2*n+2][1];
    //cout<<"n is: "<<n<<" m is: "<<m<<" k is: "<<k<<endl;
    A = new int*[n]; for(int i=0;i<n;i++){A[i] = new int[m];}
    B = new int*[n]; for(int i=0;i<n;i++){B[i] = new int[m];}
    C = new int*[m]; for(int i=0;i<m;i++){C[i] = new int[k];}
    D = new int*[m]; for(int i=0;i<m;i++){D[i] = new int[k];}
    J = new int*[n]; for(int i=0;i<n;i++){J[i] = new int[m];}
    L = new int*[m]; for(int i=0;i<m;i++){L[i] = new int[k];}
    R = new int*[n]; for(int i=0;i<n;i++){R[i] = new int[k];}

    ready_J = new sem_t[n]; for(int i=0;i<n;i++) {int g = sem_init(&ready_J[i],0,0);}
    ready_L = new sem_t[k]; for(int i=0;i<k;i++) {int g = sem_init(&ready_L[i],0,0);}
    
    int thread_count = 2*n+m;
    
    
    getInitialMatrix(A,0,n,m,lines);
    
    getInitialMatrix(B,1,n,m,lines);
    
    getInitialMatrix(C,2,n,m,lines);
    
    getInitialMatrix(D,3,n,m,lines);
    
    initializeMatrix(J,n,m);
    initializeMatrix(L,m,k);
    initializeMatrix(R,n,k);
    
    int args_AB[n]; 
    int args_CD[m]; 
    for(int i=0;i<n;i++)
    {
        args_AB[i]=i;
    }
    for(int i=0;i<m;i++)
    {
        args_CD[i]=i;
    }

    
    
    pthread_t threads[thread_count];

    
    for (int i = 0; i < n; i++)
    {
        
		pthread_create(&threads[i], NULL, addRowAB, (args_AB+i));
    }
    for (int i = n, j=0; i < n+m; i++,j++)
    {
        
		pthread_create(&threads[i], NULL, addRowCD, (args_CD+j));
    }
    for (int i = n+m, k=0; i < n+m+n; i++,k++)
    {
        
		pthread_create(&threads[i], NULL, multiplyJL, (args_AB+k));
    }


    sem_wait(&all_done);

    for(int i=0;i<thread_count;i++)
    {
        pthread_join(threads[i], NULL);
        
    }
	
    for(int i=0;i<n;i++)
    {
        int col;
        for(col=0;col<k-1;col++)
        {
            cout<<R[i][col]<<" ";
        }
        cout<<R[i][col]<<endl;
    }
    

    for(int i=0;i<n;i++)
    {
        delete[] A[i];
        delete[] B[i];
        delete[] J[i];
        delete[] R[i];
    }
    delete[] A; delete[] B; delete[] J; delete[] R; 
    delete[] ready_J; delete[] ready_L;


    for(int i=0;i<m;i++)
    {
        delete[] C[i];
        delete[] D[i];
        delete[] L[i];
    }
    delete[] C; delete[] D; delete[] L;




    return 0;
}
