/*  Copyright 2014 - UVSQ, Dassault Aviation
    Authors list: Loïc Thébault, Eric Petit

    This file is part of Mini-FEM.

    Mini-FEM is free software: you can redistribute it and/or modify it under the terms
    of the GNU Lesser General Public License as published by the Free Software
    Foundation, either version 3 of the License, or (at your option) any later version.

    Mini-FEM is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
    PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License along with
    Mini-FEM. If not, see <http://www.gnu.org/licenses/>. */

#ifdef XMPI
    #include <mpi.h>
#elif GASPI
    #include <GASPI.h>
#endif
#include <stdio.h>

#include "globals.h"
#include "halo.h"

// Halo exchange between distributed domains
void halo_exchange (double *prec, int *intfIndex, int *intfNodes, int *neighborList,
                    int nbNodes, int nbIntf, int nbIntfNodes, int operatorDim,
                    int operatorID, int rank)
{
    // Initialize communication buffers
    double *bufferSend = new double [nbIntfNodes*operatorDim];
    double *bufferRecv = new double [nbIntfNodes*operatorDim];
    int source, dest, size, tag, node1, node2 = 0;

    // Initializing reception from adjacent domains
    for (int i = 0; i < nbIntf; i++) {
        node1  = node2;
        node2  = intfIndex[i+1];
        size   = (node2 - node1) * operatorDim;
        source = neighborList[i] - 1;
        tag    = neighborList[i] + 100;
        #ifdef XMPI
            MPI_Irecv (&(bufferRecv[node1*operatorDim]), size, MPI_DOUBLE_PRECISION,
                       source, tag, MPI_COMM_WORLD, &(neighborList[2*nbIntf+i]));
        #elif GASPI
        #endif
    }

    // Buffering local data
    node2 = 0;
    // Laplacian operator
    if (operatorID == 0) {
        for (int i = 0; i < nbIntf; i++) {
            node1 = node2;
            node2 = intfIndex[i+1];
            for (int j = node1; j < node2; j++) {
                for (int k = 0; k < operatorDim; k++) {
                    int tmpNode = intfNodes[j] - 1;
                    bufferSend[j*operatorDim+k] = prec[k*operatorDim+tmpNode];
                }
            }
        }
    }
    // Elasticity operator
    else {
        for (int i = 0; i < nbIntf; i++) {
            node1 = node2;
            node2 = intfIndex[i+1];
            for (int j = node1; j < node2; j++) {
                for (int k = 0; k < operatorDim; k++) {
                    int tmpNode = intfNodes[j] - 1;
                    bufferSend[j*operatorDim+k] = prec[tmpNode*operatorDim+k];
                }
            }
        }
    }

    // Sending local data to adjacent domains
    node2 = 0;
    for (int i = 0; i < nbIntf; i++) {
        node1 = node2;
        node2 = intfIndex[i+1];
        size  = (node2 - node1) * operatorDim;
        dest  = neighborList[i] - 1;
        tag   = rank + 101;
        #ifdef XMPI
            MPI_Send (&(bufferSend[node1*operatorDim]), size, MPI_DOUBLE_PRECISION,
                      dest, tag, MPI_COMM_WORLD);
        #elif GASPI
        #endif
    }

    #ifdef XMPI
        // Waiting incoming data
        for (int i = 0; i < nbIntf; i++) {
            MPI_Wait (&(neighborList[2*nbIntf+i]), MPI_STATUS_IGNORE);
        }
    #endif

    // Assembling local and incoming data
    node2 = 0;
    // Laplacian operator
    if (operatorID == 0) {
        for (int i = 0; i < nbIntf; i++) {
            node1  = node2;
            node2  = intfIndex[i+1];
            for (int j = node1; j < node2; j++) {
                for (int k = 0; k < operatorDim; k++) {
                    int tmpNode = intfNodes[j] - 1;
                    prec[k*operatorDim+tmpNode] += bufferRecv[j*operatorDim+k];
                }
            }
        }
    }
    // Elasticity operator
    else {
        for (int i = 0; i < nbIntf; i++) {
            node1  = node2;
            node2  = intfIndex[i+1];
            for (int j = node1; j < node2; j++) {
                for (int k = 0; k < operatorDim; k++) {
                    int tmpNode = intfNodes[j] - 1;
                    prec[tmpNode*operatorDim+k] += bufferRecv[j*operatorDim+k];
                }
            }
        }
    }

    // Free communication buffers
    delete[] bufferRecv, delete[] bufferSend;
}
