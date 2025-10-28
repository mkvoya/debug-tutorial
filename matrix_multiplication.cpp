#include <iostream>
#include <vector>
using namespace std;

// Function to multiply two matrices
vector<vector<int>> multiplyMatrix(const vector<vector<int>>& matrix1, 
                                 const vector<vector<int>>& matrix2) {
    int rows1 = matrix1.size();
    int cols1 = matrix1[0].size();
    int cols2 = matrix2[0].size();
    
    // Initialize result matrix with zeros
    vector<vector<int>> result(rows1, vector<int>(cols2, 0));
    
    // Perform matrix multiplication
    for(int i = 0; i < rows1; i++) {
        for(int j = 0; j < cols2; j++) {
            for(int k = 0; k < cols1; k++) {
                result[i][j] += matrix1[i][k] * matrix2[k][j];
            }
        }
    }
    
    return result;
}

// Function to print matrix
void printMatrix(const vector<vector<int>>& matrix) {
    for(const auto& row : matrix) {
        for(int val : row) {
            cout << val << " ";
        }
        cout << endl;
    }
}

int main() {
    // Example input matrices
    vector<vector<int>> matrix1(1000, vector<int>(1000));
    for(int i = 0; i < 1000; i++) {
        for(int j = 0; j < 1000; j++) {
            matrix1[i][j] = i + j; // Fill with some values
        }
    }
    
    vector<vector<int>> matrix2(1000, vector<int>(1000));
    for(int i = 0; i < 1000; i++) {
        for(int j = 0; j < 1000; j++) {
            matrix2[i][j] = i * j; // Fill with some values
        }
    }
    
    // Perform multiplication
    vector<vector<int>> result = multiplyMatrix(matrix1, matrix2);
    
    // Calculate rank by finding number of non-zero rows in row echelon form
    int rank = 0;
    int rows = result.size();
    int cols = result[0].size();
    
    vector<vector<double>> temp(rows, vector<double>(cols));
    // Convert to double for better numerical stability
    for(int i = 0; i < rows; i++) {
        for(int j = 0; j < cols; j++) {
            temp[i][j] = static_cast<double>(result[i][j]);
        }
    }
    
    // Convert to row echelon form
    int lead = 0;
    for(int r = 0; r < rows; r++) {
        if(lead >= cols) break;
        
        int i = r;
        while(temp[i][lead] == 0) {
            i++;
            if(i == rows) {
                i = r;
                lead++;
                if(lead == cols) goto done;
            }
        }
        
        // Swap rows i and r
        swap(temp[i], temp[r]);
        
        // Normalize row r
        double lv = temp[r][lead];
        for(int j = 0; j < cols; j++) {
            temp[r][j] /= lv;
        }
        
        // Eliminate column
        for(int i = 0; i < rows; i++) {
            if(i != r) {
                double multiplier = temp[i][lead];
                for(int j = 0; j < cols; j++) {
                    temp[i][j] -= multiplier * temp[r][j];
                }
            }
        }
        lead++;
        rank++;
    }
    done:
    
    cout << "Rank of result matrix: " << rank << endl;
    
    return 0;
} 