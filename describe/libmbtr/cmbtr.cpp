#include "cmbtr.h"
#include <vector>
#include <map>
#include <utility>
#include <cmath>
#include <algorithm>
using namespace std;

CMBTR::CMBTR(vector<vector<float> > positions, vector<int> atomicNumbers, map<int,int> atomicNumberToIndexMap, int cellLimit)
    : positions(positions)
    , atomicNumbers(atomicNumbers)
    , atomicNumberToIndexMap(atomicNumberToIndexMap)
    , cellLimit(cellLimit)
{
}

vector<vector<vector<float> > > CMBTR::getDisplacementTensor()
{
    int nAtoms = this->atomicNumbers.size();

    // Initialize tensor
    vector<vector<vector<float> > > tensor(nAtoms, vector<vector<float> >(nAtoms, vector<float>(3)));

    // Calculate the distance between all pairs and store in the tensor
    for (int i=0; i < nAtoms; ++i) {
        for (int j=0; j < nAtoms; ++j) {

            // Due to symmetry only upper triangular part is processed.
            if (i <= j) {
                continue;
            }

            // Calculate distance between the two atoms, store in tensor
            vector<float>& iPos = this->positions[i];
            vector<float>& jPos = this->positions[j];
            vector<float> diff(3);
            vector<float> negDiff(3);

            for (int k=0; k < 3; ++k) {
                float iDiff = jPos[k] - iPos[k];
                diff[k] = iDiff;
                negDiff[k] = -iDiff;
            }

            tensor[i][j] = diff;
            tensor[j][i] = negDiff;
        }
    }

    return tensor;
}
vector<vector<float> > CMBTR::getDistanceMatrix()
{
    int nAtoms = this->atomicNumbers.size();

    // Initialize matrix
    vector<vector<float> > distanceMatrix(nAtoms, vector<float>(nAtoms));

    // Displacement tensor
    vector<vector<vector<float> > > tensor = this->getDisplacementTensor();

    // Calculate distances
    for (int i=0; i < nAtoms; ++i) {
        for (int j=0; j < nAtoms; ++j) {

            // Due to symmetry only upper triangular part is processed.
            if (i <= j) {
                continue;
            }

            float norm = 0;
            vector<float>& iPos = tensor[i][j];
            for (int k=0; k < 3; ++k) {
                norm += pow(iPos[k], 2.0);
            }
            norm = sqrt(norm);
            distanceMatrix[i][j] = norm;
            distanceMatrix[j][i] = norm;
        }
    }

    return distanceMatrix;
}

vector<vector<float> > CMBTR::getInverseDistanceMatrix()
{
    int nAtoms = this->atomicNumbers.size();

    // Initialize matrix
    vector<vector<float> > inverseDistanceMatrix(nAtoms, vector<float>(nAtoms));

    // Distance matrix
    vector<vector<float> > distanceMatrix = this->getDistanceMatrix();

    // Calculate inverse distances
    for (int i=0; i < nAtoms; ++i) {
        for (int j=0; j < nAtoms; ++j) {

            // Due to symmetry only upper triangular part is processed.
            if (i <= j) {
                continue;
            }

            float inverse = 1.0/distanceMatrix[i][j];
            inverseDistanceMatrix[i][j] = inverse;
            inverseDistanceMatrix[j][i] = inverse;
        }
    }

    return inverseDistanceMatrix;
}

map<pair<int, int>, vector<float> > CMBTR::getInverseDistanceMap()
{
    int nAtoms = this->atomicNumbers.size();

    // Initialize the map containing the mappings
    map<pair<int, int>, vector<float> > inverseDistanceMap;

    // Inverse distance matrix
    vector<vector<float> > inverseDistanceMatrix = this->getInverseDistanceMatrix();

    // Calculate
    for (int i=0; i < nAtoms; ++i) {
        for (int j=0; j < nAtoms; ++j) {
            int i_elem = this->atomicNumbers[i];
            int j_elem = this->atomicNumbers[j];

            // Due to symmetry only upper triangular part is processed.
            if (j <= i) {
                continue;
            }

            // The value is calculated only if either atom is inside the
            // original cell.
            if (i < this->cellLimit || j < this->cellLimit) {
                int i_index = this->atomicNumberToIndexMap[i_elem];
                int j_index = this->atomicNumberToIndexMap[j_elem];

                // We fill only the upper triangular part of the map by saving
                // both (i, j) and (j, i) into the map location where j >= i.
                vector<int> sorted(2);
                sorted[0] = i_index;
                sorted[1] = j_index;
                sort(sorted.begin(), sorted.end());
                i_index = sorted[0];
                j_index = sorted[1];

                // Get the list of old values at this location. If does not
                // exist, create new list. Add new value to list.
                map<pair<int,int>, vector<float> >::iterator iter;
                pair<int,int> loc = make_pair(i_index, j_index);
                iter = inverseDistanceMap.find(loc);
                float ij_inv = inverseDistanceMatrix[i][j];
                if (iter == inverseDistanceMap.end() ) {
                    vector<float> newList;
                    newList.push_back(ij_inv);
                    inverseDistanceMap[loc] = newList;
                } else {
                    iter->second.push_back(ij_inv);
                }
            }
        }
    }

    return inverseDistanceMap;
}
